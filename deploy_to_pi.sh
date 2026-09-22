#!/bin/bash
# ============================================================
# MedPulse Smart-Care AI — Raspberry Pi Deployment Script
# አጠቃቀም: bash deploy_to_pi.sh
# ============================================================

set -e  # ስህተት ሲፈጠር ወዲያውኑ አቁም

PROJ_DIR="/home/pi/ai_nurse_system"
VENV_DIR="$PROJ_DIR/venv"
SERVICE_DIR="/etc/systemd/system"

echo "=============================================="
echo "  MedPulse Smart-Care AI — Deployment Start"
echo "=============================================="

# --- 1. System Updates & Dependencies ---
echo ""
echo "[1/7] Installing system packages..."
sudo apt-get update -y
sudo apt-get install -y python3 python3-pip python3-venv postgresql postgresql-contrib libpq-dev

# --- 2. Virtual Environment ---
echo ""
echo "[2/7] Setting up Python virtual environment..."
cd $PROJ_DIR
if [ ! -d "$VENV_DIR" ]; then
    python3 -m venv venv
    echo "    ✓ Virtual environment created"
else
    echo "    ✓ Virtual environment already exists"
fi

# --- 3. Install Python Dependencies ---
echo ""
echo "[3/7] Installing Python dependencies..."
$VENV_DIR/bin/pip install --upgrade pip
$VENV_DIR/bin/pip install -r requirements.txt
echo "    ✓ All packages installed"

# --- 4. Create logs directory ---
echo ""
echo "[4/7] Setting up directories..."
mkdir -p $PROJ_DIR/logs
mkdir -p $PROJ_DIR/staticfiles
echo "    ✓ Directories created"

# --- 5. Database Setup (PostgreSQL) ---
echo ""
echo "[5/7] Setting up PostgreSQL database..."
source $PROJ_DIR/.env 2>/dev/null || true

if [ ! -z "$DB_NAME" ] && [ ! -z "$DB_USER" ] && [ "$DB_ENGINE" = "django.db.backends.postgresql" ]; then
    sudo -u postgres psql -c "CREATE DATABASE $DB_NAME;" 2>/dev/null || echo "    (Database already exists)"
    sudo -u postgres psql -c "CREATE USER $DB_USER WITH PASSWORD '$DB_PASSWORD';" 2>/dev/null || echo "    (User already exists)"
    sudo -u postgres psql -c "GRANT ALL PRIVILEGES ON DATABASE $DB_NAME TO $DB_USER;" 2>/dev/null || true
    echo "    ✓ PostgreSQL configured"
else
    echo "    ℹ Using SQLite (default)"
fi

# --- 6. Django Setup ---
echo ""
echo "[6/7] Running Django setup..."
$VENV_DIR/bin/python manage.py migrate --noinput
$VENV_DIR/bin/python manage.py collectstatic --noinput
echo "    ✓ Migrations applied & static files collected"

# --- 7. Install & Start Services ---
echo ""
echo "[7/7] Installing systemd services..."

# Serial port permission
sudo usermod -a -G dialout pi
echo "    ✓ Serial port permission granted"

# Copy service files
sudo cp $PROJ_DIR/systemd/medpulse.service $SERVICE_DIR/
sudo cp $PROJ_DIR/systemd/serial_bridge.service $SERVICE_DIR/

# Enable & Start services
sudo systemctl daemon-reload
sudo systemctl enable medpulse
sudo systemctl enable serial_bridge
sudo systemctl restart medpulse
sudo systemctl restart serial_bridge
echo "    ✓ Services installed and started"

echo ""
echo "=============================================="
echo "  ✅ Deployment Complete!"
echo "  🌐 System running at: http://$(hostname -I | awk '{print $1}'):8000"
echo "  📋 Check status: sudo systemctl status medpulse"
echo "=============================================="
