# v0.1 install.sh, fill it step by step

mount -t debugfs none /sys/kernel/debug
# 1. enable write for ec_sys module
echo "[INFO] Enabling write support for ec_sys module..."
echo "options ec_sys write_support=1" | sudo tee /etc/modprobe.d/ec_sys.conf

# 2. force module to load at each boot
echo "[INFO] Forcing module loading at each boot..."
echo "ec_sys" | sudo tee /etc/modules-load.d/ec_sys.conf

# 3. start the module immediately
echo "[INFO] Loading ec_sys module immediately..."
sudo modprobe -r ec_sys || true
sudo modprobe ec_sys write_support=1

# 4. check if the module is loaded
echo "[INFO] Checking if ec_sys module is loaded..."
if lsmod | grep -q ec_sys; then
    echo "[INFO] ec_sys module is loaded successfully."
else
    echo "[ERROR] Failed to load ec_sys module. Please check your system configuration."

# py 
echo "[INFO] Installing Python dependencies..."
sudo apt-get install python3-tk 
