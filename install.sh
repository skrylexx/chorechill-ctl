# v0.1 install.sh, fill it step by step

mount -t debugfs none /sys/kernel/debug
# 1. enable write for ec_sys module
echo "[INFO] Enabling write support for ec_sys module..."
echo "options ec_sys write_support=1" | sudo tee /etc/modprobe.d/ec_sys.conf

# 2. Force module loading at each boot
echo "[INFO] Forcing module loading at each boot..."
echo "ec_sys" | sudo tee /etc/modules-load.d/ec_sys.conf

# 3. Charger immédiatement pour éviter de devoir redémarrer pendant l'installation
echo "[INFO] Loading ec_sys module immediately..."
sudo modprobe -r ec_sys || true
sudo modprobe ec_sys write_support=1