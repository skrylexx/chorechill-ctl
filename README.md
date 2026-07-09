# Chore Chill CTL

> A handmade fan control daemon for Linux, built to learn C and save an old MSI GF63 Thin from dying.

Communicates directly with the **Embedded Controller (EC)** via `/sys/kernel/debug/ec/ec0/io` to read CPU temperature and control fan speed at a low level.

---

## Architecture

```
chorechill-ctl/
├── install.sh                 # Deployment & module setup script
├── Makefile                   # C build automation
├── README.md                  # You are here
├── How2Get_Good_Addresses.md  # EC investigation guide (hexdump method)
├── backend/                   # C daemon
│   ├── include/
│   │   ├── hardware.h         # Signatures for EC I/O (/sys/kernel/debug/ec/ec0/io)
│   │   ├── controller.h       # Signatures for thermal logic
│   │   └── ipc.h              # Signatures for UNIX socket IPC
│   └── src/
│       ├── main.c             # Entrypoint (handles SIGINT)
│       ├── hardware.c         # Reads CPU temp / writes fan speed via EC
│       ├── controller.c       # Thermal curve logic
│       └── ipc.c              # UNIX socket server, talks to GUI
└── frontend/                  # Python GUI
    ├── requirements.txt       # Dependencies
    └── src/
        ├── main.py            # Entrypoint
        ├── gui/
        │   ├── app_window.py  # Main window
        │   └── graph.py       # Fan curve graph
        └── ipc_client.py      # UNIX socket client, talks to C daemon
```

---

## Prerequisites

- Linux with `ec_sys` kernel module available
- **Secure Boot must be disabled** (otherwise kernel lockdown blocks EC access)
- Python 3 + `python3-tk`
- GCC

Verify Secure Boot / lockdown status:
```bash
cat /sys/kernel/security/lockdown
# [none]  → OK
# [integrity] or [confidentiality] → disable Secure Boot in BIOS first
```

---

## Setup

Run the install script (sets up `ec_sys` with write support permanently and installs Python deps):

```bash
sudo bash install.sh
```

What it does:
1. Mounts `debugfs` (`/sys/kernel/debug`)
2. Configures `ec_sys` with `write_support=1` persistently via `/etc/modprobe.d/`
3. Forces `ec_sys` to load at every boot via `/etc/modules-load.d/`
4. Loads the module immediately
5. Installs `python3-tk`

---

## Build & Run

```bash
# Build the daemon
gcc backend/src/main.c backend/src/hardware.c backend/src/ipc.c -o compiled/v0

# Load the EC module with write support (required to write fan speed)
sudo modprobe -r ec_sys || true
sudo modprobe ec_sys write_support=1

# Mount debugfs if not already mounted
sudo mount -t debugfs none /sys/kernel/debug

# Run the daemon
sudo ./compiled/v0
```

---

## How EC Addresses Were Found

The Embedded Controller (EC) exposes 256 bytes of RAM at `/sys/kernel/debug/ec/ec0/io`.

### Method: hexdump differential analysis

```bash
# Take a baseline snapshot (idle, fans quiet)
sudo hexdump -C /sys/kernel/debug/ec/ec0/io > idle.txt

# Trigger load to spin up fans (e.g. launch a heavy Docker stack)
# Then snapshot again
sudo hexdump -C /sys/kernel/debug/ec/ec0/io > load.txt

# Compare
diff idle.txt load.txt
```

Bytes that spike under load → fan speed registers.  
Bytes that increase with heat → temperature registers.

> On this machine, fans were deliberately pushed by launching a Docker stack to cause visible peaks in the hexdump, making the fan register easy to identify.

### Validated addresses (MSI GF63 Thin)

| Component       | Address | Access     | Example values              |
|-----------------|---------|------------|-----------------------------|
| CPU Temperature | `0x68`  | Read       | `0x36` (54°C) → `0x59` (89°C) |
| CPU Fan Speed   | `0x71`  | Read/Write | `0x2b` (43%) → `0x64` (100%)  |
| GPU Temperature | `0x80`  | Read       | `0x00` (standby)            |
| GPU Fan Speed   | `0x89`  | Read/Write | `0x00` (off)                |

> **Note:** To force 100% fan speed, write `0x64` (hex) to `0x71`.  
> The EC has its own control loop — the daemon must keep writing the setpoint in a loop or the EC will reclaim control and apply its own thermal curve.

For full investigation steps, see [How2Get_Good_Addresses.md](./How2Get_Good_Addresses.md).

---

## Compatibility

> v0 targets the **MSI GF63 Thin** specifically (or machines with the same motherboard).  
> EC register layout varies by manufacturer and model.

---

## Roadmap

### phase 1: closing the loop (backend logic)
- [ ] **parse python commands:** update C backend to read "SET:XX", extract the value, and trigger `set_fan_speed(X)`.
- [ ] **create controller.c (the brain):** maintain the targeted fan speed at each loop iteration so the hardware EC doesn't override the custom value.
- [ ] **static profiles:** implement hardcoded modes in C (e.g., if mode == silent -> lock at 30%, if mode == gaming -> lock at 85%).

### phase 2: advanced dashboard (frontend)
- [ ] **custom fan curves:** build a GUI graph where the user can set custom speed % for specific temperature thresholds. send this data array to the C backend.
- [ ] **ui/ux overhaul:** upgrade from standard tkinter to something cleaner (like `customtkinter` for a native, modern look).

### phase 3: production (deployment)
- [ ] **install.sh script:** automate `modprobe ec_sys` and socket permissions on the host system.
- [ ] **systemd service:** set up a daemon to run the C backend automatically on boot in the background with `root` privileges.

---

## References

- [MSI WMI Platform — kernel docs](https://docs.kernel.org/wmi/devices/msi-wmi-platform.html)
- [`ec_sys` module — kernel docs](https://www.kernel.org/doc/html/latest/admin-guide/acpi/ec_access.html)