/*
 * ipc.c  Unix-domain socket IPC server for chorechill-ctl.
 *
 * Listens on SOCKET_PATH for commands from the GUI frontend.
 * All hardware writes are now routed through the driver plugin pointer
 * passed into handle_ipc_client() instead of calling hardware.h directly.
 *
 * Supported commands:
 *   GET              — return latest telemetry as "temp,fan%,rpm"
 *   SET:<pct>        — lock fan at <pct>% via driver->set_fan_speed_percent()
 *   SET_CURVE:<data> — parse curve payload and apply via apply_profile_from_string()
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>

#include "../include/ipc.h"
#include "../include/driver_plugin.h" /* driver_plugin_t — hardware calls go through here */
#include "../include/plugin_loader.h"
#include "../include/profiles.h"      /* apply_profile_from_string() */

#define SOCKET_PATH "/tmp/chorechill-ctl.sock"

/* -1 = no override active (curve mode); >= 0 = manual % to keep writing every loop */
static volatile int s_manual_speed = -1;

int ipc_get_manual_speed(void)
{
    return s_manual_speed;
}

void ipc_clear_manual_speed(void)
{
    s_manual_speed = -1;
}

/* =========================================================================
 * PUBLIC FUNCTIONS
 * ========================================================================= */

int init_ipc(void)
{
    int sockfd;
    struct sockaddr_un server_addr;

    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0) exit(EXIT_FAILURE);

    /* Remove stale socket file from a previous run */
    unlink(SOCKET_PATH);

    memset(&server_addr, 0, sizeof(struct sockaddr_un));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_un)) < 0) {
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    /* Allow any user (the GUI runs as non-root) to connect */
    chmod(SOCKET_PATH, 0666);

    if (listen(sockfd, 5) < 0) {
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    /* Non-blocking so the main loop never stalls waiting for a connection */
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

    printf("IPC Server started on %s\n", SOCKET_PATH);
    return sockfd;
}

/*
 * handle_ipc_client()
 *
 * Accept one pending client connection (non-blocking). Parse the command
 * and route hardware writes through @driver instead of the old hardware.h
 * direct calls. Always reply with the latest telemetry string so the GUI
 * can refresh its display regardless of which command was sent.
 */
void handle_ipc_client(int sockfd, int temp, int fan, int rpm,
                        driver_plugin_t *driver)
{
    int client_fd = accept(sockfd, NULL, NULL);

    /* EAGAIN / EWOULDBLOCK means no client is waiting — nothing to do */
    if (client_fd < 0) return;

    char buffer[256];
    memset(buffer, 0, sizeof(buffer));

    int bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);

    if (bytes_read > 0) {

        /* --- SET_CURVE:<temps>;<speeds> ---------------------------------- */
        if (strncmp(buffer, "SET_CURVE:", 10) == 0) {
            /*
             * Delegate parsing to profiles.c.  apply_profile_from_string()
             * calls driver->apply_custom_curve() internally via the global
             * driver pointer set by profiles.c (see profiles.c for details).
             * After this the EC runs the curve autonomously, so we cancel
             * the manual re-write loop.
             */
            if (apply_profile_from_string(buffer + 10)) {
                /* EC is now running the curve on its own — disable manual loop */
                ipc_clear_manual_speed();
            } else {
                char err_res[] = "ERR:INVALID_FORMAT";
                ssize_t written = write(client_fd, err_res, strlen(err_res));
                (void)written;
                close(client_fd);
                return;
            }

        /* --- SET:<pct> --------------------------------------------------- */
        } else if (strncmp(buffer, "SET:", 4) == 0) {
            int target_speed = atoi(buffer + 4);
            /* Clamp between 0 and 100 before writing to the EC */
            if (target_speed < 0)   target_speed = 0;
            if (target_speed > 100) target_speed = 100;

            printf("\n[IPC] Manual override: locking fan at %d%%\n", target_speed);

            /*
             * Route the write through the plugin instead of calling
             * set_fan_speed_percent() from hardware.h directly.
             * Store the setpoint so the main loop keeps re-issuing it.
             */
            if (driver)
                driver->set_fan_speed_percent(target_speed);

            s_manual_speed = target_speed;
        }
        /* GET or any unknown command falls through and returns telemetry */
    }

    /* Always respond with the latest telemetry so the GUI can refresh */
    char response[256];
    snprintf(response, sizeof(response), "%d,%d,%d", temp, fan, rpm);
    ssize_t written = write(client_fd, response, strlen(response));
    (void)written;  /* partial writes acceptable: GUI retries on next poll */

    close(client_fd);
}