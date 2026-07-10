#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <cjson/cJSON.h>

#include "../include/profiles.h"

uint8_t *json_get_fan_curve(const char *profile) {
    // read the JSON file containing the fan curves
    FILE *file = fopen("../../config/profiles.json", "r");
    if (!file) {
        perror("Failed to open profiles.json");
        return NULL;
    }

    
    

}