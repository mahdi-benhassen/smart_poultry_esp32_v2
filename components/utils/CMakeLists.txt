cmake_minimum_required(VERSION 3.5)

idf_component_register(
    SRCS "src/config.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_idf_lib_helpers driver esp_adc_cal nvs_flash esp_log esp_system
)
