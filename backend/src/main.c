#include <stdio.h>
#include <unistd.h>
#include <signal.h> // required to ignore SIGPIPE
#include "../include/hardware.h"
#include "../include/ipc.h"

int main() {

    // ignore SIGPIPE to prevent the program from crashing if the client disconnects unexpectedly
    signal(SIGPIPE, SIG_IGN);

    printf("Starting EC controller...\n");
    int sockfd = init_ipc();
    printf("Starting socket...\n");

    // declare variables for telemetry & IPC
    int loop_counter = 0;
    int temp = 0, fan = 0, fan_rpm = 0;

    // infinite telemetry loop
    while(1) {
        // // read telemetry every 10 loops (1 second)
        if (loop_counter % 10 == 0) {
            temp = read_cpu_temp();
            fan = read_fan_speed();
            int fan_time = read_fan_speed_rpm();
            
            if (fan_time > 0) {
                fan_rpm = (478000 / fan_time); 
            } else {
                fan_rpm = 0;
            }
            
            printf("T:%d°C | F:%d%% | R:%d    \r", temp, fan, fan_rpm);
            fflush(stdout);
        }
        
        // hear client each loop
        handle_ipc_client(sockfd, temp, fan, fan_rpm);
        
        // mini pause to avoid CPU overload, and increment the loop counter
        usleep(100000); 
        loop_counter++;
    }

    return 0;
}