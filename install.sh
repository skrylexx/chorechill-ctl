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
# 5b. BUILD THE DAEMON
# ==========================================
echo "[INFO] Compiling C daemon..."
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"
cd "$SCRIPT_DIR"
make clean
make

# ==========================================
# 6. INSTALL SYSTEMD SERVICE
# ==========================================
SERVICE_FILE="/etc/systemd/system/chorechill-ctl.service"
DAEMON_PATH="${SCRIPT_DIR}/backend/compiled/chorechill-ctl"

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
systemctl start chorechill-ctl.service
echo "[INFO] Systemd daemon started."

# ==========================================
# 7. INSTALL CLIENT CLI WRAPPER
# ==========================================
CLI_WRAPPER="/usr/local/bin/chorechill"
FRONTEND_PATH="${SCRIPT_DIR}/frontend/src/main.py"
echo "[INFO] Installing CLI wrapper to ${CLI_WRAPPER}..."

cat > "$CLI_WRAPPER" << EOF
#!/bin/bash
# Wrapper to launch chore chill frontend from anywhere
python3 ${FRONTEND_PATH} "\$@"
EOF

chmod +x "$CLI_WRAPPER"
echo "[INFO] CLI wrapper installed."

echo ""
echo "[OK] Installation complete."
echo "     The C daemon has been compiled and started automatically."
echo "     Launch the GUI from any terminal by running:  chorechill"
echo ""
echo "     Systemd Status:              sudo systemctl status chorechill-ctl"
echo "     Systemd Restart:             sudo systemctl restart chorechill-ctl"
