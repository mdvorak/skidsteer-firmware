#include "skid.h"
#include "sdkconfig.h"
#include <esp_log.h>
#include <esp_sleep.h>
#include <driver/ledc.h>
#include <iot_servo.h>

static const char *TAG = "skid_motion";
#define LEDC_MAX_VALUE(resolution) ((1 << resolution) - 1)
#define MAX(a,b) ((a) > (b) ? (a) : (b))

// Config
static const ledc_mode_t SKID_MOTOR_TIMER_SPEED_MODE = 0; // 0 is HIGH_SPEED_MODE, if available, otherwise its LOW_SPEED_MODE
static const ledc_timer_t SKID_MOTOR_TIMER = LEDC_TIMER_0;
static const ledc_timer_bit_t SKID_MOTOR_RESOLUTION = LEDC_TIMER_12_BIT;
static const float SKID_MOTOR_PWM_MAX = LEDC_MAX_VALUE(SKID_MOTOR_RESOLUTION);
static const uint32_t SKID_MOTOR_PWM_FREQ = CONFIG_SKID_MOTOR_PWM_FREQ;
static const ledc_channel_t SKID_MOTOR_LEFT_A_CHANNEL = LEDC_CHANNEL_0;
static const gpio_num_t SKID_MOTOR_LEFT_A_PIN = CONFIG_SKID_MOTOR_LEFT_A_PIN;
static const ledc_channel_t SKID_MOTOR_LEFT_B_CHANNEL = LEDC_CHANNEL_1;
static const gpio_num_t SKID_MOTOR_LEFT_B_PIN = CONFIG_SKID_MOTOR_LEFT_B_PIN;
static const ledc_channel_t SKID_MOTOR_RIGHT_A_CHANNEL = LEDC_CHANNEL_2;
static const gpio_num_t SKID_MOTOR_RIGHT_A_PIN = CONFIG_SKID_MOTOR_RIGHT_A_PIN;
static const ledc_channel_t SKID_MOTOR_RIGHT_B_CHANNEL = LEDC_CHANNEL_3;
static const gpio_num_t SKID_MOTOR_RIGHT_B_PIN = CONFIG_SKID_MOTOR_RIGHT_B_PIN;
static const ledc_channel_t SKID_MOTOR_ARM_A_CHANNEL = LEDC_CHANNEL_4;
static const gpio_num_t SKID_MOTOR_ARM_A_PIN = CONFIG_SKID_MOTOR_ARM_A_PIN;
static const ledc_channel_t SKID_MOTOR_ARM_B_CHANNEL = LEDC_CHANNEL_5;
static const gpio_num_t SKID_MOTOR_ARM_B_PIN = CONFIG_SKID_MOTOR_ARM_B_PIN;

static const ledc_timer_t SKID_SERVO_TIMER = LEDC_TIMER_1;
static const ledc_channel_t SKID_SERVO_BUCKET_CHANNEL = LEDC_CHANNEL_6;
static const gpio_num_t SKID_SERVO_BUCKET_PIN = CONFIG_SKID_SERVO_BUCKET_PIN;
static const ledc_channel_t SKID_SERVO_AUX_CHANNEL = LEDC_CHANNEL_7;
static const gpio_num_t SKID_SERVO_AUX_PIN = CONFIG_SKID_SERVO_AUX_PIN;


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
            .min_width_us = 1000,
            .max_width_us = 2000,
            .freq = 50,
            .timer_number = SKID_SERVO_TIMER,
            .channel_number = 2,
            .channels = {
                    .ch = {SKID_SERVO_BUCKET_CHANNEL, SKID_SERVO_AUX_CHANNEL},
                    .servo_pin = {SKID_SERVO_BUCKET_PIN, SKID_SERVO_AUX_PIN},
            }
    };
}

// API
void skid_motion_init() {
    ESP_LOGI(TAG, "init");

    // Configure motors PWM
    ledc_timer_config_t motorTimerConfig = motor_timer_config();
    ESP_ERROR_CHECK(ledc_timer_config(&motorTimerConfig));

    ESP_ERROR_CHECK(motor_channel_config(SKID_MOTOR_LEFT_A_CHANNEL, SKID_MOTOR_LEFT_A_PIN));
    ESP_ERROR_CHECK(motor_channel_config(SKID_MOTOR_LEFT_B_CHANNEL, SKID_MOTOR_LEFT_B_PIN));
    ESP_ERROR_CHECK(motor_channel_config(SKID_MOTOR_RIGHT_A_CHANNEL, SKID_MOTOR_RIGHT_A_PIN));
    ESP_ERROR_CHECK(motor_channel_config(SKID_MOTOR_RIGHT_B_CHANNEL, SKID_MOTOR_RIGHT_B_PIN));
    ESP_ERROR_CHECK(motor_channel_config(SKID_MOTOR_ARM_A_CHANNEL, SKID_MOTOR_ARM_A_PIN));
    ESP_ERROR_CHECK(motor_channel_config(SKID_MOTOR_ARM_B_CHANNEL, SKID_MOTOR_ARM_B_PIN));

    // Servo config
    servo_config_t servoConfig = servo_config();
    ESP_ERROR_CHECK(iot_servo_init(LEDC_LOW_SPEED_MODE, &servoConfig));

    ESP_LOGI(TAG, "configured");
}

static void motor_channel_set(ledc_channel_t channel, uint32_t duty) {
    ESP_ERROR_CHECK(ledc_set_duty(SKID_MOTOR_TIMER_SPEED_MODE, channel, MAX(duty, SKID_MOTOR_PWM_MAX)));
    ESP_ERROR_CHECK(ledc_update_duty(SKID_MOTOR_TIMER_SPEED_MODE, channel));
}

static void motor_set(ledc_channel_t channelA, ledc_channel_t channelB, float dutyPercent) {
    int32_t dutyValue = (int32_t) (dutyPercent * SKID_MOTOR_PWM_MAX);

    if (dutyValue < 0) {
        motor_channel_set(channelA, 0);
        motor_channel_set(channelB, abs(dutyValue));
    } else {
        motor_channel_set(channelB, 0);
        motor_channel_set(channelA, dutyValue);
    }
}
