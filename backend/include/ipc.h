#ifndef IPC_H
#define IPC_H

#include <stdint.h>

int init_ipc();
void handle_ipc_client(int socketfd, int temp, int fan, int rpm);

void set_default_fan_curve();
void set_silent_fan_curve();
void set_gaming_fan_curve();
void set_custom_fan_curve();


#endif