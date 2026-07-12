/*
 * main.c  chorechill-ctl daemon entry point (plugin-based architecture).
 *
 * Loads a hardware driver plugin (.so) at startup, then runs the
 * main 10 Hz polling loop: every second it reads temperature + fan
 * telemetry through the plugin, and every 100 ms it services IPC
 * requests from the GUI.
 *
 * Plugin selection order (highest priority first):
 *   1. Command-line argument:  chorechill-ctl /path/to/plugin.so
 *   2. Environment variable:   CHORECHILL_PLUGIN=/path/to/plugin.so
 *   3. Compiled-in default:    /usr/lib/chorechill-ctl/plugins/plugin_msi_modern.so
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include "../include/ipc.h"
#include "../include/driver_plugin.h"
#include "../include/profiles.h"
#include "../include/plugin_loader.h"

/* Default plugin path used when neither argv[1] nor $CHORECHILL_PLUGIN is set */
#define DEFAULT_PLUGIN_PATH "/usr/lib/chorechill-ctl/plugins/plugin_msi_modern.so"

/* Global socket fd so the signal handler can close it on shutdown */
static int g_sockfd = -1;

/* Signal handler: graceful shutdown on SIGINT / SIGTERM */
static void handle_signal(int sig)
{
    printf("\n[MAIN] Signal %d received, shutting down...\n", sig);

    /* Close the Unix socket so the .sock file is cleaned up */
    if (g_sockfd >= 0)
        close(g_sockfd);

    /* Tear down the hardware driver cleanly */
    unload_plugin();

    _exit(0);
}

/* main() */
int main(int argc, char *argv[])
{
    /* ---- Signal handling ---- */
    signal(SIGPIPE, SIG_IGN);   /* ignore broken-pipe from GUI disconnects */
    signal(SIGINT,  handle_signal);
    signal(SIGTERM, handle_signal);

    /* ---- Determine which plugin to load ---- */
    const char *plugin_path = NULL;

    /* Priority 1: explicit command-line argument */
    if (argc >= 2) {
        plugin_path = argv[1];
        printf("[MAIN] Plugin path from command-line argument: %s\n", plugin_path);
    }

    /* Priority 2: environment variable */
    if (!plugin_path) {
        plugin_path = getenv("CHORECHILL_PLUGIN");
        if (plugin_path)
            printf("[MAIN] Plugin path from $CHORECHILL_PLUGIN: %s\n", plugin_path);
    }

    /* Priority 3: compiled-in default */
    if (!plugin_path) {
        plugin_path = DEFAULT_PLUGIN_PATH;
        printf("[MAIN] Using default plugin path: %s\n", plugin_path);
    }

    /* ---- Load the hardware driver plugin ---- */
    driver_plugin_t *driver = load_plugin(plugin_path);
    if (!driver) {
        fprintf(stderr, "[MAIN] Fatal: could not load plugin '%s'. Exiting.\n", plugin_path);
        exit(1);
    }

    /* Register the plugin with the profiles module so SET_CURVE commands
     * can call driver->apply_custom_curve() without a direct hardware.h dep */
    profiles_set_driver(driver);

    /* ---- Start the IPC server ---- */
    printf("[MAIN] Starting EC controller...\n");
    g_sockfd = init_ipc();
    printf("[MAIN] Waiting for UI commands on socket...\n");

    /* ---- Main loop ---- */
    int loop_counter = 0;
    int temp = 0, fan = 0, fan_rpm = 0;

    while (1) {
        /*
         * Hardware reads are expensive (sysfs / EC I/O), so we throttle
         * them to once per second (every 10 iterations of the 100 ms loop).
         */
        if (loop_counter % 10 == 0) {
            temp = driver->read_cpu_temp();
            fan  = driver->read_fan_speed_percent();

            /* The EC returns a timer value; convert to RPM */
            int fan_time = driver->read_fan_speed_rpm();
            if (fan_time > 0)
                fan_rpm = 478000 / fan_time;
            else
                fan_rpm = 0;

            printf("T:%d°C | F:%d%% | R:%d    \r", temp, fan, fan_rpm);
            fflush(stdout);
        }

        /* Service any pending IPC request from the GUI (non-blocking) */
        handle_ipc_client(g_sockfd, temp, fan, fan_rpm, driver);

        /*
         * If a manual speed override is active, keep writing the setpoint
         * to the EC every 100 ms — the EC's own thermal loop would otherwise
         * reclaim fan control within a few seconds.
         */
        int manual = ipc_get_manual_speed();
        if (manual >= 0)
            driver->set_fan_speed_percent(manual);

        usleep(100000); /* 100 ms */
        loop_counter++;
    }

    /* Unreachable: the signal handler exits via _exit() */
    return 0;
}