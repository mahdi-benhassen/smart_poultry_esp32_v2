#include "control/control_system.h"
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h>
#include <time.h>
#include "sensors/sensor_manager.h"
#include "actuators/actuator_manager.h"
#include "utils/config.h"

static const char *TAG = "CONTROL_SYS";

static control_state_t control_state = {0};
static volatile bool initialized = false;
static volatile bool running = false;
static TaskHandle_t control_task_handle = NULL;

static void control_task(void *parameter)
{
    ESP_LOGI(TAG, "Control task started");
    
    while (running) {
        if (!control_state.emergency_stop) {
            control_system_update();
        }
        vTaskDelay(pdMS_TO_TICKS(control_state.control_interval_ms));
    }
    
    ESP_LOGI(TAG, "Control task stopped");
    vTaskDelete(NULL);
}

esp_err_t control_system_init(void)
{
    if (initialized) return ESP_OK;
    
    ESP_LOGI(TAG, "Initializing control system");
    
    control_state.mode = CONTROL_MODE_AUTO;
    control_state.auto_fan_enabled = true;
    control_state.auto_heater_enabled = true;
    control_state.auto_light_enabled = true;
    control_state.auto_feeder_enabled = true;
    control_state.auto_pump_enabled = true;
    control_state.emergency_stop = false;
    control_state.last_control_time = 0;
    control_state.control_interval_ms = CONFIG_CONTROL_LOOP_INTERVAL_MS;
    
    initialized = true;
    ESP_LOGI(TAG, "Control system initialized");
    
    return ESP_OK;
}

esp_err_t control_system_start(void)
{
    if (!initialized || running) return ESP_ERR_INVALID_STATE;
    
    running = true;
    xTaskCreate(control_task, "control_task", 4096, NULL, 5, &control_task_handle);
    
    ESP_LOGI(TAG, "Control system started");
    return ESP_OK;
}

esp_err_t control_system_stop(void)
{
    if (!running) return ESP_ERR_INVALID_STATE;
    
    /* Signal task to stop, let it self-delete via vTaskDelete(NULL) */
    running = false;
    control_task_handle = NULL;
    
    ESP_LOGI(TAG, "Control system stopped");
    return ESP_OK;
}

esp_err_t control_system_set_mode(control_mode_t mode)
{
    control_state.mode = mode;
    ESP_LOGI(TAG, "Control mode set to: %d", mode);
    return ESP_OK;
}

esp_err_t control_system_get_mode(control_mode_t *mode)
{
    *mode = control_state.mode;
    return ESP_OK;
}

esp_err_t control_system_update(void)
{
    if (control_state.mode != CONTROL_MODE_AUTO) {
        return ESP_OK;
    }
    
    sensor_trigger_read_all();
    
    uint8_t sensor_count = 0;
    sensor_data_t *sensors = NULL;
    sensor_read_all(&sensors, &sensor_count);
    
    float temperature = 25.0f;
    float humidity = 60.0f;
    float ammonia = 5.0f;
    float co2 = 400.0f;
    float co = 0.0f;
    float light = 500.0f;
    float water_level = 50.0f;
    
    for (int i = 0; i < sensor_count; i++) {
        if (strcmp(sensors[i].name, "Temperature_1") == 0) {
            temperature = sensors[i].value;
        } else if (strcmp(sensors[i].name, "Humidity_1") == 0) {
            humidity = sensors[i].value;
        } else if (strcmp(sensors[i].name, "Ammonia_Sensor") == 0) {
            ammonia = sensors[i].value;
        } else if (strcmp(sensors[i].name, "CO2_Sensor") == 0) {
            co2 = sensors[i].value;
        } else if (strcmp(sensors[i].name, "CO_Sensor") == 0) {
            co = sensors[i].value;
        } else if (sensors[i].type == SENSOR_TYPE_WATER_LEVEL) {
            water_level = sensors[i].value;
        }
    }
    
    /* Get current hour from system time */
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    uint8_t current_hour = (uint8_t)timeinfo.tm_hour;
    
    /* Temperature control: fans for cooling, heaters for heating */
    if (control_state.auto_fan_enabled || control_state.auto_heater_enabled) {
        control_temperature_logic(temperature, humidity);
    }
    
    /* Gas ventilation */
    if (control_state.auto_fan_enabled) {
        control_gas_logic(ammonia, co2, co);
    }
    
    /* Humidity control */
    control_humidity_logic(humidity, temperature);
    
    /* Lighting control */
    if (control_state.auto_light_enabled) {
        control_light_logic(light, current_hour);
    }
    
    /* Water management */
    if (control_state.auto_pump_enabled) {
        control_water_logic(water_level);
    }
    
    /* Feeding schedule */
    if (control_state.auto_feeder_enabled) {
        control_feeder_logic();
    }
    
    control_state.last_control_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    return ESP_OK;
}

esp_err_t control_system_emergency_stop(void)
{
    ESP_LOGW(TAG, "EMERGENCY STOP ACTIVATED");
    
    control_state.emergency_stop = true;
    actuator_emergency_stop_all();
    
    return ESP_OK;
}

esp_err_t control_system_reset_emergency(void)
{
    control_state.emergency_stop = false;
    ESP_LOGI(TAG, "Emergency stop reset");
    return ESP_OK;
}

esp_err_t control_system_enable_auto_fan(bool enable)
{
    control_state.auto_fan_enabled = enable;
    return ESP_OK;
}

esp_err_t control_system_enable_auto_heater(bool enable)
{
    control_state.auto_heater_enabled = enable;
    return ESP_OK;
}

esp_err_t control_system_enable_auto_light(bool enable)
{
    control_state.auto_light_enabled = enable;
    return ESP_OK;
}

esp_err_t control_system_enable_auto_feeder(bool enable)
{
    control_state.auto_feeder_enabled = enable;
    return ESP_OK;
}

esp_err_t control_system_enable_auto_pump(bool enable)
{
    control_state.auto_pump_enabled = enable;
    return ESP_OK;
}

esp_err_t control_system_set_control_interval(uint32_t interval_ms)
{
    control_state.control_interval_ms = interval_ms;
    return ESP_OK;
}

esp_err_t control_system_get_state(control_state_t *state)
{
    memcpy(state, &control_state, sizeof(control_state_t));
    return ESP_OK;
}

void control_temperature_logic(float temperature, float humidity)
{
    if (temperature > poultry_config.temp_max) {
        /* Too hot: turn on fans for cooling */
        for (int i = 0; i < 4; i++) {
            actuator_set_state(i, ACTUATOR_STATE_ON);
        }
        /* Turn off heaters */
        for (int i = 4; i < 6; i++) {
            actuator_set_state(i, ACTUATOR_STATE_OFF);
        }
    } else if (temperature < poultry_config.temp_min) {
        /* Too cold: turn on heaters */
        for (int i = 4; i < 6; i++) {
            actuator_set_state(i, ACTUATOR_STATE_ON);
        }
        /* Turn off fans */
        for (int i = 0; i < 4; i++) {
            actuator_set_state(i, ACTUATOR_STATE_OFF);
        }
    } else {
        /* Temperature in acceptable range: turn off both */
        for (int i = 0; i < 4; i++) {
            actuator_set_state(i, ACTUATOR_STATE_OFF);
        }
        for (int i = 4; i < 6; i++) {
            actuator_set_state(i, ACTUATOR_STATE_OFF);
        }
    }
}

void control_humidity_logic(float humidity, float temperature)
{
    if (humidity > poultry_config.humidity_max) {
        /* Too humid: activate ventilation fans */
        for (int i = 0; i < 4; i++) {
            actuator_set_state(i, ACTUATOR_STATE_ON);
        }
    } else if (humidity < poultry_config.humidity_min) {
        /* Too dry: reduce ventilation (if not needed for temp) */
        if (temperature >= poultry_config.temp_min && temperature <= poultry_config.temp_max) {
            for (int i = 0; i < 4; i++) {
                actuator_set_state(i, ACTUATOR_STATE_OFF);
            }
        }
    }
}

void control_gas_logic(float ammonia, float co2, float co)
{
    if (ammonia > poultry_config.ammonia_max || co2 > poultry_config.co2_max || co > poultry_config.co_max) {
        /* Dangerous gas levels: maximum ventilation */
        ESP_LOGW(TAG, "High gas levels! NH3=%.1f CO2=%.1f CO=%.1f - activating ventilation",
                 ammonia, co2, co);
        for (int i = 0; i < 4; i++) {
            actuator_set_state(i, ACTUATOR_STATE_ON);
        }
    }
}

void control_light_logic(float light_level, uint8_t hour)
{
    if (hour >= 6 && hour <= 18) {
        if (light_level < 300.0f) {
            for (int i = 6; i < 8; i++) {
                actuator_set_state(i, ACTUATOR_STATE_ON);
            }
        } else {
            for (int i = 6; i < 8; i++) {
                actuator_set_state(i, ACTUATOR_STATE_OFF);
            }
        }
    } else {
        for (int i = 6; i < 8; i++) {
            actuator_set_state(i, ACTUATOR_STATE_OFF);
        }
    }
}

void control_feeder_logic(void)
{
    /* Get current time for feeding schedule */
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    /* Feed at 6:00, 12:00, and 18:00 for 30 seconds */
    bool feeding_time = (timeinfo.tm_min == 0 && timeinfo.tm_sec < 30) &&
                        (timeinfo.tm_hour == 6 || timeinfo.tm_hour == 12 || timeinfo.tm_hour == 18);
    
    if (feeding_time) {
        actuator_set_state(8, ACTUATOR_STATE_ON);
        actuator_set_state(9, ACTUATOR_STATE_ON);
    } else {
        actuator_set_state(8, ACTUATOR_STATE_OFF);
        actuator_set_state(9, ACTUATOR_STATE_OFF);
    }
}

void control_water_logic(float water_level)
{
    if (water_level < 30.0f) {
        actuator_set_state(10, ACTUATOR_STATE_ON);
        actuator_set_state(11, ACTUATOR_STATE_ON);
    } else if (water_level > 80.0f) {
        actuator_set_state(10, ACTUATOR_STATE_OFF);
        actuator_set_state(11, ACTUATOR_STATE_OFF);
    }
}
