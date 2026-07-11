# How to Find EC Addresses for Fan Control on Linux

> This guide explains how to identify the Embedded Controller (EC) memory registers that control fan speed and CPU temperature on a laptop running Linux. No prior kernel knowledge required.

---

## Table of Contents

1. [Prerequisites — Kernel Lockdown](#1-prerequisites--kernel-lockdown)
2. [Enabling the `ec_sys` Module](#2-enabling-the-ec_sys-module)
3. [Taking Snapshots of EC Memory](#3-taking-snapshots-of-ec-memory)
4. [Generating a Real Load](#4-generating-a-real-load)
5. [Finding the Addresses by Diffing](#5-finding-the-addresses-by-diffing)
6. [Identifying Temperature vs. Fan Registers](#6-identifying-temperature-vs-fan-registers)
7. [Validated Addresses — MSI GF63 Thin](#7-validated-addresses--msi-gf63-thin)
8. [Reading and Writing from C](#8-reading-and-writing-from-c)

---

## 1. Prerequisites — Kernel Lockdown

If **Secure Boot** is enabled in the BIOS, Linux activates "Lockdown" mode. This blocks direct access to `/sys/kernel/debug/ec/ec0/io`, even for `root`.

### Check your lockdown status

```bash
cat /sys/kernel/security/lockdown
```

| Output | Meaning |
|---|---|
| `[none] integrity confidentiality` | [OK] Unlocked — you can proceed |
| `none [integrity] confidentiality` | [LOCKED] — disable Secure Boot |
| `none integrity [confidentiality]` | [LOCKED] — disable Secure Boot |

**If locked:** reboot, enter your BIOS/UEFI (usually `Del`, `F2`, or `F10` at POST), find the **Secure Boot** setting and set it to **Disabled**. Save and reboot.

---

## 2. Enabling the `ec_sys` Module

The EC is exposed as a binary file at `/sys/kernel/debug/ec/ec0/io` (256 bytes, one per EC register). To access it, you need to:
- Mount the debug filesystem
- Load the `ec_sys` module with write support enabled

### One-time setup (current session only)

```bash
# mount debugfs if not already mounted
sudo mount -t debugfs none /sys/kernel/debug

# remove any previously loaded ec_sys without write support
sudo modprobe -r ec_sys || true

# reload with write support
sudo modprobe ec_sys write_support=1

# verify the EC file is accessible
ls -lh /sys/kernel/debug/ec/ec0/io
# expected output: -rw-r--r-- ... /sys/kernel/debug/ec/ec0/io
```

### Persistent setup (survives reboots)

```bash
# make write_support permanent
echo "options ec_sys write_support=1" | sudo tee /etc/modprobe.d/ec_sys.conf

# load the module automatically at boot
echo "ec_sys" | sudo tee /etc/modules-load.d/ec_sys.conf
```

> This is already handled by `install.sh` if you ran it.

---

## 3. Taking Snapshots of EC Memory

The EC has exactly 256 bytes of RAM (addresses `0x00` → `0xFF`). The strategy is:
1. Snapshot the EC at **idle** (everything cool and quiet)
2. Snapshot again **under load** (CPU hot, fans spinning)
3. Compare — any byte that changed under load is a candidate register

### Snapshot command

```bash
# save a hexdump of all 256 EC bytes to a file
sudo hexdump -C /sys/kernel/debug/ec/ec0/io > snapshot.txt
```

### What the output looks like

```
00000060  00 00 00 00 00 00 00 00  36 00 00 00 00 00 00 00  |........6.......|
00000070  00 2b 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |.+..............|
```

Each line covers 16 bytes. The left column is the starting address of the line in hex. For example:
- Line `00000060` covers addresses `0x60` to `0x6F`
- Line `00000070` covers addresses `0x70` to `0x7F`

To find the value at a specific address, count columns from left. Address `0x68` is the 9th byte of the `0x60` line → value `0x36` (= 54°C in this example).

### Take your baseline snapshot

```bash
# let the machine sit idle for 2–3 minutes, fans at minimum
sudo hexdump -C /sys/kernel/debug/ec/ec0/io > idle.txt
cat idle.txt  # inspect it — note down values that look like temperatures (30–90 range)
```

---

## 4. Generating a Real Load

You need the CPU hot enough that the fans visibly spin up. A few reliable methods:

### Option A — `stress-ng` (recommended, available on most distros)

```bash
# install
sudo apt install stress-ng

# saturate all CPU cores for 60 seconds
stress-ng --cpu 0 --timeout 60s
```

### Option B — `prime95` via `mprime`

```bash
# run torture test
mprime -t
```

### Option C — Compilation (no extra tools needed)

```bash
# clone and build a large project, e.g. the Linux kernel itself
git clone --depth=1 https://github.com/torvalds/linux.git /tmp/linux-src
cd /tmp/linux-src && make defconfig && make -j$(nproc)
```

> **Tip:** Watch the fan speed in real time in a second terminal while the load runs:
> ```bash
> watch -n 0.5 "sudo hexdump -C /sys/kernel/debug/ec/ec0/io | head -10"
> ```

---

## 5. Finding the Addresses by Diffing

Once the fans are audibly spinning and the CPU is hot, take the load snapshot:

```bash
sudo hexdump -C /sys/kernel/debug/ec/ec0/io > load.txt
```

Then compare:

```bash
diff idle.txt load.txt
```

### How to read the diff output

```
< 00000060  00 00 00 00 00 00 00 00  36 00 00 00 00 00 00 00  |........6.......|
> 00000060  00 00 00 00 00 00 00 00  59 00 00 00 00 00 00 00  |........Y.......|
< 00000070  00 2b 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |.+..............|
> 00000070  00 5a 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |.Z..............|
```

- Lines starting with `<` are the **idle** values
- Lines starting with `>` are the **load** values
- Bytes that changed are your candidates

In this example:
- Address `0x68` changed from `0x36` → `0x59` (54°C → 89°C) — **temperature register**
- Address `0x71` changed from `0x2b` → `0x5a` (43% → 90%) — **fan speed register**

---

## 6. Identifying Temperature vs. Fan Registers

Once you have a list of changed bytes, you need to tell temperatures from fan speeds.

### Distinguishing rules

| Clue | Temperature register | Fan speed register |
|---|---|---|
| Value range at idle | 30–60 (°C) | 20–50 (%) |
| Value range under load | 70–100 | 60–100 |
| Changes gradually | Yes | Yes |
| Writable (fan responds) | No | Yes |
| Reacts to cooling (fan off) | Yes | — |

### Confirming a fan register by writing to it

```bash
# write 0x64 (= 100) to a candidate address, e.g. 0x71
# if the fan spins to 100%, it's confirmed
printf '\x64' | sudo dd of=/sys/kernel/debug/ec/ec0/io bs=1 seek=$((0x71)) conv=notrunc

# restore control by writing the idle value back
printf '\x2b' | sudo dd of=/sys/kernel/debug/ec/ec0/io bs=1 seek=$((0x71)) conv=notrunc
```

> **Warning:** Do not leave the fan at 0% (`\x00`) for long — the EC will likely reclaim control within seconds, but be careful.

### Confirming a temperature register

Watch the value decrease as the machine cools down after stopping the load. A temperature register will go from ~80 back to ~40 over a few minutes. A fan register tracks it proportionally.

### Finding the fan curve registers

MSI ECs typically store the fan curve as two arrays in consecutive registers:
- **6 temperature thresholds** (°C): the temperatures at which each speed step activates
- **7 fan speed percentages** (the speed for each segment between thresholds)

To find them, write a very distinctive curve (e.g., all temperatures at `0x50` = 80°C, all speeds at `0x1E` = 30%) and look for those patterns in the hexdump:

```bash
# take a snapshot after applying a known curve via the daemon
sudo hexdump -C /sys/kernel/debug/ec/ec0/io | grep -E "50 50|1e 1e"
```

---

## 7. Validated Addresses — MSI GF63 Thin

These addresses were confirmed by differential analysis and direct write tests.

### Sensor registers (read-only)

| Component | Address | Access | Example values |
|---|---|---|---|
| CPU Temperature | `0x68` | Read | `0x36` (54°C) → `0x59` (89°C) |
| GPU Temperature | `0x80` | Read | `0x00` (standby) |

### Fan control registers (read/write)

| Component | Address | Access | Example values |
|---|---|---|---|
| CPU Fan Speed (%) | `0x71` | Read/Write | `0x2b` (43%) → `0x64` (100%) |
| GPU Fan Speed (%) | `0x89` | Read/Write | `0x00` (off) |

### Fan curve registers (write to reprogram EC behavior)

| Register block | Addresses | Content |
|---|---|---|
| Temperature thresholds | `0x6A` → `0x6F` | 6 bytes — °C values (e.g. `55, 64, 73, 76, 82, 88`) |
| Fan speed steps | `0x72` → `0x78` | 7 bytes — % values (e.g. `38, 43, 48, 54, 60, 70, 85`) |
| Fan mode control | `0xF4` | 1 byte — EC control mode flag |

> **How the curve works:** The EC reads the temperature thresholds every cycle. When CPU temp crosses threshold T[i], it ramps the fan toward speed S[i+1]. There are 6 thresholds and 7 speeds — the first speed (S[0]) applies below T[0], and the last (S[6]) applies above T[5].

> **Note on direct writes:** Writing to `0x71` forces an immediate fan speed, but the EC overrides it within a few seconds. To hold a manual speed, the daemon must re-write every ~100 ms. Writing the curve to `0x6A–0x78` programs the EC itself, which then enforces the curve autonomously.

---

## 8. Reading and Writing from C

Interaction uses standard POSIX system calls — `/sys/kernel/debug/ec/ec0/io` is treated as a binary file.

```c
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>

#define EC_FILE "/sys/kernel/debug/ec/ec0/io"

// read one byte from an EC register
int read_ec_byte(off_t address) {
    int fd = open(EC_FILE, O_RDONLY);
    if (fd == -1) return -1;

    lseek(fd, address, SEEK_SET);   // move cursor to the target register

    uint8_t value;
    read(fd, &value, 1);            // read exactly 1 byte

    close(fd);
    return value;
}

// write one byte to an EC register
int write_ec_byte(off_t address, uint8_t value) {
    int fd = open(EC_FILE, O_RDWR);
    if (fd == -1) return -1;

    lseek(fd, address, SEEK_SET);
    write(fd, &value, 1);

    close(fd);
    return 0;
}

// usage
int temp = read_ec_byte(0x68);          // CPU temp in °C
write_ec_byte(0x71, 75);               // lock fan at 75%
```

> The full production implementation is in [`backend/src/hardware.c`](./backend/src/hardware.c).

---

## References

- [`ec_sys` kernel module documentation](https://www.kernel.org/doc/html/latest/admin-guide/acpi/ec_access.html)
- [MSI WMI Platform — kernel docs](https://docs.kernel.org/wmi/devices/msi-wmi-platform.html)
- [ACPI EC specification (ACPI 6.5 spec, §12)](https://uefi.org/sites/default/files/resources/ACPI_Spec_6_5_Aug29.pdf)