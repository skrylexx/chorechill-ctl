#ifndef HARDWARE_H
#define HARDWARE_H

#include <stdint.h>

int read_cpu_temp();
int read_fan_speed();
int set_fan_speed(uint8_t speed_percent);

#endif