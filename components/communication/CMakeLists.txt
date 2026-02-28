cmake_minimum_required(VERSION 3.5)

idf_component_register(
    SRCS "src/communication.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_wifi esp_event sensors actuators utils mesh
)
