"""
MedPulse Smart-Care AI — AI Vision Patient Monitor
Runs on Raspberry Pi / Edge Device with Camera (Pi Camera or USB WebCam).

Features:
- Real-time Pose Estimation via MediaPipe Pose.
- 🚨 Fall Detection (Angle > 65°, Hip Y > 0.70, Confirmed for 3 seconds).
- ⚠️ Tremor / Seizure Detection (Rapid Wrist Trajectory Variance).
- Direct HTTPS Alert dispatch to Cloud Django API.
- Works with both Raspberry Pi Camera (rpicam-vid) and Standard USB Webcams.
- Automatic network error recovery and cooldown protection.
"""

import os
import sys
import time
import math
import subprocess
import collections
import numpy as np
import requests

# Graceful import check for OpenCV
try:
    import cv2
except ImportError:
    cv2 = None

# Optional: Load .env if present, otherwise uses default variables below
try:
    from dotenv import load_dotenv
    load_dotenv()
except ImportError:
    pass

# ================================================================
#  ⚙️ CONFIGURATION (በዚህ ክፍል ውስጥ በቀላሉ ይቀይሩ)
# ================================================================

# 1. የሰርቨሩ አድራሻ (Cloud Domain ለምሳሌ 'https://yourdomain.com' ወይም Localhost 'http://127.0.0.1:8000')
SERVER_BASE_URL = os.environ.get('MEDPULSE_SERVER_URL', 'http://127.0.0.1:8000').rstrip('/')
SERVER_URL      = f"{SERVER_BASE_URL}/api/calls/"

# 2. የካሜራው መገኛ ቦታ ስም (ለምሳሌ፦ "Room 5", "Corridor 2nd Floor", "Staircase Block-B")
LOCATION_NAME   = os.environ.get('LOCATION_NAME', os.environ.get('ROOM_NUMBER', 'Room 5'))
ROOM_NUMBER     = LOCATION_NAME  # Alias for compatibility

# 3. የቦታው አይነት፦ "ROOM", "CORRIDOR", "STAIRS", "BATHROOM", "WAITING_AREA"
LOCATION_TYPE   = os.environ.get('LOCATION_TYPE', 'ROOM').upper()

# 4. የታካሚው አልጋ (ለክፍል ካልሆነ 'General' ይተውት)
BED_NUMBER      = os.environ.get('BED_NUMBER', 'General')

# 5. የካሜራ እና የማሳያ ቅንብሮች
USE_PI_CAMERA   = os.environ.get('USE_PI_CAMERA', 'True').lower() == 'true'  # Pi Camera ከሆነ True፣ USB ካሜራ ከሆነ False
SHOW_PREVIEW    = os.environ.get('SHOW_PREVIEW', 'True').lower() == 'true'   # የስክሪን ማሳያ መስኮት
CAMERA_INDEX    = int(os.environ.get('CAMERA_INDEX', '0'))                   # ለ USB ካሜራ

# 6. የደህንነት ቁልፍ
STATION_API_KEY = os.environ.get('STATION_API_KEY', 'medpulse-station-secret-key')

# ── Detection Thresholds (የስሜታዊነት መለኪያዎች) ───────────────
FALL_ANGLE_THRESHOLD  = float(os.environ.get('FALL_ANGLE_THRESHOLD', '65.0'))
FALL_HIP_Y_THRESHOLD  = float(os.environ.get('FALL_HIP_Y_THRESHOLD', '0.70'))
FALL_CONFIRM_SECS     = float(os.environ.get('FALL_CONFIRM_SECS', '3.0'))
TREMOR_THRESHOLD      = float(os.environ.get('TREMOR_THRESHOLD', '0.022'))
TREMOR_CONFIRM_FRAMES = int(os.environ.get('TREMOR_CONFIRM_FRAMES', '15'))
COOLDOWN_SECONDS      = float(os.environ.get('COOLDOWN_SECONDS', '45.0'))
# ================================================================

# ── Camera Driver (Pi Camera via rpicam-vid or OpenCV WebCam) ──
class VideoCamera:
    def __init__(self, width=640, height=480, fps=15, use_pi=True):
        self.use_pi = use_pi
        self.proc = None
        self.cap = None
        self.buf = b''

        if self.use_pi:
            try:
                self.proc = subprocess.Popen([
                    'rpicam-vid', '-t', '0',
                    '--width',     str(width),
                    '--height',    str(height),
                    '--framerate', str(fps),
                    '--codec',     'mjpeg',
                    '--nopreview', '-o', '-'
                ], stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, bufsize=1024*1024)
                print("  [CAMERA] Initialized Raspberry Pi Camera (rpicam-vid) ✅")
            except Exception as e:
                print(f"  [CAMERA WARNING] rpicam-vid failed ({e}), falling back to OpenCV VideoCapture...")
                self.use_pi = False
                self.cap = cv2.VideoCapture(CAMERA_INDEX)
        else:
            self.cap = cv2.VideoCapture(CAMERA_INDEX)
            self.cap.set(cv2.CAP_PROP_FRAME_WIDTH, width)
            self.cap.set(cv2.CAP_PROP_FRAME_HEIGHT, height)
            self.cap.set(cv2.CAP_PROP_FPS, fps)
            print(f"  [CAMERA] Initialized USB/Standard WebCam (index {CAMERA_INDEX}) ✅")

    def read(self):
        if self.use_pi and self.proc:
            try:
                while True:
                    self.buf += self.proc.stdout.read(4096)
                    s = self.buf.find(b'\xff\xd8')
                    e = self.buf.find(b'\xff\xd9', s)
                    if s != -1 and e != -1:
                        jpg  = self.buf[s:e+2]
                        self.buf = self.buf[e+2:]
                        frame = cv2.imdecode(np.frombuffer(jpg, np.uint8), cv2.IMREAD_COLOR)
                        if frame is not None:
                            return True, frame
            except Exception:
                return False, None
        elif self.cap:
            return self.cap.read()
        return False, None

    def release(self):
        if self.proc:
            self.proc.terminate()
        if self.cap:
            self.cap.release()

# ── Drawing & Visualization ─────────────────────────────────────
JOINT_COLOR    = (255, 80, 200)
LINE_COLOR     = (0, 220, 60)
JOINT_RADIUS   = 6
LINE_THICKNESS = 2

POSE_CONNECTIONS = [
    (11,12),(11,23),(12,24),(23,24),
    (11,13),(13,15),(12,14),(14,16),
    (23,25),(25,27),(27,29),(27,31),
    (24,26),(26,28),(28,30),(28,32),
]

def draw_skeleton(frame, landmarks, h, w, lc, jc):
    for (a, b) in POSE_CONNECTIONS:
        pa = landmarks[a]; pb = landmarks[b]
        if pa.visibility > 0.4 and pb.visibility > 0.4:
            cv2.line(frame,
                (int(pa.x*w), int(pa.y*h)),
                (int(pb.x*w), int(pb.y*h)),
                lc, LINE_THICKNESS, cv2.LINE_AA)
    for lm in landmarks:
        if lm.visibility > 0.4:
            cx, cy = int(lm.x*w), int(lm.y*h)
            cv2.circle(frame, (cx,cy), JOINT_RADIUS,   jc, -1, cv2.LINE_AA)
            cv2.circle(frame, (cx,cy), JOINT_RADIUS+2, (255,255,255), 1, cv2.LINE_AA)

def body_angle(p1, p2):
    return math.degrees(math.atan2(abs(p2.x-p1.x), abs(p2.y-p1.y)+1e-6))

# ── Alert Dispatcher to Cloud API ───────────────────────────────
def send_alert(action, label, call_type='AI_FALL'):
    headers = {
        'X-Station-API-Key': STATION_API_KEY,
    }
    payload = {
        'room_number':   LOCATION_NAME,
        'bed_number':    BED_NUMBER,
        'location_type': LOCATION_TYPE,
        'action':        action,
        'call_type':     call_type,
        'notes':         f"AI Vision Alert: {label} in {LOCATION_NAME}"
    }
    try:
        r = requests.post(SERVER_URL, data=payload, headers=headers, timeout=4)
        print(f"[{time.strftime('%H:%M:%S')}] 🚨 CLOUD ALERT SENT [{label} @ {LOCATION_NAME}] → HTTP {r.status_code}")
    except Exception as e:
        print(f"[{time.strftime('%H:%M:%S')}] ❌ Server unreachable: {e}")

# ── Main Vision Loop ────────────────────────────────────────────
def main():
    if cv2 is None:
        print("❌ Error: 'opencv-python' is not installed on this system.")
        print("👉 Please install it: pip install opencv-python")
        sys.exit(1)

    try:
        import mediapipe as mp
    except ImportError:
        print("❌ Error: 'mediapipe' is not installed on this system.")
        print("👉 Please install it: pip install mediapipe")
        sys.exit(1)

    mp_pose = mp.solutions.pose
    pose = mp_pose.Pose(
        static_image_mode=False,
        model_complexity=0,
        min_detection_confidence=0.6,
        min_tracking_confidence=0.6
    )

    last_fall_alert    = 0
    last_tremor_alert  = 0
    fall_start_time    = None
    last_seen_fallen   = 0
    tremor_frame_count = 0
    wrist_history      = collections.deque(maxlen=30)
    status_label       = "Initializing..."
    status_color       = (200, 200, 200)
    line_color         = LINE_COLOR
    joint_color        = JOINT_COLOR

    print("=" * 60)
    print(f"  MedPulse Edge AI Vision Monitor | {LOCATION_NAME} ({LOCATION_TYPE})")
    print(f"  Server: {SERVER_URL}")
    print("=" * 60)

    cam = VideoCamera(640, 480, 15, use_pi=USE_PI_CAMERA)
    time.sleep(1.5)
    print("  Camera stream active ✅ Monitoring patient...\n")

    try:
        while True:
            ret, frame = cam.read()
            if not ret or frame is None:
                time.sleep(0.05)
                continue

            now  = time.time()
            h, w = frame.shape[:2]
            res  = pose.process(cv2.resize(
                       cv2.cvtColor(frame, cv2.COLOR_BGR2RGB), (320, 240)))

            is_fallen = is_tremor = False

            if res.pose_landmarks:
                lm = res.pose_landmarks.landmark
                PL = mp_pose.PoseLandmark
                ls = lm[PL.LEFT_SHOULDER];  rs = lm[PL.RIGHT_SHOULDER]
                lh = lm[PL.LEFT_HIP];       rh = lm[PL.RIGHT_HIP]
                lw = lm[PL.LEFT_WRIST];     rw = lm[PL.RIGHT_WRIST]

                # ── Fall Detection Logic ────────────────────────
                mid_sh = type('P',(),{'x':(ls.x+rs.x)/2,'y':(ls.y+rs.y)/2})()
                mid_hp = type('P',(),{'x':(lh.x+rh.x)/2,'y':(lh.y+rh.y)/2})()
                angle  = body_angle(mid_sh, mid_hp)
                is_fallen = (angle > FALL_ANGLE_THRESHOLD and mid_hp.y > FALL_HIP_Y_THRESHOLD)

                if is_fallen:
                    last_seen_fallen = now
                    if fall_start_time is None:
                        fall_start_time = now
                        print(f"[{time.strftime('%H:%M:%S')}] ⚠️  Possible fall ({angle:.1f}°) — verifying...")
                    elif (now - fall_start_time) >= FALL_CONFIRM_SECS:
                        if (now - last_fall_alert) > COOLDOWN_SECONDS:
                            print(f"[{time.strftime('%H:%M:%S')}] 🚨 FALL CONFIRMED ({angle:.1f}°) ➔ DISPATCHING EMERGENCY CALL")
                            send_alert('emergency', 'FALL DETECTED', call_type='AI_FALL')
                            last_fall_alert = now
                else:
                    if angle < 45 and fall_start_time is not None:
                        print(f"[{time.strftime('%H:%M:%S')}] ✔️  Person stood up — fall state reset.")
                        fall_start_time = None

                # ── Tremor / Seizure Detection Logic ────────────
                wrist_history.append((lw.x, lw.y, rw.x, rw.y))
                if len(wrist_history) == wrist_history.maxlen:
                    diffs = [max(
                        math.hypot(wrist_history[i][0]-wrist_history[i-1][0],
                                   wrist_history[i][1]-wrist_history[i-1][1]),
                        math.hypot(wrist_history[i][2]-wrist_history[i-1][2],
                                   wrist_history[i][3]-wrist_history[i-1][3]))
                        for i in range(1, len(wrist_history))]
                    avg_mv = sum(diffs) / len(diffs)
                    if avg_mv > TREMOR_THRESHOLD and not is_fallen:
                        tremor_frame_count += 1
                        if tremor_frame_count >= TREMOR_CONFIRM_FRAMES:
                            is_tremor = True
                            if (now - last_tremor_alert) > COOLDOWN_SECONDS:
                                print(f"[{time.strftime('%H:%M:%S')}] ⚠️  TREMOR/SEIZURE CONFIRMED (mv={avg_mv:.4f})")
                                send_alert('urgent', 'TREMOR DETECTED', call_type='AI_TREMOR')
                                last_tremor_alert  = now
                                tremor_frame_count = 0
                    else:
                        tremor_frame_count = 0

                # ── UI State & Colors ───────────────────────────
                if is_fallen:
                    status_label = "!! 🚨 FALL DETECTED !!"
                    status_color = (0,0,255); line_color=(0,0,255); joint_color=(0,0,200)
                elif is_tremor:
                    status_label = "!! ⚠️ TREMOR DETECTED !!"
                    status_color = (0,140,255); line_color=(0,140,255); joint_color=(0,100,200)
                else:
                    status_label = "Room Status: Occupied (Normal)"
                    status_color = (0,220,60); line_color=LINE_COLOR; joint_color=JOINT_COLOR

                if SHOW_PREVIEW:
                    draw_skeleton(frame, lm, h, w, line_color, joint_color)
            else:
                status_label = "Room Status: Empty"
                status_color = (140,140,140)
                tremor_frame_count = 0
                if fall_start_time and (now - last_seen_fallen) > 3.0:
                    fall_start_time = None

            if SHOW_PREVIEW:
                cv2.putText(frame, status_label, (12,38), cv2.FONT_HERSHEY_SIMPLEX, 0.9, (0,0,0), 4, cv2.LINE_AA)
                cv2.putText(frame, status_label, (10,36), cv2.FONT_HERSHEY_SIMPLEX, 0.9, status_color, 2, cv2.LINE_AA)
                cv2.putText(frame, f"{LOCATION_NAME} ({LOCATION_TYPE}) | {time.strftime('%A %H:%M:%S')}",
                            (10, h-10), cv2.FONT_HERSHEY_SIMPLEX, 0.50, (0,220,60), 1, cv2.LINE_AA)
                cv2.imshow(f"MedPulse AI Vision | {LOCATION_NAME}", frame)
                if cv2.waitKey(1) & 0xFF == ord('q'):
                    break
            else:
                time.sleep(0.03)

    except KeyboardInterrupt:
        print("\n[INFO] AI Vision Monitor stopped.")
    finally:
        cam.release()
        cv2.destroyAllWindows()
        print("[INFO] Cleanup complete.")

if __name__ == '__main__':
    main()
