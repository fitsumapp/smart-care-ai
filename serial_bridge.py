"""
serial_bridge.py — MedPulse Smart-Care AI Hardware Bridge
Reads Arduino serial signals (LoRa calls, NFC card scans) and forwards them to Django server.
Uses threading for non-blocking HTTP calls so simultaneous signals are never lost.
"""
import serial
import serial.tools.list_ports
import requests
import time
import threading

BASE_URL        = 'http://127.0.0.1:8000/api/calls/'
NFC_URL         = 'http://127.0.0.1:8000/api/acknowledge_nfc/'
RESET_CHECK_URL = 'http://127.0.0.1:8000/api/calls/check_reset/'
HEARTBEAT_URL   = 'http://127.0.0.1:8000/api/heartbeat/'

# ─── Find Arduino port ──────────────────────────────────────────
def find_arduino():
    ports = list(serial.tools.list_ports.comports())
    if not ports:
        return None
    
    print("\n🔍 Available COM Ports:")
    for p in ports:
        print(f"   • {p.device}: {p.description}")
    
    # 1. Prefer USB Serial / Arduino / CH340 devices
    for p in ports:
        desc = (p.description or '').lower()
        if any(x in desc for x in ['arduino', 'ch340', 'ch341', 'cp210', 'ftdi', 'usb-serial', 'usb serial']):
            print(f"🎯 Selected Arduino port by description: {p.device}")
            return p.device

    # 2. Test ports that can be opened
    for p in ports:
        try:
            s = serial.Serial(p.device, 9600, timeout=0.2)
            s.close()
            print(f"🎯 Selected available port: {p.device}")
            return p.device
        except Exception:
            continue
            
    return ports[0].device if ports else None

port = find_arduino()
if not port:
    print("\n❌ ERROR: Arduino not found! Please connect USB cable.")
    print("⚠️ If Arduino IDE Serial Monitor is open, PLEASE CLOSE IT FIRST (ports cannot be shared).")
    exit(1)

try:
    ser = serial.Serial(port, 9600, timeout=0.1)
    print(f"\n✅ Connected successfully to {port} at 9600 baud.")
except Exception as e:
    print(f"\n❌ ERROR connecting to {port}: {e}")
    print("⚠️ TIP: Make sure Arduino IDE Serial Monitor is CLOSED so Python can access the port.")
    exit(1)

print("🚀 MedPulse Serial Bridge running... Waiting for LoRa calls and NFC scans.")
print("=" * 65)

# ─── HTTP senders (run in separate threads = non-blocking) ──────
def send_start(room, bed):
    payload = {'room_number': room, 'bed_number': bed, 'action': 'start'}
    try:
        r = requests.post(BASE_URL, data=payload, timeout=4)
        status = r.status_code
        body   = r.text[:80]
        print(f"  [CALL SENT] Room={room} Bed={bed} | HTTP {status} | {body}")
    except Exception as e:
        print(f"  [CALL FAILED] Room={room}: {e}")

def send_arrived(room):
    payload = {'room_number': room, 'action': 'arrived'}
    try:
        r = requests.post(BASE_URL, data=payload, timeout=4)
        print(f"  [NURSE ARRIVED] Room={room} | HTTP {r.status_code}")
    except Exception as e:
        print(f"  [ARRIVED FAILED] Room={room}: {e}")

def send_nfc_scan(uid):
    payload = {'uid': uid, 'station_name': 'Nurse Station 1'}
    try:
        r = requests.post(NFC_URL, json=payload, timeout=4)
        if r.status_code == 200:
            data = r.json()
            if data.get('status') == 'success':
                nurse = data.get('nurse_name', 'Nurse')
                room_cleared = data.get('room')
                print(f"  [NFC ACCEPTED] Nurse: {nurse} | Cleared: {room_cleared or 'Active Call'}")
                # Notify Arduino TFT Screen of Success
                try:
                    ser.write(f"TFT_SUCCESS:{nurse}\n".encode())
                except Exception as ex:
                    print(f"  [SERIAL WRITE ERROR]: {ex}")
                return
            else:
                print(f"  [NFC REJECTED] {data.get('message', 'Unrecognized Card')}")
        else:
            print(f"  [NFC ERROR] HTTP {r.status_code}: {r.text}")
        
        # Send error to Arduino TFT
        try:
            ser.write(b"TFT_ERROR\n")
        except Exception:
            pass
    except Exception as e:
        print(f"  [NFC FAILED] Error: {e}")
        try:
            ser.write(b"TFT_ERROR\n")
        except Exception:
            pass

def send_heartbeat():
    while True:
        try:
            requests.get(HEARTBEAT_URL, timeout=3)
        except Exception:
            pass
        time.sleep(5)

# Start heartbeat thread
threading.Thread(target=send_heartbeat, daemon=True).start()

# ─── Reset check (once per second) ──────────────────────────────
last_reset_check = 0
signal_count = 0

while True:
    try:
        # Drain ALL pending serial lines in one iteration
        lines_this_tick = 0
        while ser.in_waiting > 0:
            raw  = ser.readline()
            line = raw.decode('utf-8', errors='ignore').strip()
            if not line:
                continue

            lines_this_tick += 1
            signal_count    += 1
            print(f"[{signal_count}] SERIAL IN: {line!r}")

            if line.startswith('START:'):
                parts = line.split(':')
                if len(parts) >= 3:
                    room, bed = parts[1].strip(), parts[2].strip()
                    print(f"    -> Dispatching CALL: Room={room}, Bed={bed}")
                    threading.Thread(target=send_start, args=(room, bed), daemon=True).start()
                elif len(parts) == 2:
                    room = parts[1].strip()
                    print(f"    -> Dispatching SPECIAL CALL: Room={room}")
                    threading.Thread(target=send_start, args=(room, "Special"), daemon=True).start()
                else:
                    print(f"    -> BAD FORMAT (expected START:room:bed)")

            elif line.startswith('ARRIVED:'):
                parts = line.split(':')
                if len(parts) >= 2:
                    room = parts[1].strip()
                    threading.Thread(target=send_arrived, args=(room,), daemon=True).start()

            elif line.startswith('SCAN:'):
                parts = line.split(':')
                if len(parts) >= 2:
                    uid = parts[1].strip()
                    print(f"    -> Dispatching NFC SCAN: UID={uid}")
                    threading.Thread(target=send_nfc_scan, args=(uid,), daemon=True).start()

            elif line.startswith('ACK:'):
                print(f"    -> Arduino ACK: {line}")

            elif line.startswith('[SYSTEM]'):
                print(f"    -> Arduino Boot: {line}")

            else:
                print(f"    -> OTHER: {line!r}")

        # Reset check (server -> Arduino) once per second
        now = time.time()
        if now - last_reset_check >= 1.0:
            last_reset_check = now
            try:
                resp = requests.get(RESET_CHECK_URL, timeout=0.5)
                if resp.status_code == 200:
                    for room in resp.json().get('reset_rooms', []):
                        ser.write(f"RESET:{room}\n".encode())
                        print(f"    RESET -> Arduino room {room}")
            except Exception:
                pass

    except serial.SerialException as e:
        print(f"SerialException: {e} - retrying in 2s...")
        time.sleep(2)
        try:
            ser.close()
            time.sleep(0.5)
            ser.open()
        except Exception:
            pass
    except Exception as e:
        print(f"Loop error: {e}")
        time.sleep(0.5)

    time.sleep(0.02)   # 20ms polling
