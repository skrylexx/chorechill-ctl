#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include "../include/hardware.h"

// base definitions for the Embedded Controller
#define EC_FILE "/sys/kernel/debug/ec/ec0/io"
#define ADDR_CPU_TEMP 0x68
#define ADDR_FAN_SPEED 0x71
#define ADDR_FAN_SPEED_RPM 0xCC

// --- PRIVATE FUNCTIONS (Internal EC operations) ---

// read a single byte from the EC at the specified address
static int read_ec_byte(off_t address) {
    int fd = open(EC_FILE, O_RDONLY);
    if (fd == -1) { 
        return -1;
    }
    
    // start reading from the specified address
    if (lseek(fd, address, SEEK_SET) == -1) {
        close(fd);
        return -1;
    }
    
    // read a single byte from the specified address
    uint8_t byte_value;
    if (read(fd, &byte_value, 1) != 1) {
        close(fd);
        return -1;
    }
    
    close(fd);
    return byte_value;
}

// read a 2-byte word from the EC at the specified address (for fan speed in RPM)
static int read_ec_word(off_t address) {
    int fd = open(EC_FILE, O_RDONLY);
    if (fd == -1) { 
        return -1;
    }

    // start reading from the specified address
    if (lseek(fd, address, SEEK_SET) == -1) {
        close(fd);
        return -1;
    }

    // start reading from specified address, with block of 2 bytes
    uint8_t bytes[2];
    if (read(fd, bytes, 2) != 2) {
        close(fd);
        return -1;
    }

    // logical operation to combine the two bytes into a single integer value (Big-Endian interpretation)
    int valeur_brute = (bytes[0] << 8) | bytes[1];

    close(fd);
    return valeur_brute;
}

// write a single byte to the EC at the specified address
static int write_ec_byte(off_t address, uint8_t value) {
    // open with read and write permissions
    int fd = open(EC_FILE, O_RDWR);
    if (fd == -1) {
        return -1;
    }

    // start writing at the specified address
    if (lseek(fd, address, SEEK_SET) == -1) {
        close(fd);
        return -1;
    }

    // inject the new byte value
    if (write(fd, &value, 1) != 1) {
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}


// --- PUBLIC FUNCTIONS (API for main.c) ---

int read_cpu_temp() {
    return read_ec_byte(ADDR_CPU_TEMP);
}

int read_fan_speed() {
    return read_ec_byte(ADDR_FAN_SPEED);
}

int read_fan_speed_rpm() {
    return read_ec_word(ADDR_FAN_SPEED_RPM);
}

void set_fan_mode(int mode) {
    // 0xF4 is the common register for user scenarios on MSI laptops
    write_ec_byte(0xF4, mode); // to update later with smth like "fanmode.json"
}


// overwrite the factory fan curve in the EC memory
void set_custom_fan_curve() {
    // addresses 0x72 to 0x78 hold the 7 fan speed percentages for each temperature step.
    // we overwrite all of them with 80% to lock the fan speed cleanly.
    for (int addr = 0x72; addr <= 0x78; addr++) {
        write_ec_byte(addr, 80); 
    }
}

// for test purposes
// restore the factory fan curve in the EC memory
void reset_default_fan_curve() {

    char done_msg[256];
    uint8_t default_speeds[] = {38, 43, 48, 54, 60, 70, 85};
    
    int index = 0;
    for (int addr = 0x72; addr <= 0x78; addr++) {
        write_ec_byte(addr, default_speeds[index]);
        index++;
    }
}