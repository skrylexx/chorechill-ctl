#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include "../include/hardware.h"

#define EC_FILE "/sys/kernel/debug/ec/ec0/io"
#define ADDR_CPU_TEMP 0x68
#define ADDR_FAN_SPEED 0x71

// private
static int read_ec_byte(off_t address) {
    int fd = open(EC_FILE, O_RDONLY);
    if (fd == -1) { return -1; }
    
    // start reading from the specified address
    if (lseek(fd, address, SEEK_SET) == -1) {
        close(fd);
        return -1;
    }
    
    uint8_t byte_value;
    if (read(fd, &byte_value, 1) != 1) {
        close(fd);
        return -1;
    }
    
    close(fd);
    return byte_value;
}

// public functions
int read_cpu_temp() {
    return read_ec_byte(ADDR_CPU_TEMP);
}

int read_fan_speed() {
    return read_ec_byte(ADDR_FAN_SPEED);
}