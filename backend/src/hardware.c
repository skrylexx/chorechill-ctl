#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include "../include/hardware.h"

#define EC_FILE "/sys/kernel/debug/ec/ec0/io"
#define ADDR_CPU_TEMP 0x68
#define ADDR_FAN_SPEED 0x71
#define ADDR_FAN_SPEED_RPM 0xCC

// ==========================================
// PRIVATE FUNCTIONS
// ==========================================

static int read_ec_byte(off_t address) {
    int fd = open(EC_FILE, O_RDONLY);
    if (fd == -1) return -1;
    if (lseek(fd, address, SEEK_SET) == -1) { close(fd); return -1; }
    
    uint8_t byte_value;
    if (read(fd, &byte_value, 1) != 1) { close(fd); return -1; }
    
    close(fd);
    return byte_value;
}

static int read_ec_word(off_t address) {
    int fd = open(EC_FILE, O_RDONLY);
    if (fd == -1) return -1;
    if (lseek(fd, address, SEEK_SET) == -1) { close(fd); return -1; }

    uint8_t bytes[2];
    if (read(fd, bytes, 2) != 2) { close(fd); return -1; }

    int raw_value = (bytes[0] << 8) | bytes[1];
    close(fd);
    return raw_value;
}

static int write_ec_byte(off_t address, uint8_t value) {
    int fd = open(EC_FILE, O_RDWR);
    if (fd == -1) return -1;
    if (lseek(fd, address, SEEK_SET) == -1) { close(fd); return -1; }
    if (write(fd, &value, 1) != 1) { close(fd); return -1; }

    close(fd);
    return 0;
}

// ==========================================
// PUBLIC FUNCTIONS
// ==========================================

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
    write_ec_byte(0xF4, mode); 
}

// lock the fan at a fixed percentage (0–100) by writing directly to the EC register
void set_fan_speed_percent(int percent) {
    // clamp input, then convert to the EC's 0–100 scale (it already speaks percent)
    if (percent < 0)   percent = 0;
    if (percent > 100) percent = 100;
    write_ec_byte(ADDR_FAN_SPEED, (uint8_t)percent);
}

// write the full arrays to the EC memory
void apply_custom_curve(uint8_t temps[6], uint8_t speeds[7]) {
    // write the 6 temperature thresholds (from 0x6A to 0x6F)
    for (int i = 0; i < 6; i++) {
        write_ec_byte(0x6A + i, temps[i]);
    }
    
    // write the 7 fan speed percentages (from 0x72 to 0x78)
    for (int i = 0; i < 7; i++) {
        write_ec_byte(0x72 + i, speeds[i]);
    }
}