# Backend Development & Packaging Roadmap

This document tracks the transition of `chorechill-ctl` from a single-machine script into a modular Linux utility distributed via a Debian package.

---

## Done

- [x] Graphical fan curve editor (7 draggable points, custom profile).
- [x] Define `driver_plugin_t` API (`backend/include/driver_plugin.h`).
- [x] Implement `plugin_loader.c` — runtime plugin loading via `dlopen`/`dlsym`.
- [x] Port MSI EC code into first plugin: `backend/plugins/msi_modern/plugin_msi_modern.c`.
- [x] CPU temperature reading via `/sys/class/thermal/` (x86_pkg_temp > acpitz > EC fallback).
- [x] DMI-based hardware auto-detection utility: `backend/detect/chorechill-detect.c`.
- [x] Refactor `main.c`, `ipc.c`, `profiles.c` to route all hardware calls through the driver pointer.
- [x] Full `debian/` packaging structure (control, changelog, rules, postinst, prerm, postrm).
- [x] Makefile targets: `all`, `plugins`, `detect`, `deb`, `clean`.

---

## Phase 1: Stabilise the MSI Modern Plugin

- [ ] Validate RPM formula (`478000 / raw`) against real measurements on MS-16R4.
- [ ] Confirm EC register map on MS-16R5 (GF65 Thin) and add to supported hardware list.
- [ ] Handle the case where the EC file exists but returns zeros (e.g. debugfs not fully mounted).
- [ ] Add unit for the raw fan timer value to the IPC response so the GUI can choose how to display RPM.

## Phase 2: First Real `.deb` Build

- [ ] Test `make deb` end-to-end on a clean Debian/Ubuntu VM.
- [ ] Verify `postinst` flow: ec_sys config, `chorechill-detect`, systemd service activation.
- [ ] Verify `prerm`/`postrm` clean removal.
- [ ] Sign the package with a GPG key and set up a simple apt repository (GitHub Releases or self-hosted).

## Phase 3: Additional Plugins

- [ ] `plugin_msi_legacy.so`: older MSI EC layouts (pre-2019 GF series).
- [ ] `plugin_asus_wmi.so`: read/write via `/sys/devices/platform/asus-nb-wmi/` ACPI interface.
- [ ] `plugin_lenovo_acpi.so`: use `thinkpad_acpi` kernel interface.
- [ ] Extend `chorechill-detect` lookup table as new plugins are added.

## Phase 4: Safety & Calibration

- [ ] Pre-flight check: detect kernel lockdown (`/sys/kernel/security/lockdown`) and warn the user before attempting EC writes.
- [ ] On first plugin install, snapshot existing EC registers to `/var/lib/chorechill/ec_defaults.hex` before any writes.
- [ ] Expose a `--dry-run` flag on the daemon that reads telemetry but never writes to the EC.
