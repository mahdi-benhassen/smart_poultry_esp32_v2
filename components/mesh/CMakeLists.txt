cmake_minimum_required(VERSION 3.5)

idf_component_register(
    SRCS "src/mesh_manager.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_wifi esp_event esp_mesh log esp_system freertos
)
