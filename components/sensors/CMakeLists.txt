cmake_minimum_required(VERSION 3.5)

idf_component_register(
    SRCS 
        "src/sensor_manager.c"
        "src/dht22.c"
        "src/mq_sensor.c"
        "src/bme280_sensor.c"
        "src/weight_sensor.c"
        "src/water_level_sensor.c"
    INCLUDE_DIRS "include"
    REQUIRES driver esp_log esp_system
)
