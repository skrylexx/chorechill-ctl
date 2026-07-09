#include <stdio.h>
#include <unistd.h>
#include "../include/hardware.h"
#include "../include/ipc.h"

int main() {
    printf("Starting EC controller...\n");

    printf("Starting socket...\n");
    int sockfd = init_ipc();

    // test: force fan speed to X% at startup
    // // can be moved in while loop to force fan speed to X% at runtime
    // set_fan_speed(X);

    // test, should be choosed by user in the GUI, but for now, we force it to manual mode at startup
    printf("Switching EC to Manual/Advanced Mode...\n");
    set_fan_mode(0x8C);

    while(1) {
        
        int temp = read_cpu_temp();
        int fan = read_fan_speed();
        int fan_time = read_fan_speed_rpm();
        
        // prevent division by zero if fan is off
        int fan_rpm = 0;
        if (fan_time > 0) {
            fan_rpm = (478000 / fan_time); 
        }
        
        printf("Temperature : %d°C\nFan Speed : %d%% || RPM : %d\n", temp, fan, fan_rpm);
        fflush(stdout);
        
        handle_ipc_client(sockfd, temp, fan, fan_rpm);
        
        set_fan_speed(80);
        // temp, to test fan speed control
        usleep(100000);
        // sleep(1); // wait for 1 second before the next reading
        // mandatory to avoid CPU overload, otherwise the program will run at 100% CPU usage
    }

    return 0;
}