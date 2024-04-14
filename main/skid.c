#include "skid.h"
#include "sdkconfig.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_sleep.h>
#include <driver/ledc.h>
#include <driver/gpio.h>
#include <iot_servo.h>

const char *TAG = "skid";

// Config
const gpio_num_t SKID_POWER_PIN = CONFIG_SKID_POWER_PIN;

const ledc_mode_t SKID_MOTOR_TIMER_SPEED_MODE = 0; // 0 is HIGH_SPEED_MODE, if available, otherwise its LOW_SPEED_MODE
const ledc_timer_t SKID_MOTOR_TIMER = LEDC_TIMER_0;
const uint32_t SKID_MOTOR_PWM_FREQ = CONFIG_SKID_MOTOR_PWM_FREQ;
const ledc_channel_t SKID_MOTOR_LEFT_A_CHANNEL = LEDC_CHANNEL_0;
const gpio_num_t SKID_MOTOR_LEFT_A_PIN = CONFIG_SKID_MOTOR_LEFT_A_PIN;
const ledc_channel_t SKID_MOTOR_LEFT_B_CHANNEL = LEDC_CHANNEL_1;
const gpio_num_t SKID_MOTOR_LEFT_B_PIN = CONFIG_SKID_MOTOR_LEFT_B_PIN;
const ledc_channel_t SKID_MOTOR_RIGHT_A_CHANNEL = LEDC_CHANNEL_2;
const gpio_num_t SKID_MOTOR_RIGHT_A_PIN = CONFIG_SKID_MOTOR_RIGHT_A_PIN;
const ledc_channel_t SKID_MOTOR_RIGHT_B_CHANNEL = LEDC_CHANNEL_3;
const gpio_num_t SKID_MOTOR_RIGHT_B_PIN = CONFIG_SKID_MOTOR_RIGHT_B_PIN;
const ledc_channel_t SKID_MOTOR_ARM_A_CHANNEL = LEDC_CHANNEL_4;
const gpio_num_t SKID_MOTOR_ARM_A_PIN = CONFIG_SKID_MOTOR_ARM_A_PIN;
const ledc_channel_t SKID_MOTOR_ARM_B_CHANNEL = LEDC_CHANNEL_5;
const gpio_num_t SKID_MOTOR_ARM_B_PIN = CONFIG_SKID_MOTOR_ARM_B_PIN;

const ledc_timer_t SKID_SERVO_TIMER = LEDC_TIMER_1;
const ledc_channel_t SKID_SERVO_BUCKET_CHANNEL = LEDC_CHANNEL_6;
const gpio_num_t SKID_SERVO_BUCKET_PIN = CONFIG_SKID_SERVO_BUCKET_PIN;
const ledc_channel_t SKID_SERVO_AUX_CHANNEL = LEDC_CHANNEL_7;
const gpio_num_t SKID_SERVO_AUX_PIN = CONFIG_SKID_SERVO_AUX_PIN;

// Config functions
static ledc_timer_config_t motor_timer_config() {
    return (ledc_timer_config_t) {
            .speed_mode = SKID_MOTOR_TIMER_SPEED_MODE,
            .timer_num  = SKID_MOTOR_TIMER,
            .duty_resolution = LEDC_TIMER_12_BIT,
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

static uint32_t ledc_max_value(ledc_timer_bit_t resolution) {
    return (1 << resolution) - 1;
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

static void power_supply(bool enabled) {
    if (enabled) {
        ESP_ERROR_CHECK(gpio_set_direction(SKID_POWER_PIN, GPIO_MODE_OUTPUT));
        ESP_ERROR_CHECK(gpio_set_level(SKID_POWER_PIN, 1));
    } else {
        ESP_LOGI(TAG, "Powering off");
        vTaskDelay(100 / portTICK_PERIOD_MS);
        ESP_ERROR_CHECK(gpio_set_direction(SKID_POWER_PIN, GPIO_MODE_INPUT));
        esp_deep_sleep_start();
    }
}

// Init
void skid_init() {
    // First enable power supply
    power_supply(true);
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
