#ifndef IPC_H
#define IPC_H

int init_ipc();
void handle_ipc_client(int sockfd, int temp, int fan, int rpm);

#endif