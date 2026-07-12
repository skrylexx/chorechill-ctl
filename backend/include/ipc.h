#ifndef IPC_H
#define IPC_H

#include "../include/driver_plugin.h"

/*
 * ipc.h — Unix-domain socket IPC interface for chorechill-ctl.
 *
 * The daemon listens on a SOCK_STREAM Unix socket.  The GUI and any
 * other client connect, send a command string, and receive a telemetry
 * response.  All I/O is non-blocking so the main loop never stalls.
 *
 * Supported commands:
 *   GET              — return latest telemetry (temp,fan%,rpm)
 *   SET:<pct>        — lock fan at <pct>% (0–100)
 *   SET_CURVE:<data> — apply a custom thermal curve to the EC
 */

/* Create, bind, and listen on the Unix socket.
 * Returns the server socket fd on success; exits on failure. */
int  init_ipc(void);

/*
 * handle_ipc_client() — Accept and dispatch one pending IPC connection.
 *
 * Called every 100 ms from the main loop.  Returns immediately
 * (EAGAIN/EWOULDBLOCK) when no client is waiting.
 *
 *   sockfd  — the listening server socket fd
 *   temp    — latest CPU temperature (°C)
 *   fan     — latest fan speed (%)
 *   rpm     — latest fan speed (RPM)
 *   driver  — active hardware driver plugin (for SET/SET_CURVE routing)
 */
void handle_ipc_client(int sockfd, int temp, int fan, int rpm,
                        driver_plugin_t *driver);

/* Returns the current manual speed setpoint (-1 = curve mode is active) */
int  ipc_get_manual_speed(void);

/* Clears the manual override; called when a SET_CURVE profile is applied */
void ipc_clear_manual_speed(void);

#endif /* IPC_H */