#ifndef WEIGHT_SENSOR_H
#define WEIGHT_SENSOR_H

#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>
#include "sensors/sensor_manager.h"

typedef struct {
    float weight;
    bool valid;
} weight_data_t;

esp_err_t weight_sensor_init(void);
esp_err_t weight_sensor_read(float *weight);
esp_err_t weight_sensor_read_all(void);
weight_data_t weight_sensor_get_data(void);
sensor_data_t* get_weight_sensors(uint8_t *count);
esp_err_t weight_sensor_tare(void);
esp_err_t weight_sensor_calibrate(float known_weight);

#endif
