#ifndef PROFILES_H
#define PROFILES_H

#include "driver_plugin.h"

/*
 * profiles.h — Fan profile string parser interface.
 *
 * Parses a SET_CURVE payload string (e.g. "55,64,73,76,82,88;38,43,48,54,60,70,85")
 * and applies it to the hardware via the active driver plugin.
 *
 * The driver pointer is set globally via profiles_set_driver() before
 * apply_profile_from_string() is called.  This keeps the IPC layer
 * decoupled from profiles.c while avoiding a circular dependency between
 * the profiles and ipc translation units.
 */

/*
 * profiles_set_driver() — Register the active hardware driver plugin.
 *
 * Must be called once from main() after load_plugin() succeeds, before
 * any IPC client can issue a SET_CURVE command.
 */
void profiles_set_driver(driver_plugin_t *driver);

/*
 * apply_profile_from_string() — Parse and apply a custom thermal curve.
 *
 * Parses @payload (format: "t0,t1,t2,t3,t4,t5;s0,s1,s2,s3,s4,s5,s6"),
 * then calls driver->apply_custom_curve() to write the curve into EC memory.
 *
 * Returns 1 on success, 0 if the payload format is invalid or no driver
 * has been registered.
 */
int apply_profile_from_string(const char *payload);

#endif /* PROFILES_H */