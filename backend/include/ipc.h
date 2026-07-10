#ifndef IPC_H
#define IPC_H

#include <stdint.h>

int init_ipc();
void handle_ipc_client(int socketfd, int temp, int fan, int rpm);

void set_custom_fan_curve();
void reset_default_fan_curve();

#endif