cmake_minimum_required(VERSION 3.5)

idf_component_register(
    SRCS "src/actuator_manager.c"
    INCLUDE_DIRS "include"
    REQUIRES driver esp_log esp_system nvs_flash
)
