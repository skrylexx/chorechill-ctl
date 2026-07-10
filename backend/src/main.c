#include <stdio.h>
#include <unistd.h>
#include "../include/hardware.h"
#include "../include/ipc.h"

int main() {
    printf("Starting EC controller...\n");

    printf("Starting socket...\n");
    int sockfd = init_ipc();

    // THE HACK: Overwrite the EC's internal table once and for all.
    // EC will naturally read this table and apply 80% without fighting us.
    printf("Injecting custom fan curve (Flat 80%%)...\n");
    // set_custom_fan_curve();
    printf("Restoring factory fan curve...\n");
    reset_default_fan_curve();

    // infinite telemetry loop
    while(1) {
        // read hardware state
        int temp = read_cpu_temp();
        int fan = read_fan_speed();
        int fan_time = read_fan_speed_rpm();
        
        // prevent division by zero if fan is physically off
        int fan_rpm = 0;
        if (fan_time > 0) {
            fan_rpm = (478000 / fan_time); 
        }
        
        // print to console (using \r to overwrite the same line cleanly)
        printf("T:%d°C | F:%d%% | R:%d    \r", temp, fan, fan_rpm);
        fflush(stdout);
        
        // handle Python frontend requests (Non-blocking)
        handle_ipc_client(sockfd, temp, fan, fan_rpm);
        
        // standard pause. 
        // no need to spam the EC anymore, it manages the % by itself now.
        sleep(1); 
    }

    return 0;
}