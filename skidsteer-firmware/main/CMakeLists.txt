cmake_minimum_required(VERSION 3.13.0)

idf_component_register(
        SRCS
        app_main.c
        INCLUDE_DIRS .
        REQUIRES
        freertos
        nvs_flash
        button
)
