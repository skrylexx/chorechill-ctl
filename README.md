# Chore Chill CTL

> A handmade fan control daemon for Linux, built to save an old MSI GF63 Thin from dying.

![Chore Chill Preview](readme_img/main_page.png)

Communicates directly with the **Embedded Controller (EC)** via `/sys/kernel/debug/ec/ec0/io` to read CPU temperature and control fan speed at a low level.

> [!NOTE]
> **TL;DR**
> - **What:** Low-level fan speed control daemon for Linux.
> - **How:** Modular C background daemon (`chorechill-ctl`) loading hardware-specific driver plugins; Python GUI (`chorechill`) controls it via UNIX socket.
> - **Prerequisites:** Secure Boot **disabled** (kernel lockdown off) + `ec_sys` module loaded.
> - **Setup:** Install the pre-built `.deb` package (or run the legacy `sudo bash install.sh` during transition).

---

## Table of Contents

- [Architecture](#architecture)
- [IPC Protocol](#ipc-protocol)
- [Prerequisites](#prerequisites)
- [Setup & Running](#setup--running)
- [How EC Addresses Were Found](#how-ec-addresses-were-found)
- [Compatibility](#compatibility)
- [Roadmap](#roadmap)
- [References](#references)

---

## Architecture

```
chorechill-ctl/
├── install.sh                      # Deployment, compilation & wrapper setup (run as root)
├── Makefile                        # C build automation
├── README.md                       # You are here
├── How2Get_Good_Addresses.md       # EC investigation guide (hexdump method)
├── config/
│   ├── profiles.json               # Fan curve profiles (default, silent, gaming, custom)
│   └── chorechill-ctl.service      # Systemd unit file (reference copy)
├── backend/                        # C daemon (runs as root, talks to EC)
│   ├── include/
│   │   ├── hardware.h              # Signatures for EC I/O (/sys/kernel/debug/ec/ec0/io)
│   │   ├── ipc.h                   # Signatures for UNIX socket IPC
│   │   └── profiles.h              # Signatures for profile parsing
│   ├── compiled/                   # Output directory for built binaries
│   └── src/
│       ├── main.c                  # Entrypoint : main loop + SIGINT/SIGTERM handler
│       ├── hardware.c              # Reads CPU temp / writes fan speed via EC
│       ├── ipc.c                   # UNIX socket server, routes commands to hardware/profiles
│       └── profiles.c              # Parses SET_CURVE payload and writes curve to EC
└── frontend/                       # Python GUI (runs as regular user)
    ├── requirements.txt            # Python dependencies
    └── src/
        ├── main.py                 # Entrypoint : wires IPC + UI + telemetry polling
        ├── gui/
        │   └── frontend.py         # Main window + CustomCurveEditor popup (customtkinter)
        └── ipc/
            └── main.py             # UNIX socket client : talks to C daemon
```

---

## IPC Protocol

The frontend and daemon communicate over a UNIX socket at `/tmp/chorechill-ctl.sock`.

| Command | Direction | Description |
|---|---|---|
| `GET` | frontend --> daemon | Poll current telemetry |
| `SET:<pct>` | frontend --> daemon | Lock fan at `<pct>`%: daemon keeps re-writing every 100 ms to hold the EC |
| `SET_CURVE:<t1,...,t6>;<s1,...,s7>` | frontend --> daemon | Push a fan curve into EC registers; EC runs it autonomously, re-write loop stops |

All commands return `<temp_c>,<fan_pct>,<rpm>` as the response.

> **Why the re-write loop?** The EC has its own thermal algorithm. Writing once to the fan speed register (`0x71`) only works for a few seconds because the EC overrides it. The daemon re-writes every 100 ms to hold manual mode. `SET_CURVE` writes to the curve registers (`0x6A–0x78`) which the EC enforces autonomously, so no re-write is needed.

---

## Prerequisites

- Linux with `ec_sys` kernel module available
- **Secure Boot must be disabled** (otherwise kernel lockdown blocks EC access)
- Python 3 + `python3-tk`
- GCC

Verify Secure Boot / lockdown status:
```bash
cat /sys/kernel/security/lockdown
# [none]  --> OK
# [integrity] or [confidentiality] --> disable Secure Boot in BIOS first
```

---

## Setup & Installation

We are moving away from raw `install.sh` scripts towards a secure, standard Debian package (`.deb`) distribution.

### Option A: Installing via Debian Package (Recommended)

To build and install the native Debian package (which sets up Python dependencies, systemd services, permissions, and performs hardware configuration/driver probing automatically):

```bash
# Build the Debian package
make deb

# Install it
sudo dpkg -i chorechill-ctl_*.deb
sudo apt-get install -f  # Fix any missing dependencies
```

### Option B: Legacy Installation (Development)

Run the old install script if building on a non-Debian system:

```bash
sudo bash install.sh
```

### Running the Client GUI

Once installed, simply start the graphical application from any terminal:

```bash
chorechill
```

You can select standard cooling profiles or visually design a tailored profile using the interactive **Custom Curve Editor** (adjusting 7 temperature thresholds and fan speeds dynamically):

![Custom Curve Editor](readme_img/custom_page.png)

### Managing the C Daemon (Systemd)

The background control daemon is registered as a systemd service. You can control it using standard systemd commands:

```bash
# Check service logs and status
sudo systemctl status chorechill-ctl

# Restart the service
sudo systemctl restart chorechill-ctl

# Stop the service
sudo systemctl stop chorechill-ctl
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

Bytes that spike under load --> fan speed registers.  
Bytes that increase with heat --> temperature registers.

> On this machine, fans were deliberately pushed by launching a Docker stack to cause visible peaks in the hexdump, making the fan register easy to identify.

### Validated addresses (MSI GF63 Thin)

| Component       | Address | Access     | Example values              |
|-----------------|---------|------------|------------------------------|
| CPU Temperature | `0x68`  | Read       | `0x36` (54°C) --> `0x59` (89°C) |
| CPU Fan Speed   | `0x71`  | Read/Write | `0x2b` (43%) --> `0x64` (100%)  |
| Fan Curve Temps | `0x6A–0x6F` | Write  | 6 temperature thresholds (°C) |
| Fan Curve Speeds| `0x72–0x78` | Write  | 7 speed percentages (%)       |
| GPU Temperature | `0x80`  | Read       | `0x00` (standby)            |
| GPU Fan Speed   | `0x89`  | Read/Write | `0x00` (off)                |

> **Note:** The EC has its own control loop: writing a curve to `0x6A–0x78` makes the EC enforce it automatically. For a manual lock (`SET:<pct>`), the daemon writes directly to `0x71`.

For full investigation steps, see [How2Get_Good_Addresses.md](./How2Get_Good_Addresses.md).

---

## Compatibility

- **Validated Hardware**: Optimized and tested on the **MSI GF63 Thin** (and other MSI laptops sharing the same motherboard architecture/EC layout).
- **Other MSI Laptops**: Laptops from different MSI series (e.g., Creator, Raider, Stealth, or other GF/GS models) might have compatible registers, but layouts should be verified first. Note that the RPM calculation formula (`478000 / raw_value`) is MSI-specific.
- **Other Laptop Brands (ASUS, Lenovo, HP, etc.)**: Not supported out of the box due to vendor-specific EC layouts. If you want to use this daemon on another brand:
  1. Follow the differential analysis guide in [How2Get_Good_Addresses.md](./How2Get_Good_Addresses.md) to locate your CPU temperature, fan speed control, and curve registers.
  2. Modify the static register address definitions in [backend/src/hardware.c](./backend/src/hardware.c).
- **System Prerequisites**:
  - Requires a Linux kernel with the `ec_sys` module loaded (`write_support=1`).
  - **Secure Boot must be disabled** in your BIOS/UEFI to prevent kernel lockdown from blocking write operations to `/sys/kernel/debug/ec/ec0/io`.

> [!NOTE]
> We are transitioning `chorechill-ctl` to a plugin-based architecture with DMI-based motherboard auto-detection. See the [ROADMAP.md](./backend/ROADMAP.md) for more details.

---

## Roadmap

- [x] Create a graphical curve editor allowing users to drag 7 points to configure their custom profile.
- [ ] Implement system DMI/motherboard configuration auto-detection.
- [ ] Refactor C hardware/EC interactions into modular dynamic load plugins (`.so` drivers).
- [ ] Migrate completely to `.deb` package distribution and deprecate `install.sh`.

---

## References

- [MSI WMI Platform - kernel docs](https://docs.kernel.org/wmi/devices/msi-wmi-platform.html)
- [`ec_sys` module - kernel docs](https://www.kernel.org/doc/html/latest/admin-guide/acpi/ec_access.html)