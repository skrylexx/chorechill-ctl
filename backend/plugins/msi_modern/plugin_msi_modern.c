/*
 * plugin_msi_modern.c  MSI Modern EC Fan-Control Driver Plugin
 *
 * Part of the chorechill-ctl project.
 * Implements driver_plugin_t for MSI laptops using the "modern" EC layout
 * (confirmed on MS-16R4 / GF63 Thin 10SC; also tested on MS-16R5).
 *
 * VALIDATED EC REGISTER MAP (via /sys/kernel/debug/ec/ec0/io)
 *
 *  Address  Width  Description
 *  0x68     1 B    CPU temperature fallback (degree C, unsigned)
 *  0x71     1 B    Fan duty cycle, read/write (0-100 %)
 *  0xCC     2 B    Fan speed raw timer, big-endian (bytes CC-CD)
 *  0x6A-0x6F 1B ea Fan curve temperature thresholds 0-5 (degree C)
 *  0x72-0x78 1B ea Fan curve speed entries 0-6 (%)
 *  0xF4     1 B    Fan control mode (0 = auto / EC firmware, 1 = manual)
 *
 * CPU TEMPERATURE READING STRATEGY
 *
 *  1. /sys/class/thermal/ zones typed "x86_pkg_temp"  (Intel package sensor)
 *  2. /sys/class/thermal/ zones typed "acpitz"         (ACPI generic fallback)
 *  3. EC register 0x68                                  (last resort)
 *
 *  The thermal subsystem is preferred: it does not require debugfs write
 *  access and works across a wider range of kernel configurations.
 *
 * BUILD
 *
 *  gcc -shared -fPIC -o plugin_msi_modern.so plugin_msi_modern.c
 */

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>

#include "../../include/driver_plugin.h"

/* EC sysfs path, requires debugfs mounted and root access */
#define EC_FILE         "/sys/kernel/debug/ec/ec0/io"

/* Thermal subsystem base path */
#define THERMAL_BASE    "/sys/class/thermal"

/* EC register addresses */
#define ADDR_CPU_TEMP       0x68
#define ADDR_FAN_SPEED      0x71
#define ADDR_FAN_SPEED_RPM  0xCC
#define ADDR_CURVE_TEMP0    0x6A
#define ADDR_CURVE_SPEED0   0x72
#define ADDR_FAN_MODE       0xF4


/* ============================================================
 * PRIVATE HELPERS: EC I/O
 * ============================================================ */

/* Read one byte from the EC at the given offset. Returns the byte (0-255) or -1. */
static int read_ec_byte(off_t address)
{
    int fd = open(EC_FILE, O_RDONLY);
    if (fd == -1) return -1;
    if (lseek(fd, address, SEEK_SET) == -1) { close(fd); return -1; }

    uint8_t val;
    if (read(fd, &val, 1) != 1) { close(fd); return -1; }

    close(fd);
    return (int)val;
}

/* Read two bytes (big-endian) from the EC at the given offset. Returns -1 on error. */
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

/* Write one byte to the EC at the given offset. Returns 0 on success, -1 on error. */
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
 * PRIVATE HELPERS: thermal subsystem
 * ============================================================ */

/*
 * read_thermal_zone_by_type()
 *
 * Scans /sys/class/thermal/thermal_zoneN/ directories and returns the
 * temperature in degrees C for the first zone whose "type" file matches
 * the given type string. Returns -1 if no matching zone is found.
 */
static int read_thermal_zone_by_type(const char *type)
{
    DIR *dir = opendir(THERMAL_BASE);
    if (!dir) return -1;

    struct dirent *entry;
    int result = -1;

    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, "thermal_zone", 12) != 0)
            continue;

        /* Read the type file for this zone */
        char type_path[512];
        snprintf(type_path, sizeof(type_path),
                 "%s/%s/type", THERMAL_BASE, entry->d_name);

        FILE *fp = fopen(type_path, "r");
        if (!fp) continue;

        char zone_type[64];
        if (!fgets(zone_type, sizeof(zone_type), fp)) { fclose(fp); continue; }
        fclose(fp);

        /* Strip trailing newline */
        zone_type[strcspn(zone_type, "\n")] = '\0';

        if (strcmp(zone_type, type) != 0)
            continue;

        /* Match found: read the temperature (in millidegrees) */
        char temp_path[512];
        snprintf(temp_path, sizeof(temp_path),
                 "%s/%s/temp", THERMAL_BASE, entry->d_name);

        FILE *tf = fopen(temp_path, "r");
        if (!tf) continue;

        long millideg;
        if (fscanf(tf, "%ld", &millideg) == 1)
            result = (int)(millideg / 1000);

        fclose(tf);
        break;
    }

    closedir(dir);
    return result;
}

/*
 * read_cpu_temp_thermal()
 *
 * Try thermal zones in order of preference:
 *   1. x86_pkg_temp  (Intel package temperature, most accurate)
 *   2. acpitz        (ACPI generic, available on most laptops)
 * Returns temperature in degrees C, or -1 if neither is available.
 */
static int read_cpu_temp_thermal(void)
{
    int temp = read_thermal_zone_by_type("x86_pkg_temp");
    if (temp >= 0) return temp;

    temp = read_thermal_zone_by_type("acpitz");
    return temp;
}


/* ============================================================
 * PLUGIN LIFE CYCLE
 * ============================================================ */

static int plugin_init(void)
{
    /* Check thermal subsystem availability first */
    int temp = read_cpu_temp_thermal();
    if (temp >= 0) {
        fprintf(stderr, "[msi_modern] Thermal subsystem available (%d°C on init).\n", temp);
    } else {
        fprintf(stderr, "[msi_modern] Thermal subsystem not available, will use EC fallback.\n");
    }

    /* Always verify EC file is accessible (needed for fan control) */
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

    fprintf(stderr, "[msi_modern] EC file accessible: %s\n", EC_FILE);
    return 0;
}

static void plugin_cleanup(void)
{
    fprintf(stderr, "[msi_modern] Driver unloaded.\n");
}


/* ============================================================
 * READ FUNCTIONS
 * ============================================================ */

/*
 * plugin_read_cpu_temp()
 *
 * Try the thermal subsystem first (x86_pkg_temp, then acpitz).
 * Fall back to the EC register 0x68 if the kernel thermal interface
 * is not available or returns an error.
 */
static int plugin_read_cpu_temp(void)
{
    int temp = read_cpu_temp_thermal();
    if (temp >= 0) return temp;

    /* EC fallback */
    return read_ec_byte(ADDR_CPU_TEMP);
}

/* Read current fan duty cycle. Returns 0-100 or -1. */
static int plugin_read_fan_speed_percent(void)
{
    return read_ec_byte(ADDR_FAN_SPEED);
}

/*
 * Read raw fan RPM timer from EC (big-endian word at 0xCC-0xCD).
 * The caller converts to RPM with: rpm = 478000 / raw_value.
 * Returns the raw timer value, or -1 on error.
 */
static int plugin_read_fan_speed_rpm(void)
{
    return read_ec_word(ADDR_FAN_SPEED_RPM);
}


/* ============================================================
 * WRITE FUNCTIONS
 * ============================================================ */

/* Switch fan control mode: 0 = auto (EC firmware), 1 = manual (daemon controls). */
static void plugin_set_fan_mode(int mode)
{
    if (write_ec_byte(ADDR_FAN_MODE, (uint8_t)mode) == -1)
        fprintf(stderr, "[msi_modern] WARNING: Failed to set fan mode to %d.\n", mode);
}

/* Lock the fan at a fixed duty cycle (0-100). Only effective in manual mode. */
static void plugin_set_fan_speed_percent(int percent)
{
    if (percent < 0)   percent = 0;
    if (percent > 100) percent = 100;

    if (write_ec_byte(ADDR_FAN_SPEED, (uint8_t)percent) == -1)
        fprintf(stderr, "[msi_modern] WARNING: Failed to set fan speed to %d%%.\n", percent);
}

/*
 * Push a full fan curve to the EC.
 *
 *   temps[6]   six ascending temperature thresholds in degrees C
 *              Written to EC registers 0x6A through 0x6F
 *   speeds[7]  seven fan duty-cycle values (0-100)
 *              Written to EC registers 0x72 through 0x78
 *
 * The EC applies the curve autonomously in auto mode.
 */
static void plugin_apply_custom_curve(const uint8_t temps[6],
                                      const uint8_t speeds[7])
{
    for (int i = 0; i < 6; i++) {
        if (write_ec_byte(ADDR_CURVE_TEMP0 + i, temps[i]) == -1)
            fprintf(stderr, "[msi_modern] WARNING: Failed to write temp threshold %d (0x%02X).\n",
                    i, ADDR_CURVE_TEMP0 + i);
    }

    for (int i = 0; i < 7; i++) {
        if (write_ec_byte(ADDR_CURVE_SPEED0 + i, speeds[i]) == -1)
            fprintf(stderr, "[msi_modern] WARNING: Failed to write speed entry %d (0x%02X).\n",
                    i, ADDR_CURVE_SPEED0 + i);
    }
}


/* ============================================================
 * EXPORTED PLUGIN DESCRIPTOR
 *
 * Single symbol looked up by the daemon via dlsym(3).
 * Name must match CHORECHILL_DRIVER_SYMBOL ("chorechill_driver").
 * ============================================================ */
driver_plugin_t chorechill_driver = {
    .driver_name        = "MSI Modern EC Driver",
    .supported_hardware = "MSI GF63 Thin / MS-16R4, MS-16R5",

    .init_driver        = plugin_init,
    .cleanup_driver     = plugin_cleanup,

    .read_cpu_temp          = plugin_read_cpu_temp,
    .read_fan_speed_percent = plugin_read_fan_speed_percent,
    .read_fan_speed_rpm     = plugin_read_fan_speed_rpm,

    .set_fan_mode           = plugin_set_fan_mode,
    .set_fan_speed_percent  = plugin_set_fan_speed_percent,
    .apply_custom_curve     = plugin_apply_custom_curve,
};
