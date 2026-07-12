#!/bin/bash
# install.sh - Legacy manual installer for chorechill-ctl.
#
# Prefer using the Debian package instead:
#   make deb && sudo dpkg -i chorechill-ctl_*.deb
#
# This script is kept for development purposes and non-Debian systems.
# Run with: sudo bash install.sh

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"
cd "$SCRIPT_DIR"

# ==========================================
# 1. MOUNT DEBUGFS
# ==========================================
echo "[INFO] Mounting debugfs..."
mount -t debugfs none /sys/kernel/debug || true

# ==========================================
# 2. CONFIGURE EC_SYS MODULE
# ==========================================
echo "[INFO] Enabling write support for ec_sys module..."
echo "options ec_sys write_support=1" | tee /etc/modprobe.d/ec_sys.conf

echo "[INFO] Forcing ec_sys to load at each boot..."
echo "ec_sys" | tee /etc/modules-load.d/ec_sys.conf

echo "[INFO] Loading ec_sys module..."
modprobe -r ec_sys || true
modprobe ec_sys write_support=1

if lsmod | grep -q ec_sys; then
    echo "[INFO] ec_sys module loaded."
else
    echo "[ERROR] Failed to load ec_sys. Check your system configuration."
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
echo "[INFO] Installing libcjson..."
apt-get install -y libcjson-dev

# ==========================================
# 5. BACKUP CURRENT EC STATE
# ==========================================
EC_FILE="/sys/kernel/debug/ec/ec0/io"
BACKUP_FILE="/etc/chorechill-ctl/ec_defaults.hex"

echo "[INFO] Saving EC defaults to ${BACKUP_FILE}..."
mkdir -p "$(dirname "$BACKUP_FILE")"
hexdump -C "$EC_FILE" > "$BACKUP_FILE"
echo "[INFO] EC defaults saved."

# ==========================================
# 6. DETECT MOTHERBOARD AND SELECT PLUGIN
# ==========================================
PLUGIN_DIR="/usr/lib/chorechill-ctl/plugins"
PLUGIN_CONF="/etc/chorechill-ctl/active_plugin"
mkdir -p "$PLUGIN_DIR"

echo "[INFO] Detecting hardware..."
make detect

DETECTED_PLUGIN=$(backend/detect/chorechill-detect 2>/dev/null)
if [ -z "$DETECTED_PLUGIN" ]; then
    echo "[WARN] Detection failed, defaulting to plugin_msi_modern.so"
    DETECTED_PLUGIN="plugin_msi_modern.so"
fi
echo "$DETECTED_PLUGIN" > "$PLUGIN_CONF"
echo "[INFO] Active plugin set to: $DETECTED_PLUGIN"

# ==========================================
# 7. BUILD DAEMON AND PLUGINS
# ==========================================
echo "[INFO] Compiling daemon..."
make clean
make all

echo "[INFO] Compiling plugins..."
make plugins

echo "[INFO] Installing plugin to ${PLUGIN_DIR}..."
install -m 644 backend/plugins/msi_modern/plugin_msi_modern.so "$PLUGIN_DIR/"

# ==========================================
# 8. INSTALL SYSTEMD SERVICE
# ==========================================
SERVICE_FILE="/etc/systemd/system/chorechill-ctl.service"
DAEMON_PATH="/usr/sbin/chorechill-ctl"
ACTIVE_PLUGIN="${PLUGIN_DIR}/${DETECTED_PLUGIN}"

echo "[INFO] Installing daemon binary..."
install -m 755 backend/compiled/chorechill-ctl "$DAEMON_PATH"

echo "[INFO] Installing systemd service..."
cat > "$SERVICE_FILE" <<EOF
[Unit]
Description=Chore Chill CTL - EC fan control daemon
After=multi-user.target

[Service]
Type=simple
ExecStart=${DAEMON_PATH} ${ACTIVE_PLUGIN}
Restart=on-failure
RestartSec=5

[Install]
WantedBy=multi-user.target
EOF

systemctl daemon-reload
systemctl enable chorechill-ctl.service
systemctl start chorechill-ctl.service
echo "[INFO] Daemon started."

# ==========================================
# 9. INSTALL FRONTEND AND CLI WRAPPER
# ==========================================
SHARE_DIR="/usr/share/chorechill-ctl"
mkdir -p "${SHARE_DIR}/frontend"
cp -r frontend/src/. "${SHARE_DIR}/frontend/"

install -m 755 /dev/stdin /usr/local/bin/chorechill <<'EOF'
#!/bin/bash
python3 /usr/share/chorechill-ctl/frontend/main.py "$@"
EOF

echo ""
echo "[OK] Installation complete."
echo "     Active plugin : $(cat /etc/chorechill-ctl/active_plugin)"
echo "     Launch GUI    : chorechill"
echo "     Daemon status : sudo systemctl status chorechill-ctl"
echo ""
