#ifndef HARDWARE_H
#define HARDWARE_H

#include <stdint.h>

// read functions
int read_cpu_temp();
int read_fan_speed();
int read_fan_speed_rpm();

// set functions
int set_fan_speed(uint8_t speed_percent);

// del functions

// modify functions

#endif