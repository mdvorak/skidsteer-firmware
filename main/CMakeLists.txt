cmake_minimum_required(VERSION 3.13.0)

idf_component_register(
        SRCS
        app_main.c
        my_platform.c
        INCLUDE_DIRS .
        REQUIRES
        freertos
        nvs_flash
        button
        bluepad32
        btstack
        console
        driver
)
