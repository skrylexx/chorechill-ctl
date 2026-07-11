# Chore Chill CTL

> A handmade fan control daemon for Linux, built to save an old MSI GF63 Thin from dying.

![Chore Chill Preview](readme_img/main_page.png)

Communicates directly with the **Embedded Controller (EC)** via `/sys/kernel/debug/ec/ec0/io` to read CPU temperature and control fan speed at a low level.

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
| `GET` | frontend → daemon | Poll current telemetry |
| `SET:<pct>` | frontend → daemon | Lock fan at `<pct>`% — daemon keeps re-writing every 100 ms to hold the EC |
| `SET_CURVE:<t1,...,t6>;<s1,...,s7>` | frontend → daemon | Push a fan curve into EC registers; EC runs it autonomously, re-write loop stops |

All commands return `<temp_c>,<fan_pct>,<rpm>` as the response.

> **Why the re-write loop?** The EC has its own thermal algorithm. Writing once to the fan speed register (`0x71`) only works for a few seconds — the EC overrides it. The daemon re-writes every 100 ms to hold manual mode. `SET_CURVE` writes to the curve registers (`0x6A–0x78`) which the EC enforces autonomously, so no re-write is needed.

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

## Setup & Running

To install, compile, configure, and register the global service and client in one step, run the installer:

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

Bytes that spike under load → fan speed registers.  
Bytes that increase with heat → temperature registers.

> On this machine, fans were deliberately pushed by launching a Docker stack to cause visible peaks in the hexdump, making the fan register easy to identify.

### Validated addresses (MSI GF63 Thin)

| Component       | Address | Access     | Example values              |
|-----------------|---------|------------|------------------------------|
| CPU Temperature | `0x68`  | Read       | `0x36` (54°C) → `0x59` (89°C) |
| CPU Fan Speed   | `0x71`  | Read/Write | `0x2b` (43%) → `0x64` (100%)  |
| Fan Curve Temps | `0x6A–0x6F` | Write  | 6 temperature thresholds (°C) |
| Fan Curve Speeds| `0x72–0x78` | Write  | 7 speed percentages (%)       |
| GPU Temperature | `0x80`  | Read       | `0x00` (standby)            |
| GPU Fan Speed   | `0x89`  | Read/Write | `0x00` (off)                |

> **Note:** The EC has its own control loop — writing a curve to `0x6A–0x78` makes the EC enforce it automatically. For a manual lock (`SET:<pct>`), the daemon writes directly to `0x71`.

For full investigation steps, see [How2Get_Good_Addresses.md](./How2Get_Good_Addresses.md).

---

## Compatibility

> v0 targets the **MSI GF63 Thin** specifically (or machines with the same motherboard).  
> EC register layout varies by manufacturer and model.

---

## Roadmap

- [x] Create a graphical curve editor allowing users to drag 7 points to configure their custom profile.

---

## References

- [MSI WMI Platform — kernel docs](https://docs.kernel.org/wmi/devices/msi-wmi-platform.html)
- [`ec_sys` module — kernel docs](https://www.kernel.org/doc/html/latest/admin-guide/acpi/ec_access.html)