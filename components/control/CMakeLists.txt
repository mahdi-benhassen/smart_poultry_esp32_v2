cmake_minimum_required(VERSION 3.5)

idf_component_register(
    SRCS "src/control_system.c"
    INCLUDE_DIRS "include"
    REQUIRES log esp_system freertos sensors actuators utils
)
