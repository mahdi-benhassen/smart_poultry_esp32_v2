#include "sensors/dht22.h"
#include "sensors/sensor_manager.h"
#include <driver/gpio.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <rom/ets_sys.h>
#include <string.h>

static const char *TAG = "DHT22";

#define DHT22_PIN GPIO_NUM_15

static dht22_data_t dht_data = {0};
static sensor_data_t dht22_sensors[2];
static bool initialized = false;

esp_err_t dht22_init(void)
{
    if (initialized) return ESP_OK;
    
    ESP_LOGI(TAG, "Initializing DHT22 sensor");
    
    gpio_set_direction(DHT22_PIN, GPIO_MODE_INPUT_OUTPUT_OD);
    
    dht22_sensors[0].id = 0;
    strncpy(dht22_sensors[0].name, "Temperature_1", sizeof(dht22_sensors[0].name) - 1);
    dht22_sensors[0].type = SENSOR_TYPE_TEMPERATURE;
    dht22_sensors[0].status = SENSOR_STATUS_OK;
    dht22_sensors[0].value = 25.0f;
    dht22_sensors[0].min_value = -40.0f;
    dht22_sensors[0].max_value = 80.0f;
    dht22_sensors[0].threshold_min = 18.0f;
    dht22_sensors[0].threshold_max = 30.0f;
    dht22_sensors[0].enabled = true;
    dht22_sensors[0].alarm_enabled = true;
    
    dht22_sensors[1].id = 1;
    strncpy(dht22_sensors[1].name, "Humidity_1", sizeof(dht22_sensors[1].name) - 1);
    dht22_sensors[1].type = SENSOR_TYPE_HUMIDITY;
    dht22_sensors[1].status = SENSOR_STATUS_OK;
    dht22_sensors[1].value = 60.0f;
    dht22_sensors[1].min_value = 0.0f;
    dht22_sensors[1].max_value = 100.0f;
    dht22_sensors[1].threshold_min = 40.0f;
    dht22_sensors[1].threshold_max = 80.0f;
    dht22_sensors[1].enabled = true;
    dht22_sensors[1].alarm_enabled = true;
    
    initialized = true;
    return ESP_OK;
}

static esp_err_t dht22_read_raw(float *temperature, float *humidity)
{
    uint8_t data[5] = {0};
    
    gpio_set_direction(DHT22_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(DHT22_PIN, 0);
    vTaskDelay(20 / portTICK_PERIOD_MS);
    
    gpio_set_level(DHT22_PIN, 1);
    ets_delay_us(30);
    
    gpio_set_direction(DHT22_PIN, GPIO_MODE_INPUT);
    
    int timeout = 0;
    while (gpio_get_level(DHT22_PIN) == 0) {
        if (++timeout > 100) return ESP_ERR_TIMEOUT;
        ets_delay_us(1);
    }
    
    timeout = 0;
    while (gpio_get_level(DHT22_PIN) == 1) {
        if (++timeout > 100) return ESP_ERR_TIMEOUT;
        ets_delay_us(1);
    }
    
    for (int i = 0; i < 40; i++) {
        timeout = 0;
        while (gpio_get_level(DHT22_PIN) == 0) {
            if (++timeout > 100) return ESP_ERR_TIMEOUT;
            ets_delay_us(1);
        }
        
        ets_delay_us(30);
        
        int bit = gpio_get_level(DHT22_PIN);
        data[i / 8] = (data[i / 8] << 1) | bit;
        
        timeout = 0;
        while (gpio_get_level(DHT22_PIN) == 1) {
            if (++timeout > 100) return ESP_ERR_TIMEOUT;
            ets_delay_us(1);
        }
    }
    
    uint8_t checksum = data[0] + data[1] + data[2] + data[3];
    if (checksum != data[4]) {
        ESP_LOGE(TAG, "Checksum failed");
        return ESP_ERR_INVALID_CRC;
    }
    
    *humidity = ((data[0] << 8) | data[1]) / 10.0f;
    *temperature = (((data[2] & 0x7F) << 8) | data[3]) / 10.0f;
    if (data[2] & 0x80) {
        *temperature = -*temperature;
    }
    
    return ESP_OK;
}

esp_err_t dht22_read(float *temperature, float *humidity)
{
    esp_err_t ret = dht22_read_raw(temperature, humidity);
    if (ret == ESP_OK) {
        dht_data.temperature = *temperature;
        dht_data.humidity = *humidity;
        dht_data.valid = true;
    } else {
        dht_data.valid = false;
    }
    return ret;
}

esp_err_t dht22_read_all(void)
{
    float temp = 0, hum = 0;
    esp_err_t ret = dht22_read(&temp, &hum);
    
    dht22_sensors[0].value = temp;
    dht22_sensors[1].value = hum;
    
    if (ret != ESP_OK) {
        dht22_sensors[0].status = SENSOR_STATUS_ERROR;
        dht22_sensors[1].status = SENSOR_STATUS_ERROR;
    } else {
        dht22_sensors[0].status = SENSOR_STATUS_OK;
        dht22_sensors[1].status = SENSOR_STATUS_OK;
    }
    
    return ret;
}

dht22_data_t dht22_get_data(void)
{
    return dht_data;
}

sensor_data_t* get_dht22_sensors(uint8_t *count)
{
    *count = 2;
    return dht22_sensors;
}
