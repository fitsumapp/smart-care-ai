"""
MedPulse Smart-Care AI — Master Station Client Bridge
Runs on Master Nurse Station PC (Connected to Arduino via USB).

Features:
- Auto-detects Arduino COM port & auto-reconnects on disconnection.
- Receives LoRa Patient Calls ('START:room:bed', 'ARRIVED:room') and posts to Cloud API.
- Receives Nurse NFC Card scans ('SCAN:UID'), validates with Cloud API, and sends TFT feedback ('TFT_SUCCESS:Name' / 'TFT_ERROR').
- Polls Cloud API for Cleared/Reset calls and transmits 'RESET:room' to Arduino (LoRa feedback to patient room).
- Sends periodic heartbeat to Cloud API.
"""

import os
import sys
import time
import json
import threading
import requests
import serial
import serial.tools.list_ports
from dotenv import load_dotenv

load_dotenv()

# ── Configuration ───────────────────────────────────────────────
SERVER_BASE_URL = os.environ.get('MEDPULSE_SERVER_URL', 'https://your-domain.com').rstrip('/')
STATION_ID      = os.environ.get('STATION_ID', 'STA-01')
STATION_API_KEY = os.environ.get('STATION_API_KEY', 'medpulse-station-secret-key')
COM_PORT_OVERRIDE = os.environ.get('COM_PORT', '')  # Set e.g. 'COM3' or leave empty for auto-detect

CALLS_API_URL     = f"{SERVER_BASE_URL}/api/calls/"
NFC_ACK_API_URL   = f"{SERVER_BASE_URL}/api/acknowledge_nfc/"
RESET_CHECK_URL   = f"{SERVER_BASE_URL}/api/calls/check_reset/"
HEARTBEAT_URL     = f"{SERVER_BASE_URL}/api/heartbeat/"

HEADERS = {
    'X-Station-ID': STATION_ID,
    'X-Station-API-Key': STATION_API_KEY,
}

# ── Find Arduino Port ───────────────────────────────────────────
def find_arduino_port():
    if COM_PORT_OVERRIDE:
        return COM_PORT_OVERRIDE

    ports = list(serial.tools.list_ports.comports())
    for p in ports:
        desc = (p.description or '').lower()
        if 'ch340' in desc or 'arduino' in desc or 'usb-serial' in desc or 'ftdi' in desc:
            return p.device

    for p in ports:
        try:
            s = serial.Serial(p.device, 9600, timeout=0.2)
            s.close()
            return p.device
        except Exception:
            continue
    return None

# ── Thread-safe Serial Sender ───────────────────────────────────
serial_lock = threading.Lock()
ser_global = None

def write_serial(cmd_str):
    global ser_global
    with serial_lock:
        if ser_global and ser_global.is_open:
            try:
                ser_global.write(f"{cmd_str}\n".encode('utf-8'))
                ser_global.flush()
                print(f"  [SERIAL OUT] ➜ {cmd_str}")
            except Exception as e:
                print(f"  [SERIAL ERROR] Failed to write '{cmd_str}': {e}")

# ── API Handlers (Run in Worker Threads) ─────────────────────────
def handle_patient_call(room, bed, action='start'):
    payload = {
        'room_number': room,
        'bed_number': bed,
        'action': action,
        'station_id': STATION_ID
    }
    try:
        r = requests.post(CALLS_API_URL, data=payload, headers=HEADERS, timeout=4)
        print(f"  [API] Call SENT ➔ Room {room} ({bed}) | HTTP {r.status_code}")
    except Exception as e:
        print(f"  [API ERROR] Call failed for Room {room}: {e}")

def handle_nurse_arrived(room):
    payload = {
        'room_number': room,
        'action': 'arrived',
        'station_id': STATION_ID
    }
    try:
        r = requests.post(CALLS_API_URL, data=payload, headers=HEADERS, timeout=4)
        print(f"  [API] Nurse Arrived ➔ Room {room} | HTTP {r.status_code}")
    except Exception as e:
        print(f"  [API ERROR] Arrived failed for Room {room}: {e}")

def handle_nfc_scan(uid):
    payload = {
        'rfid_uid': uid,
        'station_id': STATION_ID
    }
    try:
        r = requests.post(NFC_ACK_API_URL, data=payload, headers=HEADERS, timeout=4)
        if r.status_code == 200:
            data = r.json()
            nurse_name = data.get('nurse_name', 'Nurse')
            print(f"  [API] NFC VERIFIED ➔ {nurse_name} ({uid})")
            write_serial(f"TFT_SUCCESS:{nurse_name}")
        else:
            print(f"  [API] NFC DENIED ➔ Unknown UID ({uid}) | HTTP {r.status_code}")
            write_serial("TFT_ERROR")
    except Exception as e:
        print(f"  [API ERROR] NFC request failed for UID {uid}: {e}")
        write_serial("TFT_ERROR")

# ── Periodic Reset Checker (Cloud -> LoRa) ──────────────────────
def reset_polling_worker():
    while True:
        try:
            r = requests.get(RESET_CHECK_URL, headers=HEADERS, timeout=2)
            if r.status_code == 200:
                data = r.json()
                reset_rooms = data.get('reset_rooms', [])
                for room in reset_rooms:
                    print(f"  [RESET QUEUE] Transmitting LoRa Reset for Room {room}")
                    write_serial(f"RESET:{room}")
        except Exception:
            pass
        time.sleep(1.0)

# ── Periodic Heartbeat ──────────────────────────────────────────
def heartbeat_worker():
    while True:
        try:
            requests.get(HEARTBEAT_URL, headers=HEADERS, timeout=3)
        except Exception:
            pass
        time.sleep(10.0)

# ── Main Bridge Loop ────────────────────────────────────────────
def main():
    global ser_global
    print("=" * 60)
    print("  MedPulse Smart-Care AI — Master Station Bridge")
    print(f"  Server:     {SERVER_BASE_URL}")
    print(f"  Station ID: {STATION_ID}")
    print("=" * 60)

    # Start background threads
    threading.Thread(target=reset_polling_worker, daemon=True).start()
    threading.Thread(target=heartbeat_worker, daemon=True).start()

    while True:
        port = find_arduino_port()
        if not port:
            print("[SEARCHING] Arduino not found. Retrying in 2 seconds...")
            time.sleep(2)
            continue

        try:
            print(f"[CONNECTING] Connecting to Arduino on {port} (9600 baud)...")
            ser = serial.Serial(port, 9600, timeout=0.1)
            ser_global = ser
            print(f"[CONNECTED] Master Gateway online on {port} ✅")
            print("-" * 60)

            while ser.is_open:
                if ser.in_waiting > 0:
                    raw = ser.readline()
                    line = raw.decode('utf-8', errors='ignore').strip()
                    if not line:
                        continue

                    print(f"[RECV] {line}")

                    # 1. Patient Call from LoRa
                    if line.startswith('START:'):
                        parts = line.split(':')
                        if len(parts) >= 3:
                            room, bed = parts[1].strip(), parts[2].strip()
                            threading.Thread(target=handle_patient_call, args=(room, bed, 'start'), daemon=True).start()
                        elif len(parts) == 2:
                            room = parts[1].strip()
                            threading.Thread(target=handle_patient_call, args=(room, 'General', 'start'), daemon=True).start()

                    # 2. Nurse Arrived from LoRa
                    elif line.startswith('ARRIVED:'):
                        parts = line.split(':')
                        if len(parts) >= 2:
                            room = parts[1].strip()
                            threading.Thread(target=handle_nurse_arrived, args=(room,), daemon=True).start()

                    # 3. Nurse NFC Card Scan from MFRC522
                    elif line.startswith('SCAN:'):
                        parts = line.split(':')
                        if len(parts) >= 2:
                            uid = parts[1].strip()
                            threading.Thread(target=handle_nfc_scan, args=(uid,), daemon=True).start()

                    # 4. LoRa ACK
                    elif line.startswith('ACK:'):
                        print(f"  [LORA ACK] {line}")

                time.sleep(0.02)

        except serial.SerialException as e:
            print(f"[DISCONNECTED] Serial connection lost: {e}")
            ser_global = None
            time.sleep(2)
        except Exception as e:
            print(f"[ERROR] Unexpected error: {e}")
            time.sleep(1)

if __name__ == '__main__':
    main()
