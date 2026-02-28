#ifndef WATER_LEVEL_SENSOR_H
#define WATER_LEVEL_SENSOR_H

#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>
#include "sensors/sensor_manager.h"

typedef struct {
    float level;
    float percentage;
    bool valid;
} water_level_data_t;

esp_err_t water_level_sensor_init(void);
esp_err_t water_level_sensor_read(float *level, float *percentage);
esp_err_t water_level_sensor_read_all(void);
water_level_data_t water_level_sensor_get_data(void);
sensor_data_t* get_water_level_sensors(uint8_t *count);

#endif
