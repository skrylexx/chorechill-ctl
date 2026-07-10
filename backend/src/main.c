#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include "../include/hardware.h"
#include "../include/ipc.h"

int main() {
    signal(SIGPIPE, SIG_IGN);

    printf("Starting EC controller...\n");
    int sockfd = init_ipc();
    printf("Waiting for UI commands on socket...\n");

    int loop_counter = 0;
    int temp = 0, fan = 0, fan_rpm = 0;

    while(1) {
        // read hardware state only every 10 iterations (1 second)
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
        
        // check for IPC messages every 100ms
        handle_ipc_client(sockfd, temp, fan, fan_rpm);
        
        usleep(100000); 
        loop_counter++;
    }

    return 0;
}