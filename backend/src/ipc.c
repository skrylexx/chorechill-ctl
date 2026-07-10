#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include "../include/ipc.h"
#include "../include/profiles.h" // we route the payload to profiles.c

#define SOCKET_PATH "/tmp/chorechill-ctl.sock"

int init_ipc() {
    int sockfd;
    struct sockaddr_un server_addr;

    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0) exit(EXIT_FAILURE);

    unlink(SOCKET_PATH);

    memset(&server_addr, 0, sizeof(struct sockaddr_un));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);

    if (bind(sockfd, (struct sockaddr *) &server_addr, sizeof(struct sockaddr_un)) < 0) {
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    chmod(SOCKET_PATH, 0666);

    if (listen(sockfd, 5) < 0) {
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

    printf("IPC Server started on %s\n", SOCKET_PATH);
    return sockfd;
}

void handle_ipc_client(int sockfd, int temp, int fan, int rpm) {
    int client_fd = accept(sockfd, NULL, NULL);
    
    if (client_fd < 0) return;

    char buffer[256];
    memset(buffer, 0, sizeof(buffer));
    
    int bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
    
    if (bytes_read > 0) {
        // Route SET_CURVE to the profile parser
        if (strncmp(buffer, "SET_CURVE:", 10) == 0) {
            // buffer + 10 skips the "SET_CURVE:" part and sends the rest
            apply_profile_from_string(buffer + 10);
        }
    }

    char response[256];
    snprintf(response, sizeof(response), "%d,%d,%d", temp, fan, rpm);
    write(client_fd, response, strlen(response));

    close(client_fd);
}