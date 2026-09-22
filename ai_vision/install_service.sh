#!/bin/bash
# ================================================================
#  Install and Start MedPulse AI Vision as a Background Service
# ================================================================

echo "Installing MedPulse AI Vision Service..."

# Copy service file to systemd directory
sudo cp patient_monitor.service /etc/systemd/system/

# Reload systemd daemon
sudo systemctl daemon-reload

# Enable service to start automatically on boot
sudo systemctl enable patient_monitor.service

# Start the service immediately
sudo systemctl restart patient_monitor.service

echo ""
echo "========================================================"
echo "  ✅ MedPulse AI Vision Service is now RUNNING!"
echo "  Status check: sudo systemctl status patient_monitor"
echo "  View live logs: sudo journalctl -u patient_monitor -f"
echo "========================================================"
