#ifndef MONITORING_H
#define MONITORING_H

#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>

typedef struct {
    uint32_t timestamp;
    float temperature_avg;
    float humidity_avg;
    float ammonia_max;
    float co2_max;
    uint16_t alarm_count;
    uint16_t actuator_activations;
    uint8_t system_status;
} system_status_t;

typedef struct {
    uint32_t timestamp;
    char sensor_name[64];
    float value;
    float threshold_min;
    float threshold_max;
    bool alarm_triggered;
} alarm_event_t;

typedef struct {
    char message[256];
    uint32_t timestamp;
    uint8_t severity;
} log_event_t;

esp_err_t monitoring_init(void);
esp_err_t monitoring_start(void);
esp_err_t monitoring_stop(void);
esp_err_t monitoring_update(void);
esp_err_t monitoring_get_status(system_status_t *status);
esp_err_t monitoring_log_event(const char *message, uint8_t severity);
esp_err_t monitoring_check_alarms(void);
esp_err_t monitoring_get_alarm_count(uint16_t *count);
esp_err_t monitoring_clear_alarms(void);
esp_err_t monitoring_set_log_level(uint8_t level);
esp_err_t monitoring_export_data(char *buffer, uint16_t buffer_size);

#endif
