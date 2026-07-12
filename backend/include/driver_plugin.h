/*
 * driver_plugin.h — chorechill-ctl plugin API
 *
 * Every hardware driver is compiled as a shared library (.so) that exports
 * exactly one symbol named CHORECHILL_DRIVER_SYMBOL ("chorechill_driver").
 * That symbol must be of type driver_plugin_t and have all function pointers
 * populated before the library is dlopen()'d by the daemon.
 *
 * Compile a plugin as a shared library:
 *   gcc -shared -fPIC -o plugin_foo.so plugin_foo.c
 *
 * Load a plugin in the daemon (pseudo-code):
 *   void *h = dlopen("plugin_foo.so", RTLD_NOW);
 *   driver_plugin_t *drv = dlsym(h, CHORECHILL_DRIVER_SYMBOL);
 *   drv->init_driver();
 */

#ifndef DRIVER_PLUGIN_H
#define DRIVER_PLUGIN_H

#include <stdint.h>

typedef struct {
    /* Human-readable identifiers printed during daemon startup */
    const char *driver_name;        /* e.g. "MSI Modern EC Driver"            */
    const char *supported_hardware; /* e.g. "MSI GF63 Thin / MS-16R4"         */

    /* --- LIFE CYCLE -------------------------------------------------------- */

    /* Open the EC file, validate read/write access.
     * Returns 0 on success, -1 on failure (e.g. file not found, EPERM).      */
    int  (*init_driver)(void);

    /* Release any open file handles or allocated resources.                   */
    void (*cleanup_driver)(void);

    /* --- READ FUNCTIONS ---------------------------------------------------- */

    /* Read the CPU package temperature from the EC.
     * Returns temperature in °C, or -1 on error.                              */
    int  (*read_cpu_temp)(void);

    /* Read the current fan duty cycle as reported by the EC.
     * Returns a value in the range [0, 100], or -1 on error.                  */
    int  (*read_fan_speed_percent)(void);

    /* Read the current fan rotational speed.
     * Returns RPM (typically 0–6000), or -1 on error.                         */
    int  (*read_fan_speed_rpm)(void);

    /* --- WRITE FUNCTIONS --------------------------------------------------- */

    /* Set the EC fan-control mode.
     *   mode = 0 → automatic (thermal management handled by EC firmware)
     *   mode = 1 → manual   (daemon controls speed directly)                  */
    void (*set_fan_mode)(int mode);

    /* Lock the fan at a fixed duty cycle.
     * percent is clamped to [0, 100] before writing to the EC.                */
    void (*set_fan_speed_percent)(int percent);

    /* Push a full fan curve to the EC.
     *   temps[6]  — six ascending temperature thresholds (°C)
     *   speeds[7] — seven fan duty-cycle values (%) corresponding to each
     *               thermal zone (the last entry covers "above temps[5]")      */
    void (*apply_custom_curve)(const uint8_t temps[6], const uint8_t speeds[7]);

} driver_plugin_t;

/* Every .so plugin must export a global driver_plugin_t under this name.
 * Example:
 *   driver_plugin_t chorechill_driver = { .driver_name = "...", ... };        */
#define CHORECHILL_DRIVER_SYMBOL "chorechill_driver"

#endif /* DRIVER_PLUGIN_H */
