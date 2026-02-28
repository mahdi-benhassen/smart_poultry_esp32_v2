#ifndef DHT22_H
#define DHT22_H

#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>
#include "sensors/sensor_manager.h"

typedef struct {
    float temperature;
    float humidity;
    bool valid;
} dht22_data_t;

esp_err_t dht22_init(void);
esp_err_t dht22_read(float *temperature, float *humidity);
esp_err_t dht22_read_all(void);
dht22_data_t dht22_get_data(void);
sensor_data_t* get_dht22_sensors(uint8_t *count);

#endif
