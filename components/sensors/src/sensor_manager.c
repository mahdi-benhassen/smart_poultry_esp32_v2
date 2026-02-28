#include <string.h>
#include <esp_log.h>
#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <driver/gpio.h>
#include <driver/adc.h>
#include <esp_adc_cal.h>
#include "sensors/sensor_manager.h"
#include "sensors/dht22.h"
#include "sensors/mq_sensor.h"
#include "sensors/bme280_sensor.h"
#include "sensors/weight_sensor.h"
#include "sensors/water_level_sensor.h"
#include "utils/config.h"

static const char *TAG = "SENSOR_MGR";

static sensor_data_t sensors[CONFIG_MAX_SENSORS];
static uint8_t sensor_count = 0;
static sensor_callback_t user_callback = NULL;
static volatile bool initialized = false;
static SemaphoreHandle_t sensor_mutex = NULL;

/* Pointers to driver-level sensor arrays for data propagation */
static sensor_data_t *driver_arrays[5] = {NULL};
static uint8_t driver_counts[5] = {0};
#define DRIVER_DHT22   0
#define DRIVER_MQ      1
#define DRIVER_BME280  2
#define DRIVER_WEIGHT  3
#define DRIVER_WATER   4

esp_err_t sensor_manager_init(void)
{
    if (initialized) {
        ESP_LOGW(TAG, "Sensor manager already initialized");
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Initializing sensor manager");
    
    sensor_mutex = xSemaphoreCreateMutex();
    if (sensor_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create sensor mutex");
        return ESP_ERR_NO_MEM;
    }
    
    memset(sensors, 0, sizeof(sensors));
    sensor_count = 0;
    
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(ADC1_CHANNEL_3, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(ADC1_CHANNEL_4, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(ADC1_CHANNEL_5, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(ADC1_CHANNEL_7, ADC_ATTEN_DB_11);
    
    dht22_init();
    mq_sensor_init();
    bme280_init();
    weight_sensor_init();
    water_level_sensor_init();
    
    /* Store pointers to driver arrays for later data propagation */
    
    driver_arrays[DRIVER_DHT22] = get_dht22_sensors(&driver_counts[DRIVER_DHT22]);
    for (int i = 0; i < driver_counts[DRIVER_DHT22] && sensor_count < CONFIG_MAX_SENSORS; i++) {
        sensors[sensor_count++] = driver_arrays[DRIVER_DHT22][i];
    }
    
    driver_arrays[DRIVER_MQ] = get_mq_sensors(&driver_counts[DRIVER_MQ]);
    for (int i = 0; i < driver_counts[DRIVER_MQ] && sensor_count < CONFIG_MAX_SENSORS; i++) {
        sensors[sensor_count++] = driver_arrays[DRIVER_MQ][i];
    }
    
    driver_arrays[DRIVER_BME280] = get_bme280_sensors(&driver_counts[DRIVER_BME280]);
    for (int i = 0; i < driver_counts[DRIVER_BME280] && sensor_count < CONFIG_MAX_SENSORS; i++) {
        sensors[sensor_count++] = driver_arrays[DRIVER_BME280][i];
    }
    
    driver_arrays[DRIVER_WEIGHT] = get_weight_sensors(&driver_counts[DRIVER_WEIGHT]);
    for (int i = 0; i < driver_counts[DRIVER_WEIGHT] && sensor_count < CONFIG_MAX_SENSORS; i++) {
        sensors[sensor_count++] = driver_arrays[DRIVER_WEIGHT][i];
    }
    
    driver_arrays[DRIVER_WATER] = get_water_level_sensors(&driver_counts[DRIVER_WATER]);
    for (int i = 0; i < driver_counts[DRIVER_WATER] && sensor_count < CONFIG_MAX_SENSORS; i++) {
        sensors[sensor_count++] = driver_arrays[DRIVER_WATER][i];
    }
    
    initialized = true;
    ESP_LOGI(TAG, "Sensor manager initialized with %d sensors", sensor_count);
    
    return ESP_OK;
}

esp_err_t sensor_manager_deinit(void)
{
    if (!initialized) {
        return ESP_OK;
    }
    
    initialized = false;
    sensor_count = 0;
    if (sensor_mutex) {
        vSemaphoreDelete(sensor_mutex);
        sensor_mutex = NULL;
    }
    ESP_LOGI(TAG, "Sensor manager deinitialized");
    
    return ESP_OK;
}

esp_err_t sensor_register(uint8_t id, const char *name, sensor_type_t type,
                         float min_val, float max_val, float threshold_min, float threshold_max)
{
    if (sensor_count >= CONFIG_MAX_SENSORS) {
        ESP_LOGE(TAG, "Max sensors reached");
        return ESP_ERR_NO_MEM;
    }
    
    xSemaphoreTake(sensor_mutex, portMAX_DELAY);
    
    sensor_data_t *sensor = &sensors[sensor_count];
    sensor->id = id;
    strncpy(sensor->name, name, sizeof(sensor->name) - 1);
    sensor->name[sizeof(sensor->name) - 1] = '\0';
    sensor->type = type;
    sensor->status = SENSOR_STATUS_OK;
    sensor->value = 0.0f;
    sensor->min_value = min_val;
    sensor->max_value = max_val;
    sensor->threshold_min = threshold_min;
    sensor->threshold_max = threshold_max;
    sensor->last_read_time = 0;
    sensor->enabled = true;
    sensor->alarm_enabled = true;
    
    sensor_count++;
    
    xSemaphoreGive(sensor_mutex);
    
    ESP_LOGI(TAG, "Registered sensor: %s (ID: %d, Type: %d)", name, id, type);
    
    return ESP_OK;
}

esp_err_t sensor_unregister(uint8_t id)
{
    xSemaphoreTake(sensor_mutex, portMAX_DELAY);
    
    for (int i = 0; i < sensor_count; i++) {
        if (sensors[i].id == id) {
            for (int j = i; j < sensor_count - 1; j++) {
                sensors[j] = sensors[j + 1];
            }
            sensor_count--;
            xSemaphoreGive(sensor_mutex);
            ESP_LOGI(TAG, "Unregistered sensor ID: %d", id);
            return ESP_OK;
        }
    }
    
    xSemaphoreGive(sensor_mutex);
    return ESP_ERR_NOT_FOUND;
}

esp_err_t sensor_read(uint8_t id, float *value)
{
    xSemaphoreTake(sensor_mutex, portMAX_DELAY);
    
    for (int i = 0; i < sensor_count; i++) {
        if (sensors[i].id == id) {
            *value = sensors[i].value;
            xSemaphoreGive(sensor_mutex);
            return ESP_OK;
        }
    }
    
    xSemaphoreGive(sensor_mutex);
    return ESP_ERR_NOT_FOUND;
}

esp_err_t sensor_read_all(sensor_data_t **out_sensors, uint8_t *out_count)
{
    *out_sensors = sensors;
    *out_count = sensor_count;
    return ESP_OK;
}

esp_err_t sensor_set_enabled(uint8_t id, bool enabled)
{
    xSemaphoreTake(sensor_mutex, portMAX_DELAY);
    
    for (int i = 0; i < sensor_count; i++) {
        if (sensors[i].id == id) {
            sensors[i].enabled = enabled;
            xSemaphoreGive(sensor_mutex);
            ESP_LOGI(TAG, "Sensor %d enabled: %d", id, enabled);
            return ESP_OK;
        }
    }
    
    xSemaphoreGive(sensor_mutex);
    return ESP_ERR_NOT_FOUND;
}

esp_err_t sensor_set_alarm(uint8_t id, bool enabled)
{
    xSemaphoreTake(sensor_mutex, portMAX_DELAY);
    
    for (int i = 0; i < sensor_count; i++) {
        if (sensors[i].id == id) {
            sensors[i].alarm_enabled = enabled;
            xSemaphoreGive(sensor_mutex);
            return ESP_OK;
        }
    }
    
    xSemaphoreGive(sensor_mutex);
    return ESP_ERR_NOT_FOUND;
}

esp_err_t sensor_set_threshold(uint8_t id, float threshold_min, float threshold_max)
{
    xSemaphoreTake(sensor_mutex, portMAX_DELAY);
    
    for (int i = 0; i < sensor_count; i++) {
        if (sensors[i].id == id) {
            sensors[i].threshold_min = threshold_min;
            sensors[i].threshold_max = threshold_max;
            xSemaphoreGive(sensor_mutex);
            return ESP_OK;
        }
    }
    
    xSemaphoreGive(sensor_mutex);
    return ESP_ERR_NOT_FOUND;
}

/**
 * Propagate data from driver-level arrays back into the manager's sensors[] array.
 * This is needed because driver _read_all() writes to their own local arrays.
 */
static void sensor_propagate_driver_data(void)
{
    uint8_t idx = 0;
    for (int d = 0; d < 5; d++) {
        if (driver_arrays[d] == NULL) continue;
        for (int i = 0; i < driver_counts[d] && idx < sensor_count; i++, idx++) {
            sensors[idx].value = driver_arrays[d][i].value;
            sensors[idx].status = driver_arrays[d][i].status;
        }
    }
}

esp_err_t sensor_trigger_read(uint8_t id)
{
    /* Trigger a full read from all drivers and propagate */
    dht22_read_all();
    mq_sensor_read_all();
    bme280_read_all();
    weight_sensor_read_all();
    water_level_sensor_read_all();
    
    xSemaphoreTake(sensor_mutex, portMAX_DELAY);
    sensor_propagate_driver_data();
    
    for (int i = 0; i < sensor_count; i++) {
        if (sensors[i].id == id && sensors[i].enabled) {
            sensors[i].last_read_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
            
            if (user_callback) {
                user_callback(sensors[i].id, sensors[i].value, sensors[i].status);
            }
            xSemaphoreGive(sensor_mutex);
            return ESP_OK;
        }
    }
    
    xSemaphoreGive(sensor_mutex);
    return ESP_ERR_NOT_FOUND;
}

esp_err_t sensor_trigger_read_all(void)
{
    /* Read from all hardware drivers */
    dht22_read_all();
    mq_sensor_read_all();
    bme280_read_all();
    weight_sensor_read_all();
    water_level_sensor_read_all();
    
    xSemaphoreTake(sensor_mutex, portMAX_DELAY);
    
    /* Propagate updated values from driver arrays to manager array */
    sensor_propagate_driver_data();
    
    uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    for (int i = 0; i < sensor_count; i++) {
        sensors[i].last_read_time = current_time;
        
        if (user_callback) {
            user_callback(sensors[i].id, sensors[i].value, sensors[i].status);
        }
    }
    
    xSemaphoreGive(sensor_mutex);
    
    return ESP_OK;
}

bool sensor_check_alarm(uint8_t id)
{
    xSemaphoreTake(sensor_mutex, portMAX_DELAY);
    
    for (int i = 0; i < sensor_count; i++) {
        if (sensors[i].id == id && sensors[i].alarm_enabled) {
            if (sensors[i].value < sensors[i].threshold_min ||
                sensors[i].value > sensors[i].threshold_max) {
                xSemaphoreGive(sensor_mutex);
                return true;
            }
        }
    }
    
    xSemaphoreGive(sensor_mutex);
    return false;
}

bool sensor_check_all_alarms(void)
{
    xSemaphoreTake(sensor_mutex, portMAX_DELAY);
    
    for (int i = 0; i < sensor_count; i++) {
        if (sensors[i].alarm_enabled && sensors[i].enabled) {
            if (sensors[i].value < sensors[i].threshold_min ||
                sensors[i].value > sensors[i].threshold_max) {
                ESP_LOGW(TAG, "Alarm triggered: %s (value: %.2f, min: %.2f, max: %.2f)",
                        sensors[i].name, sensors[i].value,
                        sensors[i].threshold_min, sensors[i].threshold_max);
                xSemaphoreGive(sensor_mutex);
                return true;
            }
        }
    }
    
    xSemaphoreGive(sensor_mutex);
    return false;
}

void sensor_set_callback(sensor_callback_t callback)
{
    user_callback = callback;
}

const char* sensor_type_to_string(sensor_type_t type)
{
    switch (type) {
        case SENSOR_TYPE_TEMPERATURE: return "Temperature";
        case SENSOR_TYPE_HUMIDITY: return "Humidity";
        case SENSOR_TYPE_PRESSURE: return "Pressure";
        case SENSOR_TYPE_AMMONIA: return "Ammonia";
        case SENSOR_TYPE_CO2: return "CO2";
        case SENSOR_TYPE_CO: return "CO";
        case SENSOR_TYPE_METHANE: return "Methane";
        case SENSOR_TYPE_LIGHT: return "Light";
        case SENSOR_TYPE_SOUND: return "Sound";
        case SENSOR_TYPE_WATER_LEVEL: return "Water Level";
        case SENSOR_TYPE_WEIGHT: return "Weight";
        case SENSOR_TYPE_MOTION: return "Motion";
        case SENSOR_TYPE_DOOR: return "Door";
        default: return "Unknown";
    }
}

const char* sensor_status_to_string(sensor_status_t status)
{
    switch (status) {
        case SENSOR_STATUS_OK: return "OK";
        case SENSOR_STATUS_ERROR: return "Error";
        case SENSOR_STATUS_CALIBRATING: return "Calibrating";
        case SENSOR_STATUS_OFFLINE: return "Offline";
        default: return "Unknown";
    }
}
