/*
 * profiles.c — Fan profile string parser for chorechill-ctl.
 *
 * Parses the SET_CURVE IPC payload and applies the resulting thermal curve
 * to the hardware through the active driver plugin, rather than calling
 * apply_custom_curve() from hardware.h directly.
 *
 * The driver pointer is registered once at startup via profiles_set_driver().
 */

#include <stdio.h>
#include <stdint.h>

#include "../include/profiles.h"
#include "../include/driver_plugin.h"

/* Module-level pointer to the active hardware driver plugin */
static driver_plugin_t *s_driver = NULL;

/*
 * profiles_set_driver() — Register the active hardware driver plugin.
 *
 * Called from main() after load_plugin() succeeds so that subsequent
 * apply_profile_from_string() calls can route to the correct plugin.
 */
void profiles_set_driver(driver_plugin_t *driver)
{
    s_driver = driver;
}

/*
 * apply_profile_from_string() — Parse and apply a custom thermal curve.
 *
 * Expected payload format: "t0,t1,t2,t3,t4,t5;s0,s1,s2,s3,s4,s5,s6"
 * where t0..t5 are six temperature setpoints (°C) and s0..s6 are seven
 * fan speed setpoints (%).
 *
 * Example: "55,64,73,76,82,88;38,43,48,54,60,70,85"
 *
 * Returns 1 on success, 0 if the format is invalid or no driver is set.
 */
int apply_profile_from_string(const char *payload)
{
    if (!s_driver) {
        printf("\n[PROFILES] Error: no driver registered — call profiles_set_driver() first\n");
        return 0;
    }

    uint8_t temps[6];
    uint8_t speeds[7];

    /*
     * %hhu parses directly into an unsigned char (uint8_t).
     * We expect exactly 13 values: 6 temps + 7 speeds.
     */
    int parsed = sscanf(payload,
        "%hhu,%hhu,%hhu,%hhu,%hhu,%hhu;%hhu,%hhu,%hhu,%hhu,%hhu,%hhu,%hhu",
        &temps[0],  &temps[1],  &temps[2],  &temps[3],  &temps[4],  &temps[5],
        &speeds[0], &speeds[1], &speeds[2], &speeds[3], &speeds[4], &speeds[5], &speeds[6]);

    if (parsed == 13) {
        printf("\n[PROFILES] Valid profile received. Injecting into EC via plugin...\n");
        /* Route the write through the plugin instead of hardware.h */
        s_driver->apply_custom_curve(temps, speeds);
        return 1;
    } else {
        printf("\n[PROFILES] Error: Invalid payload format (%d/13 values parsed)\n", parsed);
        return 0;
    }
}