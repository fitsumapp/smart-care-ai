"""
simulate_call.py — MedPulse Smart-Care AI Interactive Simulator
Use this script to test all calls (LoRa, AI Fall, AI Tremor, NFC Scan, Public Area Alerts) on localhost.
"""

import requests
import time

BASE_API_URL  = 'http://127.0.0.1:8000/api/calls/'
NFC_API_URL   = 'http://127.0.0.1:8000/api/acknowledge_nfc/'
CLEAR_API_URL = 'http://127.0.0.1:8000/api/calls/clear/'

def send_call(room, bed='General', action='start', call_type=None, location_type=None, notes=None):
    payload = {
        'room_number': str(room),
        'bed_number': str(bed),
        'action': action
    }
    if call_type:
        payload['call_type'] = call_type
    if location_type:
        payload['location_type'] = location_type
    if notes:
        payload['notes'] = notes

    try:
        r = requests.post(BASE_API_URL, data=payload, timeout=3)
        if r.status_code in [200, 201]:
            print(f"  [SUCCESS] Call sent: {room} ({bed}) | Action: {action} | Type: {call_type or 'Default'} | Loc: {location_type or 'ROOM'}")
        else:
            print(f"  [FAILED] Server returned status {r.status_code}: {r.text}")
    except Exception as e:
        print(f"  [ERROR] Could not connect to server: {e}")

def send_nfc_scan(uid, station_name="Station 1"):
    payload = {
        'uid': uid,
        'station_name': station_name
    }
    try:
        r = requests.post(NFC_API_URL, json=payload, timeout=3)
        data = r.json()
        if r.status_code == 200 and data.get('status') == 'success':
            print(f"  [NFC ACCEPTED] Nurse: {data.get('nurse_name')} | Card UID: {uid} | Room Cleared: {data.get('room')}")
        else:
            print(f"  [NFC REJECTED] {data.get('message', 'Card not recognized')} | UID: {uid}")
    except Exception as e:
        print(f"  [ERROR] NFC request failed: {e}")

def clear_room(room):
    try:
        r = requests.post(CLEAR_API_URL, json={'room_number': str(room)}, timeout=3)
        print(f"  [CLEARED] Room/Area {room} cleared.")
    except Exception as e:
        print(f"  [ERROR] Clear failed: {e}")

if __name__ == "__main__":
    print("=" * 65)
    print("  🏥 MedPulse Smart-Care AI — Localhost Call Simulator")
    print("  Server: http://127.0.0.1:8000")
    print("=" * 65)

    while True:
        print("\n--- Select an Action to Test ---")
        print("1. 🛏️  Standard Patient Call (LoRa Bed 1, 2, 3...)")
        print("2. 🚨 AI Fall in Patient Room (e.g. Room 5)")
        print("3. 🏢 AI Fall in CORRIDOR (e.g. Corridor 2nd Floor)")
        print("4. 🪜 AI Fall on STAIRS (e.g. Staircase Block-B)")
        print("5. ⚠️  AI Tremor / Seizure Alert (Room 10)")
        print("6. 🏷️  Nurse NFC Card Scan (Acknowledge Call)")
        print("7. 👩‍⚕️ Nurse Arrived at Room (Stop Timer)")
        print("8. 🚨 Doctor Entering (Special ER Room Popup)")
        print("9. 🧹 Clear All Active Calls")
        print("q. ❌ Quit")

        choice = input("\nEnter choice (1-9 / q): ").strip()

        if choice == 'q':
            print("Simulator closed.")
            break

        elif choice == '1':
            room = input("  Enter Room Number (e.g. 5): ").strip() or "5"
            bed = input("  Enter Bed (e.g. 1, 2, Bathroom): ").strip() or "Bed 1"
            send_call(room, bed, 'start')

        elif choice == '2':
            room = input("  Enter Room Number for Fall Alert (default 5): ").strip() or "5"
            print(f"  Simulating AI Camera detecting patient fall in Room {room}...")
            send_call(room, 'General', 'emergency', call_type='AI_FALL', location_type='ROOM', notes=f'AI Vision: Fall in Room {room}')

        elif choice == '3':
            corridor = input("  Enter Corridor Name (default 'Corridor 2nd Floor'): ").strip() or "Corridor 2nd Floor"
            print(f"  Simulating AI Camera detecting fall in {corridor}...")
            send_call(corridor, 'Area', 'emergency', call_type='AI_FALL', location_type='CORRIDOR', notes=f'AI Vision: Fall in {corridor}')

        elif choice == '4':
            stairs = input("  Enter Stairs Name (default 'Staircase Block-B'): ").strip() or "Staircase Block-B"
            print(f"  Simulating AI Camera detecting fall on {stairs}...")
            send_call(stairs, 'Area', 'emergency', call_type='AI_FALL', location_type='STAIRS', notes=f'AI Vision: Fall on {stairs}')

        elif choice == '5':
            room = input("  Enter Room Number for Tremor Alert (default 10): ").strip() or "10"
            print(f"  Simulating AI Camera detecting tremor in Room {room}...")
            send_call(room, 'General', 'urgent', call_type='AI_TREMOR', location_type='ROOM', notes=f'AI Vision: Tremor in Room {room}')

        elif choice == '6':
            uid = input("  Enter Nurse NFC Card UID (or press Enter for default 'A1B2C3D4'): ").strip() or "A1B2C3D4"
            send_nfc_scan(uid)

        elif choice == '7':
            room = input("  Enter Room Number: ").strip() or "5"
            send_call(room, 'General', 'arrived')

        elif choice == '8':
            room = input("  Enter Special Room ID (e.g. ER1): ").strip() or "ER1"
            send_call(room, 'General', 'emergency')

        elif choice == '9':
            try:
                calls = requests.get(BASE_API_URL, timeout=3).json()
                for c in calls:
                    clear_room(c['room'])
                print(f"  Cleared {len(calls)} active call(s).")
            except Exception as e:
                print(f"  Failed to clear calls: {e}")

        else:
            print("  Invalid choice, please try again.")
