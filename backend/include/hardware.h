#ifndef HARDWARE_H
#define HARDWARE_H

#include <stdint.h>

// --- READ FUNCTIONS ---
int read_cpu_temp();
int read_fan_speed();
int read_fan_speed_rpm();

// --- SET FUNCTIONS ---
void set_fan_mode(int mode);
void set_fan_speed_percent(int percent);  // lock fan at a fixed % (0–100)

// inject 6 temperatures and 7 speeds directly into the EC memory
void apply_custom_curve(uint8_t temps[6], uint8_t speeds[7]);

#endif