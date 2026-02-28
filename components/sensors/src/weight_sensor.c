#include "sensors/weight_sensor.h"
#include "sensors/sensor_manager.h"
#include <driver/adc.h>
#include <esp_log.h>
#include <string.h>

static const char *TAG = "WEIGHT_SENSOR";

#define WEIGHT_ADC_CHANNEL_1 ADC1_CHANNEL_6
#define WEIGHT_ADC_CHANNEL_2 ADC1_CHANNEL_7

/* ESP32 ADC reference voltage is 3.3V */
#define ADC_VREF 3.3f
#define ADC_MAX  4095.0f

static weight_data_t weight_data = {0};
static sensor_data_t weight_sensors[2];
static bool initialized = false;
static float tare_offset_1 = 0.0f;
static float tare_offset_2 = 0.0f;
static float calibration_factor_1 = 1.0f;
static float calibration_factor_2 = 1.0f;

static float read_weight_from_channel(adc_channel_t channel, float tare, float cal_factor)
{
    int adc_value = adc1_get_raw(channel);
    float voltage = (float)adc_value / ADC_MAX * ADC_VREF;
    float weight = (voltage * cal_factor) - tare;
    if (weight < 0) weight = 0;
    return weight;
}

esp_err_t weight_sensor_init(void)
{
    if (initialized) return ESP_OK;
    
    ESP_LOGI(TAG, "Initializing weight sensors");
    
    weight_sensors[0].id = 30;
    strncpy(weight_sensors[0].name, "Feeder_Weight", sizeof(weight_sensors[0].name) - 1);
    weight_sensors[0].type = SENSOR_TYPE_WEIGHT;
    weight_sensors[0].status = SENSOR_STATUS_OK;
    weight_sensors[0].value = 0.0f;
    weight_sensors[0].min_value = 0.0f;
    weight_sensors[0].max_value = 50.0f;
    weight_sensors[0].threshold_min = 0.0f;
    weight_sensors[0].threshold_max = 20.0f;
    weight_sensors[0].enabled = true;
    weight_sensors[0].alarm_enabled = false;
    
    weight_sensors[1].id = 31;
    strncpy(weight_sensors[1].name, "Bird_Weight", sizeof(weight_sensors[1].name) - 1);
    weight_sensors[1].type = SENSOR_TYPE_WEIGHT;
    weight_sensors[1].status = SENSOR_STATUS_OK;
    weight_sensors[1].value = 0.0f;
    weight_sensors[1].min_value = 0.0f;
    weight_sensors[1].max_value = 10.0f;
    weight_sensors[1].threshold_min = 0.5f;
    weight_sensors[1].threshold_max = 5.0f;
    weight_sensors[1].enabled = true;
    weight_sensors[1].alarm_enabled = false;
    
    initialized = true;
    return ESP_OK;
}

esp_err_t weight_sensor_read(float *weight)
{
    if (!initialized) return ESP_ERR_INVALID_STATE;
    
    *weight = read_weight_from_channel(WEIGHT_ADC_CHANNEL_1, tare_offset_1, calibration_factor_1);
    
    weight_data.weight = *weight;
    weight_data.valid = true;
    
    return ESP_OK;
}

esp_err_t weight_sensor_read_all(void)
{
    if (!initialized) return ESP_ERR_INVALID_STATE;
    
    /* Read both sensors from their respective ADC channels */
    weight_sensors[0].value = read_weight_from_channel(WEIGHT_ADC_CHANNEL_1, tare_offset_1, calibration_factor_1);
    weight_sensors[1].value = read_weight_from_channel(WEIGHT_ADC_CHANNEL_2, tare_offset_2, calibration_factor_2);
    
    weight_data.weight = weight_sensors[0].value;
    weight_data.valid = true;
    
    weight_sensors[0].status = SENSOR_STATUS_OK;
    weight_sensors[1].status = SENSOR_STATUS_OK;
    
    return ESP_OK;
}

weight_data_t weight_sensor_get_data(void)
{
    return weight_data;
}

sensor_data_t* get_weight_sensors(uint8_t *count)
{
    *count = 2;
    return weight_sensors;
}

esp_err_t weight_sensor_tare(void)
{
    int adc_value = adc1_get_raw(WEIGHT_ADC_CHANNEL_1);
    float voltage = (float)adc_value / ADC_MAX * ADC_VREF;
    tare_offset_1 = voltage * calibration_factor_1;
    
    adc_value = adc1_get_raw(WEIGHT_ADC_CHANNEL_2);
    voltage = (float)adc_value / ADC_MAX * ADC_VREF;
    tare_offset_2 = voltage * calibration_factor_2;
    
    ESP_LOGI(TAG, "Weight sensors tared. Offsets: %.2f, %.2f", tare_offset_1, tare_offset_2);
    return ESP_OK;
}

esp_err_t weight_sensor_calibrate(float known_weight)
{
    int adc_value = adc1_get_raw(WEIGHT_ADC_CHANNEL_1);
    float voltage = (float)adc_value / ADC_MAX * ADC_VREF;
    if (voltage > 0.01f) {
        calibration_factor_1 = known_weight / voltage;
    }
    
    adc_value = adc1_get_raw(WEIGHT_ADC_CHANNEL_2);
    voltage = (float)adc_value / ADC_MAX * ADC_VREF;
    if (voltage > 0.01f) {
        calibration_factor_2 = known_weight / voltage;
    }
    
    ESP_LOGI(TAG, "Weight sensors calibrated. Factors: %.4f, %.4f", calibration_factor_1, calibration_factor_2);
    return ESP_OK;
}
