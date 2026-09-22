"""
MedPulse Smart-Care AI — Sub-Station Client Bridge
Runs on Sub-Station PCs (Connected to NFC Terminal Arduino via USB).

Features:
- Auto-detects Arduino COM port & auto-reconnects on disconnection.
- Receives Nurse NFC Card scans ('SCAN:UID'), validates with Cloud API.
- Sends TFT feedback ('TFT_SUCCESS:Name' / 'TFT_ERROR') to Arduino Round Display.
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
STATION_ID      = os.environ.get('STATION_ID', 'STA-02')
STATION_API_KEY = os.environ.get('STATION_API_KEY', 'medpulse-station-secret-key')
COM_PORT_OVERRIDE = os.environ.get('COM_PORT', '')

NFC_ACK_API_URL = f"{SERVER_BASE_URL}/api/acknowledge_nfc/"
HEARTBEAT_URL   = f"{SERVER_BASE_URL}/api/heartbeat/"

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

# ── NFC Verification Handler ────────────────────────────────────
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
    print(f"  MedPulse Smart-Care AI — Sub-Station Bridge ({STATION_ID})")
    print(f"  Server:     {SERVER_BASE_URL}")
    print("=" * 60)

    threading.Thread(target=heartbeat_worker, daemon=True).start()

    while True:
        port = find_arduino_port()
        if not port:
            print("[SEARCHING] NFC Terminal not found. Retrying in 2 seconds...")
            time.sleep(2)
            continue

        try:
            print(f"[CONNECTING] Connecting to NFC Terminal on {port} (9600 baud)...")
            ser = serial.Serial(port, 9600, timeout=0.1)
            ser_global = ser
            print(f"[CONNECTED] NFC Terminal online on {port} ✅")
            print("-" * 60)

            while ser.is_open:
                if ser.in_waiting > 0:
                    raw = ser.readline()
                    line = raw.decode('utf-8', errors='ignore').strip()
                    if not line:
                        continue

                    print(f"[RECV] {line}")

                    if line.startswith('SCAN:'):
                        parts = line.split(':')
                        if len(parts) >= 2:
                            uid = parts[1].strip()
                            threading.Thread(target=handle_nfc_scan, args=(uid,), daemon=True).start()

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
