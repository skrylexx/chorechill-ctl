# 💻 Hardware Investigation and Control Guide (EC)

This document lists the necessary steps to unlock low-level hardware access under Linux and identify the EC (Embedded Controller) memory addresses allowing fan control and temperature reading, specifically on MSI architectures.

## 1. Kernel Lockdown Release

By default, if **Secure Boot** is enabled in the BIOS, Linux activates "Lockdown" mode. This prohibits direct access to physical memory, even for the `root` user.

### Check Lockdown status:
```bash
cat /sys/kernel/security/lockdown
```

* **[integrity]** or **[confidentiality]**: The system is locked. `hexdump` will be blocked.
* **[none]**: The system is unlocked. Physical access is allowed.

**Required Action:** If the system is locked, you must restart the PC, enter the BIOS/UEFI, and **disable Secure Boot**.

---

## 2. Enabling the `ec_sys` Module (Write Mode)

To communicate with the EC, the kernel must load the `ec_sys` module with write permissions, and the debug filesystem must be mounted.

### Temporary Enablement (For development)

```bash
# 1. Mount debugfs (if not already done by the distribution)
sudo mount -t debugfs none /sys/kernel/debug

# 2. Reload the module with write support
sudo modprobe -r ec_sys
sudo modprobe ec_sys write_support=1

# 3. Check that the EC binary file is accessible
ls -l /sys/kernel/debug/ec/ec0/io
```

### Persistent Configuration (To be integrated into `install.sh`)

For the C daemon to run at every boot, this configuration must be made permanent:

```bash
# Allow write access for the module
echo "options ec_sys write_support=1" | sudo tee /etc/modprobe.d/ec_sys.conf

# Force loading at boot
echo "ec_sys" | sudo tee /etc/modules-load.d/ec_sys.conf
```

---

## 3. Investigation: Finding Memory Addresses

The EC has 256 bytes of RAM (from `0x00` to `0xFF`). To find out which memory cell controls what, we must proceed by *diffing* (comparison): read the memory at idle, then read it under load, and look for values that have changed.

### The Hexdump Method

```bash
sudo hexdump -C /sys/kernel/debug/ec/ec0/io
```

**Procedure:**

1. Run the command when the PC is **cold and quiet**. Save the result.
2. Run a load test (e.g., compilation, stress test) to **heat up the CPU and speed up the fans**.
3. Rerun the command and compare the hexadecimal variations (convert from Hex to Decimal to confirm).

### 🎯 Validated Addresses (Current Machine)

According to the readings taken, here is the exact mapping of the machine:

| Component | Address (Hex) | Access Type | Value Range (Examples) |
| --- | --- | --- | --- |
| **CPU Temperature** | `0x68` | Read | `0x36` (54°C) -> `0x59` (89°C) |
| **CPU Fan Speed** | `0x71` | Read / Write | `0x2b` (43%) -> `0x64` (100%) |
| GPU Temperature | `0x80` | Read | `0x00` (Standby) |
| GPU Fan Speed | `0x89` | Read / Write | `0x00` (Off) |

*(Note: To write a speed of 100%, the hexadecimal value to send is `0x64`)*.

---

## 4. C Implementation (Basic Concept)

Interaction with these addresses is done via standard POSIX system calls.
The file `/sys/kernel/debug/ec/ec0/io` is manipulated like a standard binary file.

1. `open()`: Open with `O_RDONLY` (to read) or `O_RDWR` (to write) flags.
2. `lseek()`: Move the cursor to the target address (e.g., `0x71`).
3. `read()` / `write()`: Read or inject a byte (`uint8_t`).
4. `close()`: Free the file descriptor.

**Warning:** The EC has its own internal regulation loop. Forcing a static value at address `0x71` requires the C program to loop to maintain the setpoint; otherwise, the EC will take back control and apply its own thermal curve.