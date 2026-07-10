#ifndef HARDWARE_H
#define HARDWARE_H

#include <stdint.h>

// --- READ FUNCTIONS ---
int read_cpu_temp();
int read_fan_speed();
int read_fan_speed_rpm();

// --- SET FUNCTIONS ---
// kept doe the future
void set_fan_mode(int mode);

// hack to override the EC's native fan curve
void set_custom_fan_curve();
void reset_default_fan_curve();

#endif