#include <stdio.h>
#include <unistd.h>
#include "../include/hardware.h"

int main() {
    printf("Starting EC controller...\n");

    // test: force fan speed to X% at startup
    // // can be moved in while loop to force fan speed to X% at runtime
    // set_fan_speed(X);

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
        
        // 1 second pause
        sleep(1); 
    }

    return 0;
}