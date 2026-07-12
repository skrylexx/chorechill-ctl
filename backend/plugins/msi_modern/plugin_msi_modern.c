/*
 * plugin_msi_modern.c — MSI Modern EC Fan-Control Driver Plugin
 *
 * Part of the chorechill-ctl project.
 * Implements driver_plugin_t for MSI laptops using the "modern" EC layout
 * (confirmed on MS-16R4 / GF63 Thin 10SC; also tested on MS-16R5).
 *
 * ============================================================
 * VALIDATED EC REGISTER MAP (via /sys/kernel/debug/ec/ec0/io)
 * ============================================================
 *
 *  Address  Width  Description
 *  -------  -----  -----------
 *  0x68     1 B    CPU package temperature (°C, unsigned)
 *  0x71     1 B    Fan duty cycle, read/write (0–100 %)
 *  0xCC     2 B    Fan speed in RPM, big-endian (bytes CC–CD)
 *  0x6A     1 B    Fan curve — temperature threshold 0 (°C)
 *  0x6B     1 B    Fan curve — temperature threshold 1 (°C)
 *  0x6C     1 B    Fan curve — temperature threshold 2 (°C)
 *  0x6D     1 B    Fan curve — temperature threshold 3 (°C)
 *  0x6E     1 B    Fan curve — temperature threshold 4 (°C)
 *  0x6F     1 B    Fan curve — temperature threshold 5 (°C)
 *  0x72     1 B    Fan curve — speed for zone 0 (%)
 *  0x73     1 B    Fan curve — speed for zone 1 (%)
 *  0x74     1 B    Fan curve — speed for zone 2 (%)
 *  0x75     1 B    Fan curve — speed for zone 3 (%)
 *  0x76     1 B    Fan curve — speed for zone 4 (%)
 *  0x77     1 B    Fan curve — speed for zone 5 (%)
 *  0x78     1 B    Fan curve — speed for zone 6 (%) [above threshold 5]
 *  0xF4     1 B    Fan control mode (0 = auto / EC firmware, 1 = manual)
 *
 * ============================================================
 * BUILD INSTRUCTIONS
 * ============================================================
 *
 *  # Compile this file as a position-independent shared library:
 *  gcc -shared -fPIC -o plugin_msi_modern.so plugin_msi_modern.c
 *
 *  # Install alongside the daemon (example path):
 *  install -m 644 plugin_msi_modern.so /usr/lib/chorechill-ctl/
 *
 *  # The daemon loads the plugin at runtime via dlopen(3):
 *  PLUGIN=$(chorechill-detect)          # e.g. "plugin_msi_modern.so"
 *  chorechill-daemon --plugin $PLUGIN
 */

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>

/* Include the plugin API definition. When compiling standalone (without the
 * full project tree) copy driver_plugin.h next to this file and adjust the
 * path below accordingly. */
#include "../../include/driver_plugin.h"

/* Path to the EC I/O file exposed by the acpi_ec kernel module.
 * Requires debugfs to be mounted and read/write access (typically root).     */
#define EC_FILE "/sys/kernel/debug/ec/ec0/io"

/* ============================================================
 * EC REGISTER ADDRESSES
 * ============================================================ */
#define ADDR_CPU_TEMP       0x68  /* CPU temperature (°C)                     */
#define ADDR_FAN_SPEED      0x71  /* Fan duty cycle (0–100 %)                 */
#define ADDR_FAN_SPEED_RPM  0xCC  /* Fan speed MSB (big-endian word at CC–CD) */
#define ADDR_CURVE_TEMP0    0x6A  /* First temperature threshold of the curve  */
#define ADDR_CURVE_SPEED0   0x72  /* First speed entry of the curve            */
#define ADDR_FAN_MODE       0xF4  /* Fan control mode register                 */

/* ============================================================
 * PRIVATE HELPERS
 * These are file-local (static) to avoid symbol clashes when
 * multiple plugins are loaded into the same process.
 * ============================================================ */

/*
 * read_ec_byte() — seek to 'address' in the EC file and read one byte.
 * Returns the byte value (0–255) on success, or -1 on any I/O error.
 */
static int read_ec_byte(off_t address)
{
    int fd = open(EC_FILE, O_RDONLY);
    if (fd == -1) return -1;

    if (lseek(fd, address, SEEK_SET) == -1) { close(fd); return -1; }

    uint8_t byte_value;
    if (read(fd, &byte_value, 1) != 1) { close(fd); return -1; }

    close(fd);
    return (int)byte_value;
}

/*
 * read_ec_word() — seek to 'address' and read two consecutive bytes.
 * The EC stores RPM in big-endian order (MSB first).
 * Returns the 16-bit value on success, or -1 on any I/O error.
 */
static int read_ec_word(off_t address)
{
    int fd = open(EC_FILE, O_RDONLY);
    if (fd == -1) return -1;

    if (lseek(fd, address, SEEK_SET) == -1) { close(fd); return -1; }

    uint8_t bytes[2];
    if (read(fd, bytes, 2) != 2) { close(fd); return -1; }

    close(fd);
    return (int)((bytes[0] << 8) | bytes[1]);
}

/*
 * write_ec_byte() — seek to 'address' and write a single byte.
 * Returns 0 on success, -1 on any I/O error.
 */
static int write_ec_byte(off_t address, uint8_t value)
{
    int fd = open(EC_FILE, O_RDWR);
    if (fd == -1) return -1;

    if (lseek(fd, address, SEEK_SET) == -1) { close(fd); return -1; }
    if (write(fd, &value, 1) != 1)          { close(fd); return -1; }

    close(fd);
    return 0;
}

/* ============================================================
 * PLUGIN LIFE CYCLE
 * ============================================================ */

/*
 * plugin_init() — verify that the EC file is accessible.
 *
 * Opens the EC file in read-only mode.  Write access is checked lazily the
 * first time a write function is called (so that the plugin can still be
 * used in a read-only monitoring mode without root).
 *
 * Returns 0 on success, -1 if the EC file cannot be opened.
 */
static int plugin_init(void)
{
    int fd = open(EC_FILE, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr,
                "[msi_modern] ERROR: Cannot open EC file '%s'.\n"
                "             Ensure debugfs is mounted and you have read access.\n"
                "             Try: mount -t debugfs none /sys/kernel/debug\n",
                EC_FILE);
        return -1;
    }
    close(fd);

    fprintf(stderr,
            "[msi_modern] Driver initialised. EC file accessible: %s\n",
            EC_FILE);
    return 0;
}

/*
 * plugin_cleanup() — release resources held by this driver.
 *
 * This plugin opens and closes the EC file on every individual operation, so
 * there is no persistent handle to release.  Future versions that keep a
 * persistent fd open should close it here.
 */
static void plugin_cleanup(void)
{
    fprintf(stderr, "[msi_modern] Driver cleaned up (no persistent handles).\n");
}

/* ============================================================
 * READ FUNCTIONS
 * ============================================================ */

/*
 * plugin_read_cpu_temp() — read CPU package temperature.
 * Returns temperature in °C (typically 30–100), or -1 on error.
 */
static int plugin_read_cpu_temp(void)
{
    return read_ec_byte(ADDR_CPU_TEMP);
}

/*
 * plugin_read_fan_speed_percent() — read current fan duty cycle.
 * Returns a value in [0, 100], or -1 on error.
 */
static int plugin_read_fan_speed_percent(void)
{
    return read_ec_byte(ADDR_FAN_SPEED);
}

/*
 * plugin_read_fan_speed_rpm() — read current fan rotational speed.
 * Returns RPM (big-endian 16-bit word at 0xCC–0xCD), or -1 on error.
 */
static int plugin_read_fan_speed_rpm(void)
{
    return read_ec_word(ADDR_FAN_SPEED_RPM);
}

/* ============================================================
 * WRITE FUNCTIONS
 * ============================================================ */

/*
 * plugin_set_fan_mode() — switch between automatic and manual fan control.
 *
 *   mode = 0 → EC firmware manages the fan (auto / cooler boost off)
 *   mode = 1 → daemon controls fan speed directly via ADDR_FAN_SPEED
 *
 * Invalid values are silently ignored by the EC, but callers should stick
 * to 0 and 1 for portability.
 */
static void plugin_set_fan_mode(int mode)
{
    if (write_ec_byte(ADDR_FAN_MODE, (uint8_t)mode) == -1) {
        fprintf(stderr,
                "[msi_modern] WARNING: Failed to set fan mode to %d.\n", mode);
    }
}

/*
 * plugin_set_fan_speed_percent() — lock the fan at a fixed duty cycle.
 *
 * Clamps 'percent' to [0, 100] before writing.  Only effective when the EC
 * is in manual mode (set_fan_mode(1)).
 */
static void plugin_set_fan_speed_percent(int percent)
{
    /* Hard clamp — never pass out-of-range values to the EC */
    if (percent < 0)   percent = 0;
    if (percent > 100) percent = 100;

    if (write_ec_byte(ADDR_FAN_SPEED, (uint8_t)percent) == -1) {
        fprintf(stderr,
                "[msi_modern] WARNING: Failed to set fan speed to %d%%.\n",
                percent);
    }
}

/*
 * plugin_apply_custom_curve() — push a full fan curve to the EC.
 *
 *   temps[6]  — six ascending temperature thresholds in °C
 *               Written to EC registers 0x6A … 0x6F
 *   speeds[7] — seven fan duty-cycle percentages (0–100)
 *               Written to EC registers 0x72 … 0x78
 *               speeds[0] applies below temps[0]; speeds[6] applies above temps[5].
 *
 * The EC applies the curve autonomously in auto mode.  In manual mode the
 * curve registers are ignored until the mode is switched back to auto.
 */
static void plugin_apply_custom_curve(const uint8_t temps[6],
                                      const uint8_t speeds[7])
{
    /* Write 6 temperature thresholds: 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F */
    for (int i = 0; i < 6; i++) {
        if (write_ec_byte(ADDR_CURVE_TEMP0 + i, temps[i]) == -1) {
            fprintf(stderr,
                    "[msi_modern] WARNING: Failed to write temp threshold %d "
                    "(addr=0x%02X, val=%u).\n",
                    i, ADDR_CURVE_TEMP0 + i, temps[i]);
        }
    }

    /* Write 7 fan speed entries: 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78 */
    for (int i = 0; i < 7; i++) {
        if (write_ec_byte(ADDR_CURVE_SPEED0 + i, speeds[i]) == -1) {
            fprintf(stderr,
                    "[msi_modern] WARNING: Failed to write speed entry %d "
                    "(addr=0x%02X, val=%u).\n",
                    i, ADDR_CURVE_SPEED0 + i, speeds[i]);
        }
    }
}

/* ============================================================
 * EXPORTED PLUGIN DESCRIPTOR
 *
 * This is the single symbol the daemon looks up via dlsym(3).
 * Its name MUST match CHORECHILL_DRIVER_SYMBOL ("chorechill_driver").
 * ============================================================ */
driver_plugin_t chorechill_driver = {
    .driver_name        = "MSI Modern EC Driver",
    .supported_hardware = "MSI GF63 Thin / MS-16R4, MS-16R5",

    /* Life cycle */
    .init_driver        = plugin_init,
    .cleanup_driver     = plugin_cleanup,

    /* Reads */
    .read_cpu_temp          = plugin_read_cpu_temp,
    .read_fan_speed_percent = plugin_read_fan_speed_percent,
    .read_fan_speed_rpm     = plugin_read_fan_speed_rpm,

    /* Writes */
    .set_fan_mode           = plugin_set_fan_mode,
    .set_fan_speed_percent  = plugin_set_fan_speed_percent,
    .apply_custom_curve     = plugin_apply_custom_curve,
};
