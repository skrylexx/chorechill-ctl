#ifndef DRIVER_PLUGIN_H
#define DRIVER_PLUGIN_H

#include <stdint.h>

typedef struct {
    const char *driver_name;
    const char *supported_hardware;

    // --- LIFE CYCLE ---
    int (*init_driver)(void);
    void (*cleanup_driver)(void);

    // --- READ FUNCTIONS ---
    int (*read_cpu_temp)(void);
    int (*read_fan_speed_percent)(void);
    int (*read_fan_speed_rpm)(void);

    // --- WRITE FUNCTIONS ---
    void (*set_fan_mode)(int mode);
    void (*set_fan_speed_percent)(int percent);
    void (*apply_custom_curve)(const uint8_t temps[6], const uint8_t speeds[7]);
} driver_plugin_t;

// Standard export symbol name that plugins must export
#define CHORECHILL_DRIVER_SYMBOL "chorechill_driver"

#endif // DRIVER_PLUGIN_H
