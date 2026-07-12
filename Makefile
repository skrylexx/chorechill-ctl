##############################################################################
# Makefile — chorechill-ctl build system
#
# Targets:
#   make / make all   — build the main daemon binary  (backend/compiled/chorechill-ctl)
#   make plugins      — build all hardware driver plugins as .so files
#   make detect       — build the hardware detection utility (chorechill-detect)
#   make deb          — build a Debian .deb package via dpkg-buildpackage
#   make clean        — remove all compiled artefacts
##############################################################################

CC      = gcc

# Daemon CFLAGS: strict warnings, optimise, link with libdl for dlopen()
CFLAGS  = -Wall -Wextra -O2
LDFLAGS = -ldl

# Plugin CFLAGS: position-independent code required for shared libraries
PLUGIN_CFLAGS = -Wall -Wextra -O2 -fPIC -shared

# --- Directories ---
SRC_DIR     = backend/src
INCLUDE_DIR = backend/include
OUT_DIR     = backend/compiled
PLUGIN_DIR  = backend/plugins
DETECT_DIR  = backend/detect

# --- Daemon sources (no hardware.c — hardware is now provided by plugins) ---
DAEMON_SRCS = $(SRC_DIR)/main.c        \
              $(SRC_DIR)/ipc.c         \
              $(SRC_DIR)/profiles.c    \
              $(SRC_DIR)/plugin_loader.c

DAEMON_TARGET = $(OUT_DIR)/chorechill-ctl

# --- Plugin sources and targets ---
MSI_MODERN_SRC = $(PLUGIN_DIR)/msi_modern/plugin_msi_modern.c
MSI_MODERN_SO  = $(PLUGIN_DIR)/msi_modern/plugin_msi_modern.so

# --- Detection utility ---
DETECT_SRC    = $(DETECT_DIR)/chorechill-detect.c
DETECT_TARGET = $(DETECT_DIR)/chorechill-detect

##############################################################################
# Targets
##############################################################################

.PHONY: all plugins detect deb clean

# Default target: build the daemon
all: $(DAEMON_TARGET)

$(DAEMON_TARGET): $(DAEMON_SRCS)
	@mkdir -p $(OUT_DIR)
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -I$(SRC_DIR) \
	      $(DAEMON_SRCS) -o $(DAEMON_TARGET) $(LDFLAGS)
	@echo "[OK] Daemon built --> $(DAEMON_TARGET)"

# Build all hardware driver plugins as position-independent shared libraries
plugins: $(MSI_MODERN_SO)

$(MSI_MODERN_SO): $(MSI_MODERN_SRC)
	$(CC) $(PLUGIN_CFLAGS) -I$(INCLUDE_DIR) \
	      $(MSI_MODERN_SRC) -o $(MSI_MODERN_SO)
	@echo "[OK] Plugin built --> $(MSI_MODERN_SO)"

# Build the DMI-based hardware detection utility
detect: $(DETECT_TARGET)

$(DETECT_TARGET): $(DETECT_SRC)
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) \
	      $(DETECT_SRC) -o $(DETECT_TARGET)
	@echo "[OK] Detection utility built --> $(DETECT_TARGET)"

# Build the Debian .deb package (requires debhelper installed)
deb:
	dpkg-buildpackage -us -uc -b
	@echo "[OK] .deb package built."

# Remove all compiled artefacts
clean:
	rm -f $(DAEMON_TARGET)
	rm -f $(MSI_MODERN_SO)
	rm -f $(DETECT_TARGET)
	@echo "[OK] Cleaned."
