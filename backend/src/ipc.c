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
#include "../include/hardware.h"  // we route SET: commands directly to hardware
#include "../include/profiles.h" // we route SET_CURVE: payload to profiles.c

#define SOCKET_PATH "/tmp/chorechill-ctl.sock"

// -1 = no override active (curve mode); >= 0 = manual % to keep writing every loop
static volatile int s_manual_speed = -1;

int ipc_get_manual_speed() {
    return s_manual_speed;
}

void ipc_clear_manual_speed() {
    s_manual_speed = -1;
}

// ==========================================
// PUBLIC FUNCTIONS
// ==========================================

int init_ipc() {
    int sockfd;
    struct sockaddr_un server_addr;

    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0) exit(EXIT_FAILURE);

    // remove stale socket file from a previous run
    unlink(SOCKET_PATH);

    memset(&server_addr, 0, sizeof(struct sockaddr_un));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);

    if (bind(sockfd, (struct sockaddr *) &server_addr, sizeof(struct sockaddr_un)) < 0) {
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // allow any user (the GUI runs as non-root) to connect
    chmod(SOCKET_PATH, 0666);

    if (listen(sockfd, 5) < 0) {
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // non-blocking so the main loop never stalls waiting for a connection
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

    printf("IPC Server started on %s\n", SOCKET_PATH);
    return sockfd;
}

void handle_ipc_client(int sockfd, int temp, int fan, int rpm) {
    int client_fd = accept(sockfd, NULL, NULL);

    // EAGAIN/EWOULDBLOCK = no client waiting, nothing to do
    if (client_fd < 0) return;

    char buffer[256];
    memset(buffer, 0, sizeof(buffer));

    int bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);

    if (bytes_read > 0) {
        // route SET_CURVE to the profile parser (e.g. "SET_CURVE:55,64,...;38,43,...")
        if (strncmp(buffer, "SET_CURVE:", 10) == 0) {
            // buffer + 10 skips the "SET_CURVE:" prefix and sends the raw payload
            if (apply_profile_from_string(buffer + 10)) {
                // the EC now runs its own curve autonomously, disabling the manual re-write loop
                ipc_clear_manual_speed();
            } else {
                char err_res[] = "ERR:INVALID_FORMAT";
                ssize_t written = write(client_fd, err_res, strlen(err_res));
                (void)written;
                close(client_fd);
                return;
            }

        // route SET: to a direct fan speed override (e.g. "SET:75" --> lock at 75%)
        } else if (strncmp(buffer, "SET:", 4) == 0) {
            int target_speed = atoi(buffer + 4);
            // clamp the value between 0 and 100 before writing to the EC
            if (target_speed < 0)   target_speed = 0;
            if (target_speed > 100) target_speed = 100;
            printf("\n[IPC] Manual override: locking fan at %d%%\n", target_speed);
            // store it so the main loop keeps re-writing it (EC would override otherwise)
            s_manual_speed = target_speed;
        }
        // GET or any unknown command falls through and just returns telemetry below
    }

    // always respond with the latest telemetry so the GUI can update its display
    char response[256];
    snprintf(response, sizeof(response), "%d,%d,%d", temp, fan, rpm);
    ssize_t written = write(client_fd, response, strlen(response));
    (void)written;  // partial writes acceptable: GUI will retry on next poll

    close(client_fd);
}