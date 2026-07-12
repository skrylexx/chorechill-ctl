/*
 * plugin_loader.c — Runtime hardware driver plugin loader for chorechill-ctl.
 *
 * Uses the POSIX dlopen()/dlsym()/dlclose() API to load a hardware driver
 * plugin (.so) that exports a driver_plugin_t struct under the symbol name
 * CHORECHILL_DRIVER_SYMBOL.  The loader is intentionally minimal: one plugin
 * may be active at a time, and the handle is stored in a module-level static.
 */

#include <stdio.h>
#include <dlfcn.h>          /* dlopen, dlsym, dlclose, dlerror */

#include "plugin_loader.h"  /* our own header */

/* Module-level handle returned by dlopen(); NULL when no plugin is loaded. */
static void           *s_dl_handle = NULL;

/* Cached pointer to the active driver struct; NULL when unloaded. */
static driver_plugin_t *s_driver   = NULL;

/* -------------------------------------------------------------------------
 * load_plugin()
 *
 * 1. dlopen() the shared library at @plugin_path.
 * 2. dlsym() for the CHORECHILL_DRIVER_SYMBOL export.
 * 3. Call driver->init_driver() so the plugin can open the EC device.
 * 4. Return the driver_plugin_t pointer on success, NULL on any error.
 * ------------------------------------------------------------------------- */
driver_plugin_t *load_plugin(const char *plugin_path)
{
    if (!plugin_path) {
        fprintf(stderr, "[PLUGIN] load_plugin: plugin_path is NULL\n");
        return NULL;
    }

    /* Unload any previously active plugin first */
    if (s_dl_handle) {
        fprintf(stderr, "[PLUGIN] Warning: a plugin is already loaded — unloading it first\n");
        unload_plugin();
    }

    /* Clear any stale dlerror state */
    dlerror();

    /*
     * RTLD_NOW  — resolve all symbols immediately so we catch missing
     *             dependencies before the daemon enters its main loop.
     * RTLD_LOCAL — keep plugin symbols private so they do not pollute
     *              the global symbol table and interfere with other .so files.
     */
    s_dl_handle = dlopen(plugin_path, RTLD_NOW | RTLD_LOCAL);
    if (!s_dl_handle) {
        fprintf(stderr, "[PLUGIN] dlopen('%s') failed: %s\n", plugin_path, dlerror());
        return NULL;
    }

    /* Locate the exported driver struct symbol */
    dlerror(); /* clear again before dlsym */
    driver_plugin_t *driver = (driver_plugin_t *)dlsym(s_dl_handle, CHORECHILL_DRIVER_SYMBOL);
    const char *dlsym_err = dlerror();
    if (dlsym_err) {
        fprintf(stderr, "[PLUGIN] dlsym('%s') failed: %s\n",
                CHORECHILL_DRIVER_SYMBOL, dlsym_err);
        dlclose(s_dl_handle);
        s_dl_handle = NULL;
        return NULL;
    }

    /* Basic sanity: make sure the required function pointers are present */
    if (!driver->init_driver    ||
        !driver->cleanup_driver ||
        !driver->read_cpu_temp  ||
        !driver->read_fan_speed_percent ||
        !driver->read_fan_speed_rpm     ||
        !driver->set_fan_mode           ||
        !driver->set_fan_speed_percent  ||
        !driver->apply_custom_curve)
    {
        fprintf(stderr, "[PLUGIN] Plugin '%s' has NULL function pointers — aborting load\n",
                plugin_path);
        dlclose(s_dl_handle);
        s_dl_handle = NULL;
        return NULL;
    }

    /* Announce which driver we are loading */
    printf("[PLUGIN] Loading driver: %s (supports: %s)\n",
           driver->driver_name      ? driver->driver_name      : "<unnamed>",
           driver->supported_hardware ? driver->supported_hardware : "<unknown hw>");

    /* Call the driver's own initialisation hook */
    if (driver->init_driver() != 0) {
        fprintf(stderr, "[PLUGIN] init_driver() returned an error for plugin '%s'\n",
                plugin_path);
        dlclose(s_dl_handle);
        s_dl_handle = NULL;
        return NULL;
    }

    printf("[PLUGIN] Plugin loaded successfully from: %s\n", plugin_path);
    s_driver = driver;
    return driver;
}

/* -------------------------------------------------------------------------
 * unload_plugin()
 *
 * Call cleanup_driver() (if a plugin is active) and then dlclose() the
 * handle.  Safe to call multiple times or when nothing is loaded.
 * ------------------------------------------------------------------------- */
void unload_plugin(void)
{
    if (!s_dl_handle) {
        /* Nothing to do */
        return;
    }

    /* Allow the plugin to release its EC file descriptor and resources */
    if (s_driver && s_driver->cleanup_driver) {
        printf("[PLUGIN] Calling cleanup_driver()...\n");
        s_driver->cleanup_driver();
    }

    dlclose(s_dl_handle);
    s_dl_handle = NULL;
    s_driver    = NULL;
    printf("[PLUGIN] Plugin unloaded.\n");
}
