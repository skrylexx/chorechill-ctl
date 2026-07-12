# Backend Development & Packaging Roadmap

This document outlines the evolutionary steps to transition `chorechill-ctl` from a single-machine script into a professional, modular Linux utility distributed via a Debian package (`.deb`).

---

## Phase 1: Motherboard & Configuration Detection
To make the tool plug-and-play across different hardware configurations, the system must automatically detect the host motherboard and platform details on startup or install.
- [ ] **Implementation Plan**:
  - Create a detection utility/library (e.g., `chorechill-detect` or integrated into the daemon) that reads system DMI tables.
  - Query files in `/sys/class/dmi/id/`:
    - `board_vendor` (e.g., `MSI`, `ASUSTeK COMPUTER INC.`, `LENOVO`)
    - `board_name` (motherboard model)
    - `product_name` (laptop model, e.g., `GF63 Thin 9SCXR`)
  - Expose a clean C/Python interface returning a normalized hardware signature.

## Phase 2: Plugin Architecture for EC & WMI Drivers
Instead of hardcoding register maps in a single `hardware.c`, hardware-specific logic will be split into modular plugins.
- [ ] **Implementation Plan**:
  - Define a unified **Driver Plugin API** (shared header `driver_plugin.h`) with signatures for:
    - `init_driver()`
    - `read_cpu_temp()`
    - `read_gpu_temp()`
    - `set_fan_speed(int percentage)`
    - `apply_fan_curve(int *temps, int *speeds)`
    - `cleanup_driver()`
  - Implement driver plugins as dynamic shared libraries (`.so`) loaded at runtime via `dlopen`/`dlsym` (or via JSON-based mapping declarations for standard configurations).
  - Develop initial plugins:
    - `plugin_msi_modern.so`: Uses raw EC writes for newer MSI series.
    - `plugin_msi_legacy.so`: Uses older MSI EC layouts.
    - `plugin_asus_wmi.so`: Interacts with `/sys/devices/platform/asus-nb-wmi/` or equivalent ACPI interfaces.
    - `plugin_lenovo_acpi.so`: Interacts with `thinkpad_acpi` interfaces.

## Phase 3: Debian Packaging (`.deb`)
Replace the raw `install.sh` with a native Debian package. A `.deb` package improves user trust, simplifies dependency management, handles upgrades gracefully, and integrates nicely with package managers (`apt`, `dpkg`).
- [ ] **Implementation Plan**:
  - Structure the project source with `debian/` metadata:
    - `debian/control`: Declare package dependencies (`libcjson1`, `python3`, `python3-tk`, `python3-customtkinter` or custom python modules).
    - `debian/rules`: Automated build system integration using `debhelper` (compiles daemon and builds UI assets).
    - `debian/postinst` (Post-installation script):
      - Enable/configure `ec_sys write_support=1` under `/etc/modprobe.d/` and `/etc/modules-load.d/`.
      - Check systemd, reload daemon, and start services.
      - Probe motherboard using the detection tool to symlink/configure the correct default plugin.
    - `debian/prerm` & `debian/postrm` (Removal scripts):
      - Cleanly stop systemd services and unload configurations.
  - Automate package building in the `Makefile` under a new target: `make deb`.

## Phase 4: Safety & Calibration
- [ ] **Implementation Plan**:
  - Run a pre-flight check in the plugin loader to verify if kernel lockdown is enabled (which blocks EC access). Provide a user-friendly system notification if Secure Boot needs to be disabled.
  - Automatically back up existing EC configurations to `/var/lib/chorechill/` on the first installation of a new plugin.

