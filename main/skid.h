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

void skid_power(bool enabled);

void skid_motion_init();

void skid_leds_init();

void skid_motor_set(const skid_motor_t *motor, double dutyPercent);

void skid_servo_set(const skid_servo_t *servo, float angle);

#ifdef __cplusplus
} // extern "C"
#endif
