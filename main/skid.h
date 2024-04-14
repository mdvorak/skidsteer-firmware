#pragma once
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void skid_power(bool enabled);

void skid_motion_init();

void skid_leds_init();

#ifdef __cplusplus
} // extern "C"
#endif
