cmake_minimum_required(VERSION 3.5)

idf_component_register(
    SRCS "src/config.c"
    INCLUDE_DIRS "include"
    REQUIRES driver nvs_flash log esp_system freertos
)
