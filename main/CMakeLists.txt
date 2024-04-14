cmake_minimum_required(VERSION 3.21.0)

set(CMAKE_C_STANDARD 23)
set(CMAKE_CXX_STANDARD 23)

idf_component_register(
        SRCS
        app_main.c
        my_platform.c
        skid_power.c
        skid_motion.c
        skid_leds.c

        INCLUDE_DIRS .

        REQUIRES
        freertos
        nvs_flash
        button
        bluepad32
        btstack
        console
        driver
        servo
)
