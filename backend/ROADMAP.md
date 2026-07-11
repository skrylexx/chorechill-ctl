# Backend Development Roadmap

This document outlines the planned architectural improvements for the C daemon (`chorechill-ctl`) to enhance compatibility, flexibility, and automation.

---

## 1. Dynamic Address & Register Detection
Currently, the EC memory addresses for temperature sensors and fan registers (`0x68`, `0x71`, etc.) are statically defined in [`backend/include/hardware.h`](./include/hardware.h). 
- [ ] **Implementation Plan**:
  - Implement a heuristic search/probing sequence on startup to dynamically identify active sensor cells.
  - Parse local BIOS/WMI interfaces (`/sys/class/wmi/`) to match against known platform models.
  - Dynamically load register offsets from a runtime configuration template instead of static compilation flags.

## 2. Automated Initial Build Calibration
- [ ] **Implementation Plan**:
  - Enhance `install.sh` or the C entrypoint to run a calibration phase upon initial installation.
  - Capture factory-default EC registers values automatically, generating a machine-specific fallback profile (`/etc/chorechill-ctl/factory_defaults.json`) before making any writes.
  - Validate write permissions and `lockdown` module status programmatically during build compilation tests.

## 3. Cross-Brand & MSI Family Compatibility
To transition `chorechill-ctl` from a single-machine script to a general-purpose Linux utility:
- [ ] **Implementation Plan**:
  - Generalize C register mappings to support wider MSI motherboard revisions (e.g. Creator, Raider, and Stealth series).
  - Add modular abstraction drivers for other hardware manufacturers that expose similar EC structures under Linux (e.g., ASUS `asus-nb-wmi`, Lenovo `thinkpad_acpi`, HP `hp-wmi`).
  - Implement a unified configuration parser allowing users to select their laptop brand/model from the frontend to swap register maps dynamically in C.
