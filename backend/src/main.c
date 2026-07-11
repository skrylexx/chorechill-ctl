#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include "../include/hardware.h"
#include "../include/ipc.h"

// global so the signal handler can close it before exit
static int g_sockfd = -1;

static void handle_signal(int sig) {
    printf("\n[MAIN] Signal %d received, shutting down...\n", sig);
    // close the socket so the .sock file is cleaned up by init_ipc on next start
    if (g_sockfd >= 0) close(g_sockfd);
    _exit(0);
}

int main() {
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT,  handle_signal);
    signal(SIGTERM, handle_signal);

    printf("Starting EC controller...\n");
    g_sockfd = init_ipc();
    printf("Waiting for UI commands on socket...\n");

    int loop_counter = 0;
    int temp = 0, fan = 0, fan_rpm = 0;

    while (1) {
        // read hardware state only every 10 iterations (1 second)
        if (loop_counter % 10 == 0) {
            temp = read_cpu_temp();
            fan  = read_fan_speed();
            int fan_time = read_fan_speed_rpm();

            if (fan_time > 0) {
                fan_rpm = (478000 / fan_time);
            } else {
                fan_rpm = 0;
            }

            printf("T:%d°C | F:%d%% | R:%d    \r", temp, fan, fan_rpm);
            fflush(stdout);
        }

        // check for IPC messages every 100ms
        handle_ipc_client(g_sockfd, temp, fan, fan_rpm);

        // if a manual override is active, keep writing the setpoint every cycle —
        // the EC runs its own thermal loop and will reclaim control otherwise
        int manual = ipc_get_manual_speed();
        if (manual >= 0) {
            set_fan_speed_percent(manual);
        }

        usleep(100000);
        loop_counter++;
    }

    return 0;
}