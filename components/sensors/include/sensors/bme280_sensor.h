#ifndef BME280_H
#define BME280_H

#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>
#include "sensors/sensor_manager.h"

typedef struct {
    float temperature;
    float humidity;
    float pressure;
    bool valid;
} bme280_data_t;

esp_err_t bme280_init(void);
esp_err_t bme280_read(float *temperature, float *humidity, float *pressure);
esp_err_t bme280_read_all(void);
bme280_data_t bme280_get_data(void);
sensor_data_t* get_bme280_sensors(uint8_t *count);

#endif
