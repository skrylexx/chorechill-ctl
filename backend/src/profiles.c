#include <stdio.h>
#include <stdint.h>
#include "../include/profiles.h"
#include "../include/hardware.h"

int apply_profile_from_string(const char* payload) {
    uint8_t temps[6];
    uint8_t speeds[7];

    // we expect exactly 6 temps and 7 speeds separated by a semicolon
    // example payload: "55,64,73,76,82,88;38,43,48,54,60,70,85"
    // %hhu is used to parse directly into an unsigned char (uint8_t)
    int parsed = sscanf(payload, "%hhu,%hhu,%hhu,%hhu,%hhu,%hhu;%hhu,%hhu,%hhu,%hhu,%hhu,%hhu,%hhu",
        &temps[0], &temps[1], &temps[2], &temps[3], &temps[4], &temps[5],
        &speeds[0], &speeds[1], &speeds[2], &speeds[3], &speeds[4], &speeds[5], &speeds[6]);

    // check if we successfully extracted all 13 values
    if (parsed == 13) {
        printf("\n[PROFILES] Valid profile received. Injecting into EC...\n");
        apply_custom_curve(temps, speeds);
        return 1;
    } else {
        printf("\n[PROFILES] Error: Invalid payload format (%d/13 parsed)\n", parsed);
        return 0;
    }
}