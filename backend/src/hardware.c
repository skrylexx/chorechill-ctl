#include "../include/hardware.h"
#include <fcntl.h>   // for O_RDONLY
#include <unistd.h>  // for read, close
#include <stdio.h>
#include <stdlib.h>

// will have to replace with good file, here is chore temp for GF63 Thin mo-bo
#define TEMP_FILE "/sys/class/hwmon/hwmon4/temp1_input"

int read_cpu_temp() {
    int fd = open(TEMP_FILE, O_RDONLY);
    if (fd == -1) {
        perror("Can't open temp file");
        return -1;
    }

    char buffer[16];
    // read raw file
    ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1);
    close(fd);

    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        int temp_millidegrees = atoi(buffer); // convert string to int
        return temp_millidegrees / 1000;
    }

    return -1;
}

int read_fan_speed() {
    int fd = open("/sys/class/hwmon/hwmon4/fan1_input", O_RDONLY);
    if (fd == -1) {
        perror("Can't open fan speed file");
        return -1;
    }

    char buffer[16];
    ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1);
    close(fd);

    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        return atoi(buffer); // convert string to int
    }

    return -1;
}