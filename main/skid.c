#include "skid.h"
#include "sdkconfig.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_sleep.h>
#include <driver/ledc.h>
#include <driver/gpio.h>

#define SKID_MOTOR_TIMER LEDC_TIMER_0
#define SKID_MOTOR_PWM_FREQ (CONFIG_SKID_MOTOR_PWM_FREQ)
#define SKID_MOTOR_LEFT_A_CHANNEL LEDC_CHANNEL_0
#define SKID_MOTOR_LEFT_A_PIN (CONFIG_SKID_MOTOR_LEFT_A_PIN)
#define SKID_MOTOR_LEFT_B_CHANNEL LEDC_CHANNEL_1
#define SKID_MOTOR_LEFT_B_PIN (CONFIG_SKID_MOTOR_LEFT_B_PIN)
#define SKID_MOTOR_RIGHT_A_CHANNEL LEDC_CHANNEL_2
#define SKID_MOTOR_RIGHT_A_PIN (CONFIG_SKID_MOTOR_RIGHT_A_PIN)
#define SKID_MOTOR_RIGHT_B_CHANNEL LEDC_CHANNEL_3
#define SKID_MOTOR_RIGHT_B_PIN (CONFIG_SKID_MOTOR_RIGHT_B_PIN)
#define SKID_MOTOR_ARM_A_CHANNEL LEDC_CHANNEL_4
#define SKID_MOTOR_ARM_A_PIN (CONFIG_SKID_MOTOR_ARM_A_PIN)
#define SKID_MOTOR_ARM_B_CHANNEL LEDC_CHANNEL_5
#define SKID_MOTOR_ARM_B_PIN (CONFIG_SKID_MOTOR_ARM_B_PIN)

const ledc_timer_config_t motor_timer = {
#if SOC_LEDC_SUPPORT_HS_MODE
        .speed_mode = LEDC_HIGH_SPEED_MODE,
#else
        .speed_mode = LEDC_LOW_SPEED_MODE,
#endif
        .timer_num  = SKID_MOTOR_TIMER,
        .duty_resolution = LEDC_TIMER_12_BIT,
        .freq_hz = SKID_MOTOR_PWM_FREQ,
        .clk_cfg = LEDC_AUTO_CLK
};

static esp_err_t motor_channel_config(ledc_channel_t channel, gpio_num_t gpio_num) {
    ledc_channel_config_t channel_conf = {
            .channel    = channel,
            .duty       = 0,
            .gpio_num   = gpio_num,
            .speed_mode = LEDC_HIGH_SPEED_MODE,
            .timer_sel  = motor_timer.timer_num,
    };
    return ledc_channel_config(&channel_conf);
}

static void power_supply(bool enabled) {
    if (enabled) {
        ESP_ERROR_CHECK(gpio_set_direction(GPIO_NUM_23, GPIO_MODE_OUTPUT));
        ESP_ERROR_CHECK(gpio_set_level(GPIO_NUM_23, 1));
    } else {
        ESP_LOGI("skid", "Powering off");
        vTaskDelay(100 / portTICK_PERIOD_MS);
        ESP_ERROR_CHECK(gpio_set_direction(GPIO_NUM_23, GPIO_MODE_INPUT));
        esp_deep_sleep_start();
    }
}

void skid_init() {
    // First enable power supply
    power_supply(true);

    // Configure motors PWM
    ESP_ERROR_CHECK(motor_channel_config(SKID_MOTOR_LEFT_A_CHANNEL, SKID_MOTOR_LEFT_A_PIN));
    ESP_ERROR_CHECK(motor_channel_config(SKID_MOTOR_LEFT_B_CHANNEL, SKID_MOTOR_LEFT_B_PIN));
    ESP_ERROR_CHECK(motor_channel_config(SKID_MOTOR_RIGHT_A_CHANNEL, SKID_MOTOR_RIGHT_A_PIN));
    ESP_ERROR_CHECK(motor_channel_config(SKID_MOTOR_RIGHT_B_CHANNEL, SKID_MOTOR_RIGHT_B_PIN));
    ESP_ERROR_CHECK(motor_channel_config(SKID_MOTOR_ARM_A_CHANNEL, SKID_MOTOR_ARM_A_PIN));
    ESP_ERROR_CHECK(motor_channel_config(SKID_MOTOR_ARM_B_CHANNEL, SKID_MOTOR_ARM_B_PIN));
}
