#pragma once

#include <stdbool.h>
#include <hal/ledc_types.h>
#include <soc/gpio_num.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct skid_motor skid_motor_t;
typedef struct skid_servo skid_servo_t;

extern const skid_motor_t SKID_MOTOR_LEFT;
extern const skid_motor_t SKID_MOTOR_RIGHT;
extern const skid_motor_t SKID_MOTOR_ARM;
extern const skid_servo_t SKID_SERVO_BUCKET;
extern const skid_servo_t SKID_SERVO_AUX;

extern const double SKID_MOTOR_HOLD;

void skid_power_init();

void skid_power_report_activity();

void skid_power(bool enabled);

void skid_motion_init();

bool skid_motor_set(const skid_motor_t *motor, double dutyPercent);

void skid_servo_set(const skid_servo_t *servo, double dutyPercent);

void skid_leds_init();

void skids_leds_set(uint32_t red, uint32_t green, uint32_t blue);

void skid_leds_preset(uint32_t preset);

void skid_leds_cycle();

#ifdef __cplusplus
} // extern "C"
#endif
