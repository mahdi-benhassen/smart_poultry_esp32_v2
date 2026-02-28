#include "monitoring/monitoring.h"
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <sensors/sensor_manager.h>
#include <actuators/actuator_manager.h>
#include <utils/config.h>
#include <string.h>

static const char *TAG = "MONITORING";

#define MAX_LOG_ENTRIES 100
#define MAX_ALARM_ENTRIES 50

static volatile bool initialized = false;
static volatile bool running = false;
static TaskHandle_t monitoring_task_handle = NULL;
static uint16_t alarm_count = 0;
static uint8_t log_level = 3;
static system_status_t current_status = {0};
static log_event_t log_entries[MAX_LOG_ENTRIES];
static uint8_t log_index = 0;

static void monitoring_task(void *parameter)
{
    ESP_LOGI(TAG, "Monitoring task started");
    
    while (running) {
        monitoring_update();
        vTaskDelay(pdMS_TO_TICKS(CONFIG_MONITORING_INTERVAL_MS));
    }
    
    ESP_LOGI(TAG, "Monitoring task stopped");
    vTaskDelete(NULL);
}

esp_err_t monitoring_init(void)
{
    if (initialized) return ESP_OK;
    
    ESP_LOGI(TAG, "Initializing monitoring system");
    
    memset(log_entries, 0, sizeof(log_entries));
    log_index = 0;
    alarm_count = 0;
    
    monitoring_log_event("System initialized", 3);
    
    initialized = true;
    ESP_LOGI(TAG, "Monitoring system initialized");
    
    return ESP_OK;
}

esp_err_t monitoring_start(void)
{
    if (!initialized || running) return ESP_ERR_INVALID_STATE;
    
    running = true;
    xTaskCreate(monitoring_task, "monitoring_task", 4096, NULL, 4, &monitoring_task_handle);
    
    ESP_LOGI(TAG, "Monitoring system started");
    return ESP_OK;
}

esp_err_t monitoring_stop(void)
{
    if (!running) return ESP_ERR_INVALID_STATE;
    
    /* Signal the task to stop; it will self-delete via vTaskDelete(NULL) */
    running = false;
    monitoring_task_handle = NULL;
    
    ESP_LOGI(TAG, "Monitoring system stopped");
    return ESP_OK;
}

esp_err_t monitoring_update(void)
{
    sensor_trigger_read_all();
    
    uint8_t sensor_count = 0;
    sensor_data_t *sensors = NULL;
    sensor_read_all(&sensors, &sensor_count);
    
    float temp_sum = 0, hum_sum = 0;
    float ammonia_max = 0, co2_max = 0;
    uint8_t temp_count = 0, hum_count = 0;
    
    for (int i = 0; i < sensor_count; i++) {
        if (sensors[i].type == SENSOR_TYPE_TEMPERATURE) {
            temp_sum += sensors[i].value;
            temp_count++;
        } else if (sensors[i].type == SENSOR_TYPE_HUMIDITY) {
            hum_sum += sensors[i].value;
            hum_count++;
        } else if (sensors[i].type == SENSOR_TYPE_AMMONIA) {
            if (sensors[i].value > ammonia_max) {
                ammonia_max = sensors[i].value;
            }
        } else if (sensors[i].type == SENSOR_TYPE_CO2) {
            if (sensors[i].value > co2_max) {
                co2_max = sensors[i].value;
            }
        }
    }
    
    current_status.timestamp = xTaskGetTickCount() * portTICK_PERIOD_MS;
    current_status.temperature_avg = temp_count > 0 ? temp_sum / temp_count : 0;
    current_status.humidity_avg = hum_count > 0 ? hum_sum / hum_count : 0;
    current_status.ammonia_max = ammonia_max;
    current_status.co2_max = co2_max;
    current_status.alarm_count = alarm_count;
    current_status.system_status = running ? 1 : 0;
    
    monitoring_check_alarms();
    
    return ESP_OK;
}

esp_err_t monitoring_get_status(system_status_t *status)
{
    memcpy(status, &current_status, sizeof(system_status_t));
    return ESP_OK;
}

esp_err_t monitoring_log_event(const char *message, uint8_t severity)
{
    if (severity > log_level) return ESP_OK;
    
    log_entries[log_index].timestamp = xTaskGetTickCount() * portTICK_PERIOD_MS;
    log_entries[log_index].severity = severity;
    strncpy(log_entries[log_index].message, message, sizeof(log_entries[log_index].message) - 1);
    
    log_index = (log_index + 1) % MAX_LOG_ENTRIES;
    
    ESP_LOGI(TAG, "[%d] %s", severity, message);
    
    return ESP_OK;
}

esp_err_t monitoring_check_alarms(void)
{
    bool alarm_triggered = sensor_check_all_alarms();
    
    if (alarm_triggered) {
        alarm_count++;
        monitoring_log_event("Alarm triggered - check sensors", 1);
    }
    
    return ESP_OK;
}

esp_err_t monitoring_get_alarm_count(uint16_t *count)
{
    *count = alarm_count;
    return ESP_OK;
}

esp_err_t monitoring_clear_alarms(void)
{
    alarm_count = 0;
    monitoring_log_event("Alarms cleared", 2);
    return ESP_OK;
}

esp_err_t monitoring_set_log_level(uint8_t level)
{
    log_level = level;
    return ESP_OK;
}

esp_err_t monitoring_export_data(char *buffer, uint16_t buffer_size)
{
    snprintf(buffer, buffer_size,
        "Status: Temp=%.2f, Humidity=%.2f, Ammonia=%.2f, CO2=%.2f, Alarms=%d",
        current_status.temperature_avg,
        current_status.humidity_avg,
        current_status.ammonia_max,
        current_status.co2_max,
        current_status.alarm_count);
    
    return ESP_OK;
}
