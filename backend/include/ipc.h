#ifndef IPC_H
#define IPC_H

int  init_ipc();
void handle_ipc_client(int sockfd, int temp, int fan, int rpm);

// returns the current manual speed setpoint (-1 = no override, curve mode is active)
int  ipc_get_manual_speed();

// clears the manual override (called when a SET_CURVE profile is applied)
void ipc_clear_manual_speed();

#endif