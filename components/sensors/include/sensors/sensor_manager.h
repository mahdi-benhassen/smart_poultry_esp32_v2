#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>
#include <esp_adc/adc_oneshot.h>

extern adc_oneshot_unit_handle_t adc1_handle;

typedef enum {
    SENSOR_TYPE_TEMPERATURE,
    SENSOR_TYPE_HUMIDITY,
    SENSOR_TYPE_PRESSURE,
    SENSOR_TYPE_AMMONIA,
    SENSOR_TYPE_CO2,
    SENSOR_TYPE_CO,
    SENSOR_TYPE_METHANE,
    SENSOR_TYPE_LIGHT,
    SENSOR_TYPE_SOUND,
    SENSOR_TYPE_WATER_LEVEL,
    SENSOR_TYPE_WEIGHT,
    SENSOR_TYPE_MOTION,
    SENSOR_TYPE_DOOR
} sensor_type_t;

typedef enum {
    SENSOR_STATUS_OK,
    SENSOR_STATUS_ERROR,
    SENSOR_STATUS_CALIBRATING,
    SENSOR_STATUS_OFFLINE
} sensor_status_t;

typedef struct {
    uint8_t id;
    char name[64];
    sensor_type_t type;
    sensor_status_t status;
    float value;
    float min_value;
    float max_value;
    float threshold_min;
    float threshold_max;
    uint32_t last_read_time;
    bool enabled;
    bool alarm_enabled;
} sensor_data_t;

typedef struct {
    sensor_data_t *sensors;
    uint8_t count;
    uint8_t max_count;
} sensor_manager_t;

typedef void (*sensor_callback_t)(uint8_t sensor_id, float value, sensor_status_t status);

esp_err_t sensor_manager_init(void);
esp_err_t sensor_manager_deinit(void);
esp_err_t sensor_register(uint8_t id, const char *name, sensor_type_t type, 
                          float min_val, float max_val, float threshold_min, float threshold_max);
esp_err_t sensor_unregister(uint8_t id);
esp_err_t sensor_read(uint8_t id, float *value);
esp_err_t sensor_read_all(sensor_data_t **sensors, uint8_t *count);
esp_err_t sensor_set_enabled(uint8_t id, bool enabled);
esp_err_t sensor_set_alarm(uint8_t id, bool enabled);
esp_err_t sensor_set_threshold(uint8_t id, float threshold_min, float threshold_max);
esp_err_t sensor_trigger_read(uint8_t id);
esp_err_t sensor_trigger_read_all(void);
bool sensor_check_alarm(uint8_t id);
bool sensor_check_all_alarms(void);
void sensor_set_callback(sensor_callback_t callback);
const char* sensor_type_to_string(sensor_type_t type);
const char* sensor_status_to_string(sensor_status_t status);

#endif
