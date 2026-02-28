#ifndef MQ_SENSOR_H
#define MQ_SENSOR_H

#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>
#include "sensors/sensor_manager.h"

typedef enum {
    MQ_TYPE_MQ2,
    MQ_TYPE_MQ3,
    MQ_TYPE_MQ4,
    MQ_TYPE_MQ5,
    MQ_TYPE_MQ6,
    MQ_TYPE_MQ7,
    MQ_TYPE_MQ8,
    MQ_TYPE_MQ9,
    MQ_TYPE_MQ135
} mq_sensor_type_t;

typedef struct {
    float ammonia;
    float co2;
    float co;
    float methane;
    float lpg;
    bool valid;
} mq_sensor_data_t;

esp_err_t mq_sensor_init(void);
esp_err_t mq_sensor_read(float *ammonia, float *co2, float *co);
esp_err_t mq_sensor_read_all(void);
mq_sensor_data_t mq_sensor_get_data(void);
sensor_data_t* get_mq_sensors(uint8_t *count);
esp_err_t mq_sensor_calibrate(mq_sensor_type_t type, float clean_air_r0);

#endif
