#include "actuators/actuator_manager.h"
#include <driver/gpio.h>
#include <driver/ledc.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <string.h>
#include "utils/config.h"

static const char *TAG = "ACTUATOR_MGR";

static actuator_data_t actuators[CONFIG_MAX_ACTUATORS];
static uint8_t actuator_count = 0;
static actuator_callback_t user_callback = NULL;
static volatile bool initialized = false;
static SemaphoreHandle_t actuator_mutex = NULL;

esp_err_t actuator_manager_init(void)
{
    if (initialized) {
        ESP_LOGW(TAG, "Actuator manager already initialized");
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Initializing actuator manager");
    
    actuator_mutex = xSemaphoreCreateMutex();
    if (actuator_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create actuator mutex");
        return ESP_ERR_NO_MEM;
    }
    
    memset(actuators, 0, sizeof(actuators));
    actuator_count = 0;
    
    /* Fan actuators — pins chosen to avoid sensor GPIO conflicts */
    actuators[0].id = 0;
    strncpy(actuators[0].name, "Fan_1", sizeof(actuators[0].name) - 1);
    actuators[0].type = ACTUATOR_TYPE_FAN;
    actuators[0].pin = GPIO_NUM_2;
    actuator_count++;
    
    actuators[1].id = 1;
    strncpy(actuators[1].name, "Fan_2", sizeof(actuators[1].name) - 1);
    actuators[1].type = ACTUATOR_TYPE_FAN;
    actuators[1].pin = GPIO_NUM_4;
    actuator_count++;
    
    actuators[2].id = 2;
    strncpy(actuators[2].name, "Fan_3", sizeof(actuators[2].name) - 1);
    actuators[2].type = ACTUATOR_TYPE_FAN;
    actuators[2].pin = GPIO_NUM_5;
    actuator_count++;
    
    actuators[3].id = 3;
    strncpy(actuators[3].name, "Fan_4", sizeof(actuators[3].name) - 1);
    actuators[3].type = ACTUATOR_TYPE_FAN;
    actuators[3].pin = GPIO_NUM_18;
    actuator_count++;
    
    /* Heater actuators */
    actuators[4].id = 4;
    strncpy(actuators[4].name, "Heater_1", sizeof(actuators[4].name) - 1);
    actuators[4].type = ACTUATOR_TYPE_HEATER;
    actuators[4].pin = GPIO_NUM_19;
    actuator_count++;
    
    actuators[5].id = 5;
    strncpy(actuators[5].name, "Heater_2", sizeof(actuators[5].name) - 1);
    actuators[5].type = ACTUATOR_TYPE_HEATER;
    actuators[5].pin = GPIO_NUM_21;
    actuator_count++;
    
    /* Light actuators */
    actuators[6].id = 6;
    strncpy(actuators[6].name, "Light_1", sizeof(actuators[6].name) - 1);
    actuators[6].type = ACTUATOR_TYPE_LIGHT;
    actuators[6].pin = GPIO_NUM_22;
    actuator_count++;
    
    actuators[7].id = 7;
    strncpy(actuators[7].name, "Light_2", sizeof(actuators[7].name) - 1);
    actuators[7].type = ACTUATOR_TYPE_LIGHT;
    actuators[7].pin = GPIO_NUM_23;
    actuator_count++;
    
    /* Feeder actuators */
    actuators[8].id = 8;
    strncpy(actuators[8].name, "Feeder_1", sizeof(actuators[8].name) - 1);
    actuators[8].type = ACTUATOR_TYPE_FEEDER;
    actuators[8].pin = GPIO_NUM_25;
    actuator_count++;
    
    actuators[9].id = 9;
    strncpy(actuators[9].name, "Feeder_2", sizeof(actuators[9].name) - 1);
    actuators[9].type = ACTUATOR_TYPE_FEEDER;
    actuators[9].pin = GPIO_NUM_26;
    actuator_count++;
    
    /* Water pump actuators */
    actuators[10].id = 10;
    strncpy(actuators[10].name, "Water_Pump_1", sizeof(actuators[10].name) - 1);
    actuators[10].type = ACTUATOR_TYPE_PUMP;
    actuators[10].pin = GPIO_NUM_27;
    actuator_count++;
    
    actuators[11].id = 11;
    strncpy(actuators[11].name, "Water_Pump_2", sizeof(actuators[11].name) - 1);
    actuators[11].type = ACTUATOR_TYPE_PUMP;
    actuators[11].pin = GPIO_NUM_14;
    actuator_count++;
    
    for (int i = 0; i < actuator_count; i++) {
        actuators[i].state = ACTUATOR_STATE_OFF;
        actuators[i].duty_cycle = 0;
        actuators[i].enabled = true;
        actuators[i].manual_override = false;
        actuators[i].last_activation_time = 0;
        actuators[i].total_runtime = 0;
        actuators[i].activation_count = 0;
        
        gpio_reset_pin(actuators[i].pin);
        gpio_set_direction(actuators[i].pin, GPIO_MODE_OUTPUT);
        gpio_set_level(actuators[i].pin, 0);
    }
    
    initialized = true;
    ESP_LOGI(TAG, "Actuator manager initialized with %d actuators", actuator_count);
    
    return ESP_OK;
}

esp_err_t actuator_manager_deinit(void)
{
    if (!initialized) return ESP_OK;
    
    actuator_emergency_stop_all();
    initialized = false;
    actuator_count = 0;
    if (actuator_mutex) {
        vSemaphoreDelete(actuator_mutex);
        actuator_mutex = NULL;
    }
    
    return ESP_OK;
}

esp_err_t actuator_register(uint8_t id, const char *name, actuator_type_t type, uint8_t pin)
{
    if (actuator_count >= CONFIG_MAX_ACTUATORS) {
        return ESP_ERR_NO_MEM;
    }
    
    xSemaphoreTake(actuator_mutex, portMAX_DELAY);
    
    actuator_data_t *actuator = &actuators[actuator_count];
    actuator->id = id;
    strncpy(actuator->name, name, sizeof(actuator->name) - 1);
    actuator->name[sizeof(actuator->name) - 1] = '\0';
    actuator->type = type;
    actuator->pin = pin;
    actuator->state = ACTUATOR_STATE_OFF;
    actuator->enabled = true;
    actuator->manual_override = false;
    
    actuator_count++;
    
    xSemaphoreGive(actuator_mutex);
    
    return ESP_OK;
}

esp_err_t actuator_unregister(uint8_t id)
{
    xSemaphoreTake(actuator_mutex, portMAX_DELAY);
    
    for (int i = 0; i < actuator_count; i++) {
        if (actuators[i].id == id) {
            gpio_set_level(actuators[i].pin, 0);
            for (int j = i; j < actuator_count - 1; j++) {
                actuators[j] = actuators[j + 1];
            }
            actuator_count--;
            xSemaphoreGive(actuator_mutex);
            return ESP_OK;
        }
    }
    
    xSemaphoreGive(actuator_mutex);
    return ESP_ERR_NOT_FOUND;
}

esp_err_t actuator_set_state(uint8_t id, actuator_state_t state)
{
    xSemaphoreTake(actuator_mutex, portMAX_DELAY);
    
    for (int i = 0; i < actuator_count; i++) {
        if (actuators[i].id == id) {
            /* Skip if manual override is active and caller is setting auto */
            if (actuators[i].manual_override && state != ACTUATOR_STATE_AUTO) {
                xSemaphoreGive(actuator_mutex);
                return ESP_OK;
            }
            
            actuators[i].state = state;
            
            if (state == ACTUATOR_STATE_ON) {
                gpio_set_level(actuators[i].pin, 1);
                actuators[i].last_activation_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
                actuators[i].activation_count++;
            } else if (state == ACTUATOR_STATE_OFF) {
                gpio_set_level(actuators[i].pin, 0);
            } else if (state == ACTUATOR_STATE_AUTO) {
                actuators[i].manual_override = false;
            }
            
            if (user_callback) {
                user_callback(id, state);
            }
            
            xSemaphoreGive(actuator_mutex);
            return ESP_OK;
        }
    }
    
    xSemaphoreGive(actuator_mutex);
    return ESP_ERR_NOT_FOUND;
}

esp_err_t actuator_set_duty_cycle(uint8_t id, uint8_t duty_cycle)
{
    xSemaphoreTake(actuator_mutex, portMAX_DELAY);
    
    for (int i = 0; i < actuator_count; i++) {
        if (actuators[i].id == id) {
            if (duty_cycle > 100) duty_cycle = 100;
            actuators[i].duty_cycle = duty_cycle;
            xSemaphoreGive(actuator_mutex);
            return ESP_OK;
        }
    }
    
    xSemaphoreGive(actuator_mutex);
    return ESP_ERR_NOT_FOUND;
}

esp_err_t actuator_get_state(uint8_t id, actuator_state_t *state)
{
    xSemaphoreTake(actuator_mutex, portMAX_DELAY);
    
    for (int i = 0; i < actuator_count; i++) {
        if (actuators[i].id == id) {
            *state = actuators[i].state;
            xSemaphoreGive(actuator_mutex);
            return ESP_OK;
        }
    }
    
    xSemaphoreGive(actuator_mutex);
    return ESP_ERR_NOT_FOUND;
}

esp_err_t actuator_get_all(actuator_data_t **out_actuators, uint8_t *out_count)
{
    *out_actuators = actuators;
    *out_count = actuator_count;
    return ESP_OK;
}

esp_err_t actuator_set_enabled(uint8_t id, bool enabled)
{
    xSemaphoreTake(actuator_mutex, portMAX_DELAY);
    
    for (int i = 0; i < actuator_count; i++) {
        if (actuators[i].id == id) {
            actuators[i].enabled = enabled;
            if (!enabled) {
                gpio_set_level(actuators[i].pin, 0);
                actuators[i].state = ACTUATOR_STATE_OFF;
            }
            xSemaphoreGive(actuator_mutex);
            return ESP_OK;
        }
    }
    
    xSemaphoreGive(actuator_mutex);
    return ESP_ERR_NOT_FOUND;
}

esp_err_t actuator_set_manual_override(uint8_t id, bool override)
{
    xSemaphoreTake(actuator_mutex, portMAX_DELAY);
    
    for (int i = 0; i < actuator_count; i++) {
        if (actuators[i].id == id) {
            actuators[i].manual_override = override;
            xSemaphoreGive(actuator_mutex);
            return ESP_OK;
        }
    }
    
    xSemaphoreGive(actuator_mutex);
    return ESP_ERR_NOT_FOUND;
}

esp_err_t actuator_set_all_auto(void)
{
    xSemaphoreTake(actuator_mutex, portMAX_DELAY);
    
    for (int i = 0; i < actuator_count; i++) {
        actuators[i].manual_override = false;
        actuators[i].state = ACTUATOR_STATE_AUTO;
    }
    
    xSemaphoreGive(actuator_mutex);
    return ESP_OK;
}

esp_err_t actuator_emergency_stop_all(void)
{
    ESP_LOGW(TAG, "Emergency stop all actuators!");
    
    if (actuator_mutex) xSemaphoreTake(actuator_mutex, portMAX_DELAY);
    
    for (int i = 0; i < actuator_count; i++) {
        gpio_set_level(actuators[i].pin, 0);
        actuators[i].state = ACTUATOR_STATE_OFF;
    }
    
    if (actuator_mutex) xSemaphoreGive(actuator_mutex);
    
    return ESP_OK;
}

void actuator_set_callback(actuator_callback_t callback)
{
    user_callback = callback;
}

const char* actuator_type_to_string(actuator_type_t type)
{
    switch (type) {
        case ACTUATOR_TYPE_FAN: return "Fan";
        case ACTUATOR_TYPE_HEATER: return "Heater";
        case ACTUATOR_TYPE_LIGHT: return "Light";
        case ACTUATOR_TYPE_FEEDER: return "Feeder";
        case ACTUATOR_TYPE_PUMP: return "Pump";
        case ACTUATOR_TYPE_SERVO: return "Servo";
        case ACTUATOR_TYPE_VALVE: return "Valve";
        default: return "Unknown";
    }
}

const char* actuator_state_to_string(actuator_state_t state)
{
    switch (state) {
        case ACTUATOR_STATE_OFF: return "OFF";
        case ACTUATOR_STATE_ON: return "ON";
        case ACTUATOR_STATE_AUTO: return "AUTO";
        case ACTUATOR_STATE_ERROR: return "ERROR";
        default: return "Unknown";
    }
}
