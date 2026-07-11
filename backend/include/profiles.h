#ifndef PROFILES_H
#define PROFILES_H

// parse the payload string and apply it to the hardware, returns 1 on success, 0 on failure
int apply_profile_from_string(const char* payload);

#endif