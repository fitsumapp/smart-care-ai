# concurrent_test.py - Tests two simultaneous calls to the dashboard
import requests
import threading
import time

BASE = 'http://127.0.0.1:8000/api/calls/'

def send(room, bed, label):
    payload = {'room_number': str(room), 'bed_number': str(bed), 'action': 'start'}
    t0 = time.time()
    try:
        r = requests.post(BASE, data=payload, timeout=5)
        elapsed = time.time() - t0
        print(f"  [{label}] Room {room} -> {r.status_code} ({elapsed:.2f}s)")
    except Exception as e:
        print(f"  [{label}] Room {room} ERROR: {e}")

def clear_all():
    try:
        calls = requests.get(BASE, timeout=3).json()
        for c in calls:
            requests.post('http://127.0.0.1:8000/api/calls/clear/',
                json={'room_number': c['room']},
                headers={'Content-Type': 'application/json', 'X-CSRFToken': 'test'},
                timeout=2)
        print(f"  Cleared {len(calls)} call(s)")
    except Exception as e:
        print(f"  Clear failed: {e}")

def check_dashboard():
    try:
        calls = requests.get(BASE, timeout=3).json()
        print(f"\n  Dashboard Active Calls: {len(calls)}")
        for c in calls:
            print(f"     Room {c['room']} | Priority {c['priority']} | "
                  f"Duration {c['duration']}s | Ack: {c['is_acknowledged']}")
        return len(calls)
    except Exception as e:
        print(f"  Check failed: {e}")
        return 0

print("=" * 55)
print("  AI Nurse - Concurrent Call Test")
print("=" * 55)
print("\nOptions:")
print("  1 -> Room 4 (Critical) + Room 5 (Normal)")
print("  2 -> Room 4 (Normal)   + Room 5 (Normal)")
print("  3 -> Enter custom rooms")
choice = input("\nSelect (1/2/3): ").strip()

if choice == '1':
    calls = [('4', '1', 'Room4-Critical'), ('5', '1', 'Room5-Normal')]
elif choice == '2':
    calls = [('4', '1', 'Room4-Normal'), ('5', '1', 'Room5-Normal')]
elif choice == '3':
    r1 = input("  Room number 1: ").strip()
    r2 = input("  Room number 2: ").strip()
    calls = [(r1, '1', f'Room{r1}'), (r2, '1', f'Room{r2}')]
else:
    print("Invalid choice")
    exit()

# Step 1: Clear
print("\n[1] Clearing existing calls...")
clear_all()
time.sleep(0.5)

# Step 2: Send simultaneously
print(f"\n[2] Sending {len(calls)} calls SIMULTANEOUSLY...")
threads = [threading.Thread(target=send, args=c) for c in calls]
for t in threads: t.start()
for t in threads: t.join()

# Step 3: Check immediately
print("\n[3] Dashboard check (immediate):")
n = check_dashboard()

# Step 4: Wait and check again
print("\n[4] Waiting 5 seconds...")
time.sleep(5)
print("[5] Dashboard check (after 5s):")
check_dashboard()

# Verdict
print("\n" + "=" * 55)
if n >= len(calls):
    print("  RESULT: SERVER OK - Both calls recorded!")
    print("  If dashboard still shows 1 -> JS/filter issue")
elif n == 1:
    print("  RESULT: SERVER ISSUE - Only 1 call recorded!")
    print("  Second call did not reach the server")
else:
    print("  RESULT: No calls recorded - server connection issue")
print("=" * 55)
