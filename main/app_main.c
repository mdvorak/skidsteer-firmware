#include <stdlib.h>
#include <btstack_port_esp32.h>
#include <btstack_run_loop.h>
#include <uni.h>
#include <driver/gpio.h>
#include "sdkconfig.h"
#include "skid.h"

// Sanity check
#ifndef CONFIG_BLUEPAD32_PLATFORM_CUSTOM
#error "Must use BLUEPAD32_PLATFORM_CUSTOM"
#endif

// Defined in my_platform.c
struct uni_platform *get_my_platform(void);

void app_main(void) {
    // Configure skid first
    skid_power(true);
    skid_motion_init();
    skid_leds_init();

    // Configure BTstack for ESP32 VHCI Controller
    btstack_init();
    uni_platform_set_custom(get_my_platform());
    uni_init(0, NULL);

    // Status led
#if defined(CONFIG_SKID_STATUS_LED_PIN) && defined(CONFIG_SKID_STATUS_LED_VALUE)
    const gpio_num_t statusLedPin = CONFIG_SKID_STATUS_LED_PIN;
    gpio_set_direction(statusLedPin, GPIO_MODE_OUTPUT);
    gpio_set_level(statusLedPin, CONFIG_SKID_STATUS_LED_VALUE);
#endif

    // Does not return.
    btstack_run_loop_execute();
}
