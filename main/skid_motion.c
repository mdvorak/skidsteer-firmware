#include "skid.h"
#include "sdkconfig.h"
#include <esp_log.h>
#include <esp_sleep.h>
#include <driver/ledc.h>
#include <iot_servo.h>

static const char *TAG = "skid_motion";
#define LEDC_MAX_VALUE(resolution) ((1 << resolution) - 1)
#define MIN(a, b) ((a) < (b) ? (a) : (b))

// Motors
static const ledc_mode_t SKID_MOTOR_TIMER_SPEED_MODE = 0; // 0 is HIGH_SPEED_MODE, if available, otherwise its LOW_SPEED_MODE
static const ledc_timer_t SKID_MOTOR_TIMER = LEDC_TIMER_0;
static const ledc_timer_bit_t SKID_MOTOR_RESOLUTION = LEDC_TIMER_12_BIT;
static const uint32_t SKID_MOTOR_PWM_FREQ = CONFIG_SKID_MOTOR_PWM_FREQ;
static const uint32_t SKID_MOTOR_PWM_MAX = LEDC_MAX_VALUE(SKID_MOTOR_RESOLUTION);
static const uint32_t SKID_MOTOR_PWM_MIN = SKID_MOTOR_PWM_MAX / 3;

typedef struct skid_motor_channel {
    ledc_channel_t channel;
    gpio_num_t pin;
} skid_motor_channel_t;

typedef struct skid_motor {
    skid_motor_channel_t a;
    skid_motor_channel_t b;
} skid_motor_t;

const double SKID_MOTOR_HOLD = 9.0;
const skid_motor_t SKID_MOTOR_LEFT = {
        .a = {LEDC_CHANNEL_0, (CONFIG_SKID_MOTOR_LEFT_A_PIN)},
        .b = {LEDC_CHANNEL_1, (CONFIG_SKID_MOTOR_LEFT_B_PIN)},
};
const skid_motor_t SKID_MOTOR_RIGHT = {
        .a = {LEDC_CHANNEL_2, (CONFIG_SKID_MOTOR_RIGHT_A_PIN)},
        .b = {LEDC_CHANNEL_3, (CONFIG_SKID_MOTOR_RIGHT_B_PIN)},
};
const skid_motor_t SKID_MOTOR_ARM = {
        .a = {LEDC_CHANNEL_4, (CONFIG_SKID_MOTOR_ARM_A_PIN)},
        .b = {LEDC_CHANNEL_5, (CONFIG_SKID_MOTOR_ARM_B_PIN)},
};
static const skid_motor_t *SKID_MOTORS[] = {&SKID_MOTOR_LEFT, &SKID_MOTOR_RIGHT, &SKID_MOTOR_ARM};

// Servos
static const ledc_mode_t SKID_SERVO_SPEED_MODE = LEDC_LOW_SPEED_MODE;
static const ledc_timer_t SKID_SERVO_TIMER = LEDC_TIMER_1;
static const uint16_t SKID_SERVO_FREQ = 50;
static const uint16_t SKID_SERVO_MIN_WIDTH_US = 1000;
static const uint16_t SKID_SERVO_MAX_WIDTH_US = 2000;

typedef struct skid_servo {
    ledc_channel_t channel;
    gpio_num_t pin;
} skid_servo_t;

const skid_servo_t SKID_SERVO_BUCKET = {
        .channel = LEDC_CHANNEL_6,
        .pin = (CONFIG_SKID_SERVO_BUCKET_PIN),
};
const skid_servo_t SKID_SERVO_AUX = {
        .channel = LEDC_CHANNEL_7,
        .pin = (CONFIG_SKID_SERVO_AUX_PIN),
};


// Config functions
static ledc_timer_config_t motor_timer_config() {
    return (ledc_timer_config_t) {
            .speed_mode = SKID_MOTOR_TIMER_SPEED_MODE,
            .timer_num  = SKID_MOTOR_TIMER,
            .duty_resolution = SKID_MOTOR_RESOLUTION,
            .freq_hz = SKID_MOTOR_PWM_FREQ,
            .clk_cfg = LEDC_AUTO_CLK,
    };
}

static esp_err_t motor_channel_config(ledc_channel_t channel, gpio_num_t gpio_num) {
    ledc_channel_config_t channel_conf = {
            .channel    = channel,
            .duty       = 0,
            .hpoint     = 0,
            .gpio_num   = gpio_num,
            .speed_mode = SKID_MOTOR_TIMER_SPEED_MODE,
            .timer_sel  = SKID_MOTOR_TIMER,
    };
    return ledc_channel_config(&channel_conf);
}

static servo_config_t servo_config() {
    return (servo_config_t) {
            .max_angle = 180,
            .min_width_us = SKID_SERVO_MIN_WIDTH_US,
            .max_width_us = SKID_SERVO_MAX_WIDTH_US,
            .freq = SKID_SERVO_FREQ,
            .timer_number = SKID_SERVO_TIMER,
            .channel_number = 2,
            .channels = {
                    .ch = {SKID_SERVO_BUCKET.channel, SKID_SERVO_AUX.channel},
                    .servo_pin = {SKID_SERVO_BUCKET.pin, SKID_SERVO_AUX.pin},
            }
    };
}

// API
void skid_motion_init() {
    ESP_LOGI(TAG, "init");

    // Configure motors PWM
    ledc_timer_config_t motorTimerConfig = motor_timer_config();
    ESP_ERROR_CHECK(ledc_timer_config(&motorTimerConfig));

    for (size_t i = 0; i < sizeof(SKID_MOTORS) / sizeof(SKID_MOTORS[0]); i++) {
        const skid_motor_t *motor = SKID_MOTORS[i];
        ESP_ERROR_CHECK(motor_channel_config(motor->a.channel, motor->a.pin));
        ESP_ERROR_CHECK(motor_channel_config(motor->b.channel, motor->b.pin));
    }

    // Servo config
    servo_config_t servoConfig = servo_config();
    ESP_ERROR_CHECK(iot_servo_init(SKID_SERVO_SPEED_MODE, &servoConfig));

    ESP_LOGI(TAG, "configured");
}

static void skid_motor_channel_set(ledc_channel_t channel, uint32_t duty) {
    ESP_ERROR_CHECK(ledc_set_duty(SKID_MOTOR_TIMER_SPEED_MODE, channel, MIN(duty, SKID_MOTOR_PWM_MAX)));
    ESP_ERROR_CHECK(ledc_update_duty(SKID_MOTOR_TIMER_SPEED_MODE, channel));
}

void skid_motor_set(const skid_motor_t *motor, double dutyPercent) {
    if (dutyPercent == SKID_MOTOR_HOLD) {
        // Hold
        skid_motor_channel_set(motor->a.channel, SKID_MOTOR_PWM_MAX);
        skid_motor_channel_set(motor->b.channel, SKID_MOTOR_PWM_MAX);
        return;
    }

    int32_t dutyValue = (int32_t) (dutyPercent * SKID_MOTOR_PWM_MAX);

    if (abs(dutyValue) < SKID_MOTOR_PWM_MIN) {
        // Stop
        skid_motor_channel_set(motor->a.channel, 0);
        skid_motor_channel_set(motor->b.channel, 0);
    } else if (dutyValue < 0) {
        // Reverse
        skid_motor_channel_set(motor->a.channel, 0);
        skid_motor_channel_set(motor->b.channel, abs(dutyValue));
    } else {
        // Forward
        skid_motor_channel_set(motor->b.channel, 0);
        skid_motor_channel_set(motor->a.channel, dutyValue);
    }
}

void skid_servo_set(const skid_servo_t *servo, float angle) {
    ESP_ERROR_CHECK(iot_servo_write_angle(SKID_SERVO_SPEED_MODE, servo->channel, angle));
}
