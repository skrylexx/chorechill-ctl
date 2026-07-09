#include <stdio.h>
#include <unistd.h>
#include "../include/hardware.h"

int main() {
    printf("Starting EC controller...\n");

    while(1) {
        int temp = read_cpu_temp();
        int fan = read_fan_speed();
        
        printf("Temperature : %d°C\nFan Speed : %d%% || RPM\n", temp, fan);
        
        sleep(1); // Pause d'une seconde
    }

    return 0;
}