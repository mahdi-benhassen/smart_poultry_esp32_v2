#ifndef ACTUATOR_MANAGER_H
#define ACTUATOR_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>

typedef enum {
    ACTUATOR_TYPE_FAN,
    ACTUATOR_TYPE_HEATER,
    ACTUATOR_TYPE_LIGHT,
    ACTUATOR_TYPE_FEEDER,
    ACTUATOR_TYPE_PUMP,
    ACTUATOR_TYPE_SERVO,
    ACTUATOR_TYPE_VALVE
} actuator_type_t;

typedef enum {
    ACTUATOR_STATE_OFF,
    ACTUATOR_STATE_ON,
    ACTUATOR_STATE_AUTO,
    ACTUATOR_STATE_ERROR
} actuator_state_t;

typedef struct {
    uint8_t id;
    char name[64];
    actuator_type_t type;
    actuator_state_t state;
    uint8_t pin;
    uint8_t duty_cycle;
    bool enabled;
    bool manual_override;
    uint32_t last_activation_time;
    uint32_t total_runtime;
    uint32_t activation_count;
} actuator_data_t;

typedef void (*actuator_callback_t)(uint8_t actuator_id, actuator_state_t state);

esp_err_t actuator_manager_init(void);
esp_err_t actuator_manager_deinit(void);
esp_err_t actuator_register(uint8_t id, const char *name, actuator_type_t type, uint8_t pin);
esp_err_t actuator_unregister(uint8_t id);
esp_err_t actuator_set_state(uint8_t id, actuator_state_t state);
esp_err_t actuator_set_duty_cycle(uint8_t id, uint8_t duty_cycle);
esp_err_t actuator_get_state(uint8_t id, actuator_state_t *state);
esp_err_t actuator_get_all(actuator_data_t **actuators, uint8_t *count);
esp_err_t actuator_set_enabled(uint8_t id, bool enabled);
esp_err_t actuator_set_manual_override(uint8_t id, bool override);
esp_err_t actuator_set_all_auto(void);
esp_err_t actuator_emergency_stop_all(void);
void actuator_set_callback(actuator_callback_t callback);
const char* actuator_type_to_string(actuator_type_t type);
const char* actuator_state_to_string(actuator_state_t state);

#endif
