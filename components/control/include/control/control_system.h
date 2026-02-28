#ifndef CONTROL_SYSTEM_H
#define CONTROL_SYSTEM_H

#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>

typedef enum {
    CONTROL_MODE_MANUAL,
    CONTROL_MODE_AUTO,
    CONTROL_MODE_SCHEDULED,
    CONTROL_MODE_ADAPTIVE
} control_mode_t;

typedef struct {
    float temperature;
    float humidity;
    float ammonia;
    float co2;
    float light;
    uint32_t timestamp;
} environment_reading_t;

typedef struct {
    control_mode_t mode;
    bool auto_fan_enabled;
    bool auto_heater_enabled;
    bool auto_light_enabled;
    bool auto_feeder_enabled;
    bool auto_pump_enabled;
    bool emergency_stop;
    uint32_t last_control_time;
    uint32_t control_interval_ms;
} control_state_t;

esp_err_t control_system_init(void);
esp_err_t control_system_start(void);
esp_err_t control_system_stop(void);
esp_err_t control_system_set_mode(control_mode_t mode);
esp_err_t control_system_get_mode(control_mode_t *mode);
esp_err_t control_system_update(void);
esp_err_t control_system_emergency_stop(void);
esp_err_t control_system_reset_emergency(void);
esp_err_t control_system_enable_auto_fan(bool enable);
esp_err_t control_system_enable_auto_heater(bool enable);
esp_err_t control_system_enable_auto_light(bool enable);
esp_err_t control_system_enable_auto_feeder(bool enable);
esp_err_t control_system_enable_auto_pump(bool enable);
esp_err_t control_system_set_control_interval(uint32_t interval_ms);
esp_err_t control_system_get_state(control_state_t *state);

void control_temperature_logic(float temperature, float humidity);
void control_humidity_logic(float humidity, float temperature);
void control_gas_logic(float ammonia, float co2, float co);
void control_light_logic(float light_level, uint8_t hour);
void control_feeder_logic(void);
void control_water_logic(float water_level);

#endif
