#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h> // required for file permissions
#include <fcntl.h> // required to use fcntl() for non-blocking socket
#include <errno.h> // required to read the error type (EAGAIN)
#include "../include/ipc.h"
#include "../include/hardware.h"

#define SOCKET_PATH "/tmp/chorechill-ctl.sock"

int init_ipc() {
    int sockfd;
    struct sockaddr_un server_addr;

    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    unlink(SOCKET_PATH);

    memset(&server_addr, 0, sizeof(struct sockaddr_un));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);

    if (bind(sockfd, (struct sockaddr *) &server_addr, sizeof(struct sockaddr_un)) < 0) {
        perror("Socket bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    if (chmod(SOCKET_PATH, 0666) < 0) {
        perror("Error changing socket permissions");
    }

    if (listen(sockfd, 5) < 0) {
        perror("Socket listen failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // set the socket to non-blocking mode
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

    printf("IPC Server started on %s\n", SOCKET_PATH);
    return sockfd;
}

void handle_ipc_client(int sockfd, int temp, int fan, int rpm) {
    // accept() is a blocking call, but since the socket is set to non-blocking mode, it will return immediately if no client is trying to connect.
    int client_fd = accept(sockfd, NULL, NULL);
    
    if (client_fd < 0) {
        // if the error is EAGAIN (or EWOULDBLOCK), it just means "No one is connected".
        // this is normal behavior, we exit the function in silence to let the while(1) loop continue !
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return; 
        } else {
            perror("Unexpected accept error");
            return;
        }
    }

    // read the request from the client (even if we don't use it, it's a good practice to read it)
    char buffer[256];
    memset(buffer, 0, sizeof(buffer));

    // read the command sent by the client
    int bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
    
    if (bytes_read > 0) {
        // analyze the command received from the client
        if (strncmp(buffer, "SET_DEFAULT", 11) == 0) {
            printf("\n[IPC] Commande reçue : Restauration de la courbe d'usine.\n");
            reset_default_fan_curve();
            
        } else if (strncmp(buffer, "SET_BOOST", 9) == 0) {
            printf("\n[IPC] Commande reçue : Activation du profil Boost.\n");
            set_custom_fan_curve();
        } 
        // if it's just a "GET", we don't do anything special, we go directly to the response
    }
    
    // prepare the response string with the temperature, fan speed, and RPM values
    char response[256];
    snprintf(response, sizeof(response), "%d,%d,%d", temp, fan, rpm);
    write(client_fd, response, strlen(response));

    close(client_fd);
}