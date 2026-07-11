#!/bin/bash
# install.sh — sets up the host system for chorechill-ctl
# run with: sudo bash install.sh

set -e

# ==========================================
# 1. MOUNT DEBUGFS
# ==========================================
echo "[INFO] Mounting debugfs..."
mount -t debugfs none /sys/kernel/debug || true

# ==========================================
# 2. CONFIGURE EC_SYS MODULE
# ==========================================

# enable write support for the ec_sys module
echo "[INFO] Enabling write support for ec_sys module..."
echo "options ec_sys write_support=1" | tee /etc/modprobe.d/ec_sys.conf

# force ec_sys to load at each boot
echo "[INFO] Forcing module loading at each boot..."
echo "ec_sys" | tee /etc/modules-load.d/ec_sys.conf

# start the module immediately
echo "[INFO] Loading ec_sys module immediately..."
modprobe -r ec_sys || true
modprobe ec_sys write_support=1

# check if the module is loaded
echo "[INFO] Checking if ec_sys module is loaded..."
if lsmod | grep -q ec_sys; then
    echo "[INFO] ec_sys module is loaded successfully."
else
    echo "[ERROR] Failed to load ec_sys module. Please check your system configuration."
    exit 1
fi

# ==========================================
# 3. INSTALL PYTHON DEPENDENCIES
# ==========================================
echo "[INFO] Installing Python dependencies..."
apt-get install -y python3-tk
pip3 install customtkinter --break-system-packages

# ==========================================
# 4. INSTALL C DEPENDENCIES
# ==========================================
# cJSON is used in backend/src/ipc.c to serialize telemetry data to JSON
echo "[INFO] Installing cJSON (C JSON library)..."
apt-get install -y libcjson-dev
# note: add the "-lcjson" flag when compiling (already handled by the Makefile)

# ==========================================
# 5. CAPTURE DEFAULT EC VALUES (BACKUP)
# ==========================================
EC_FILE="/sys/kernel/debug/ec/ec0/io"
BACKUP_FILE="/etc/chorechill-ctl/ec_defaults.hex"

echo "[INFO] Saving EC default values to ${BACKUP_FILE}..."
mkdir -p "$(dirname "$BACKUP_FILE")"
hexdump -C "$EC_FILE" > "$BACKUP_FILE"
echo "[INFO] EC defaults saved."

# ==========================================
# 6. INSTALL SYSTEMD SERVICE
# ==========================================
SERVICE_FILE="/etc/systemd/system/chorechill-ctl.service"
DAEMON_PATH="$(realpath "$(dirname "$0")/backend/compiled/chorechill-ctl")"

echo "[INFO] Installing systemd service..."

cat > "$SERVICE_FILE" << EOF
[Unit]
Description=Chore Chill CTL — EC fan control daemon
After=multi-user.target

[Service]
Type=simple
ExecStart=${DAEMON_PATH}
Restart=on-failure
RestartSec=5

[Install]
WantedBy=multi-user.target
EOF

systemctl daemon-reload
systemctl enable chorechill-ctl.service

echo ""
echo "[OK] Installation complete."
echo "     Build the daemon with:       make"
echo "     Start it now with:           sudo systemctl start chorechill-ctl"
echo "     Check status with:           sudo systemctl status chorechill-ctl"
