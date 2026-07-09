#include <stdio.h>
#include <unistd.h> // for sleep()
#include "../include/hardware.h"

int main() {
    printf("Démarrage du moniteur de température...\n");

    while(1) {
        int temp = read_cpu_temp();
        if (temp != -1) {
            printf("Actual temperature : %d°C\n", temp);
        }
        sleep(2);
    }

    return 0;
}