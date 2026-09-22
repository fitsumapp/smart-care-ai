#!/bin/bash
# ================================================================
#  MedPulse AI Vision — Raspberry Pi Automated Setup Script
#  Installs dependencies for Camera, OpenCV & MediaPipe
# ================================================================

echo "========================================================"
echo "  🏥 MedPulse Smart-Care AI — Raspberry Pi Camera Setup"
echo "========================================================"

# 1. Update package list
echo "[1/4] Updating system packages..."
sudo apt update && sudo apt upgrade -y

# 2. Install system camera & vision dependencies
echo "[2/4] Installing camera and system libraries..."
sudo apt install -y python3-pip python3-venv libcamera-tools \
    libatlas-base-dev libopenjp2-7 libtiff5 libgl1-mesa-glx libglib2.0-0

# 3. Create virtual environment
echo "[3/4] Creating Python virtual environment..."
python3 -m venv venv
source venv/bin/activate

# 4. Install Python libraries
echo "[4/4] Installing Python packages (OpenCV, MediaPipe, Requests)..."
pip install --upgrade pip
pip install -r requirements.txt

echo ""
echo "========================================================"
echo "  ✅ Setup Complete!"
echo "  To run: source venv/bin/activate && python patient_monitor.py"
echo "========================================================"
