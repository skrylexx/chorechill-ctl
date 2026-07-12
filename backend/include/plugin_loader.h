#ifndef PLUGIN_LOADER_H
#define PLUGIN_LOADER_H

#include "driver_plugin.h"

/*
 * plugin_loader.h — Dynamic plugin loading interface for chorechill-ctl.
 *
 * Provides a simple dlopen/dlsym wrapper that loads a hardware driver
 * plugin (.so) at runtime, calls its init_driver() hook, and returns
 * a pointer to the populated driver_plugin_t struct.
 */

/*
 * load_plugin() — Open and initialise a hardware driver plugin.
 *
 * Opens the shared library at @plugin_path with dlopen(), locates the
 * exported CHORECHILL_DRIVER_SYMBOL symbol, and calls init_driver().
 *
 * Returns a pointer to the driver_plugin_t struct on success,
 * or NULL if dlopen(), dlsym(), or init_driver() fails.
 * Error details are printed to stderr via dlerror().
 */
driver_plugin_t *load_plugin(const char *plugin_path);

/*
 * unload_plugin() — Tear down and close the currently loaded plugin.
 *
 * Calls cleanup_driver() on the active plugin (if any) and then
 * dlclose()s the handle.  Safe to call even if no plugin is loaded.
 */
void unload_plugin(void);

#endif /* PLUGIN_LOADER_H */
