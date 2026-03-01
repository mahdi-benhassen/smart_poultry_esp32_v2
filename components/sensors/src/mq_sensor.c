#include "sensors/mq_sensor.h"
#include "sensors/sensor_manager.h"
#include <esp_adc/adc_oneshot.h>
#include <esp_log.h>
#include <math.h>
#include <string.h>

static const char *TAG = "MQ_SENSOR";

#define MQ2_CHANNEL ADC1_CHANNEL_3
#define MQ135_CHANNEL ADC1_CHANNEL_4
#define MQ7_CHANNEL ADC1_CHANNEL_5

/* ESP32 ADC reference voltage is 3.3V */
#define ADC_VREF 3.3f
#define ADC_MAX  4095.0f
/* Load resistor value in kOhm */
#define RL_VALUE 10.0f

static mq_sensor_data_t mq_data = {0};
static sensor_data_t mq_sensors[4];
static bool initialized = false;

static float MQ2_R0 = 10.0f;
static float MQ135_R0 = 100.0f;
static float MQ7_R0 = 26.0f;

static float mq_read_raw(adc_channel_t channel)
{
    int adc_value = 0;
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, channel, &adc_value));
    return (float)adc_value;
}

static float mq_calculate_rs(float adc_raw)
{
    float voltage = (adc_raw / ADC_MAX) * ADC_VREF;
    if (voltage < 0.01f) voltage = 0.01f; /* prevent div by zero */
    return ((ADC_VREF - voltage) / voltage) * RL_VALUE;
}

esp_err_t mq_sensor_init(void)
{
    if (initialized) return ESP_OK;
    
    ESP_LOGI(TAG, "Initializing MQ sensors");
    
    mq_sensors[0].id = 10;
    strncpy(mq_sensors[0].name, "Ammonia_Sensor", sizeof(mq_sensors[0].name) - 1);
    mq_sensors[0].type = SENSOR_TYPE_AMMONIA;
    mq_sensors[0].status = SENSOR_STATUS_OK;
    mq_sensors[0].value = 5.0f;
    mq_sensors[0].min_value = 0.0f;
    mq_sensors[0].max_value = 500.0f;
    mq_sensors[0].threshold_min = 0.0f;
    mq_sensors[0].threshold_max = 25.0f;
    mq_sensors[0].enabled = true;
    mq_sensors[0].alarm_enabled = true;
    
    mq_sensors[1].id = 11;
    strncpy(mq_sensors[1].name, "CO2_Sensor", sizeof(mq_sensors[1].name) - 1);
    mq_sensors[1].type = SENSOR_TYPE_CO2;
    mq_sensors[1].status = SENSOR_STATUS_OK;
    mq_sensors[1].value = 400.0f;
    mq_sensors[1].min_value = 0.0f;
    mq_sensors[1].max_value = 10000.0f;
    mq_sensors[1].threshold_min = 0.0f;
    mq_sensors[1].threshold_max = 3000.0f;
    mq_sensors[1].enabled = true;
    mq_sensors[1].alarm_enabled = true;
    
    mq_sensors[2].id = 12;
    strncpy(mq_sensors[2].name, "CO_Sensor", sizeof(mq_sensors[2].name) - 1);
    mq_sensors[2].type = SENSOR_TYPE_CO;
    mq_sensors[2].status = SENSOR_STATUS_OK;
    mq_sensors[2].value = 2.0f;
    mq_sensors[2].min_value = 0.0f;
    mq_sensors[2].max_value = 500.0f;
    mq_sensors[2].threshold_min = 0.0f;
    mq_sensors[2].threshold_max = 50.0f;
    mq_sensors[2].enabled = true;
    mq_sensors[2].alarm_enabled = true;
    
    mq_sensors[3].id = 13;
    strncpy(mq_sensors[3].name, "Methane_Sensor", sizeof(mq_sensors[3].name) - 1);
    mq_sensors[3].type = SENSOR_TYPE_METHANE;
    mq_sensors[3].status = SENSOR_STATUS_OK;
    mq_sensors[3].value = 0.5f;
    mq_sensors[3].min_value = 0.0f;
    mq_sensors[3].max_value = 100.0f;
    mq_sensors[3].threshold_min = 0.0f;
    mq_sensors[3].threshold_max = 20.0f;
    mq_sensors[3].enabled = true;
    mq_sensors[3].alarm_enabled = true;
    
    initialized = true;
    return ESP_OK;
}

esp_err_t mq_sensor_read(float *ammonia, float *co2, float *co)
{
    if (!initialized) return ESP_ERR_INVALID_STATE;
    
    float rs_mq2 = mq_calculate_rs(mq_read_raw(MQ2_CHANNEL));
    float rs_mq135 = mq_calculate_rs(mq_read_raw(MQ135_CHANNEL));
    float rs_mq7 = mq_calculate_rs(mq_read_raw(MQ7_CHANNEL));
    
    /* Simplified ppm calculation based on Rs/R0 ratio */
    *ammonia = (rs_mq2 / MQ2_R0) * 10.0f;
    *co2 = (rs_mq135 / MQ135_R0) * 100.0f;
    *co = (rs_mq7 / MQ7_R0) * 5.0f;
    
    /* Also estimate methane from MQ2 */
    mq_data.methane = (rs_mq2 / MQ2_R0) * 5.0f;
    
    mq_data.ammonia = *ammonia;
    mq_data.co2 = *co2;
    mq_data.co = *co;
    mq_data.valid = true;
    
    return ESP_OK;
}

esp_err_t mq_sensor_read_all(void)
{
    float ammonia = 0, co2 = 0, co = 0;
    esp_err_t ret = mq_sensor_read(&ammonia, &co2, &co);
    
    mq_sensors[0].value = ammonia;
    mq_sensors[1].value = co2;
    mq_sensors[2].value = co;
    mq_sensors[3].value = mq_data.methane;
    
    if (ret != ESP_OK) {
        for (int i = 0; i < 4; i++) {
            mq_sensors[i].status = SENSOR_STATUS_ERROR;
        }
    } else {
        for (int i = 0; i < 4; i++) {
            mq_sensors[i].status = SENSOR_STATUS_OK;
        }
    }
    
    return ret;
}

mq_sensor_data_t mq_sensor_get_data(void)
{
    return mq_data;
}

sensor_data_t* get_mq_sensors(uint8_t *count)
{
    *count = 4;
    return mq_sensors;
}

esp_err_t mq_sensor_calibrate(mq_sensor_type_t type, float clean_air_r0)
{
    if (clean_air_r0 <= 0.0f) {
        ESP_LOGE(TAG, "Invalid R0 value: %.2f", clean_air_r0);
        return ESP_ERR_INVALID_ARG;
    }
    
    switch (type) {
        case MQ_TYPE_MQ2:
            MQ2_R0 = clean_air_r0;
            break;
        case MQ_TYPE_MQ135:
            MQ135_R0 = clean_air_r0;
            break;
        case MQ_TYPE_MQ7:
            MQ7_R0 = clean_air_r0;
            break;
        default:
            ESP_LOGW(TAG, "Calibration for sensor type %d not implemented", type);
            return ESP_ERR_NOT_SUPPORTED;
    }
    
    ESP_LOGI(TAG, "Calibrated MQ sensor type: %d, R0: %.2f", type, clean_air_r0);
    return ESP_OK;
}
