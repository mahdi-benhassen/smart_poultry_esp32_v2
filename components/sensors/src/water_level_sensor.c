#include "sensors/water_level_sensor.h"
#include "sensors/sensor_manager.h"
#include <driver/gpio.h>
#include <esp_log.h>
#include <string.h>

static const char *TAG = "WATER_LEVEL";

/* Tank 1 uses 3 float switches for level detection */
#define WATER_LEVEL_1_LOW_GPIO   GPIO_NUM_32
#define WATER_LEVEL_1_MID_GPIO   GPIO_NUM_33
#define WATER_LEVEL_1_HIGH_GPIO  GPIO_NUM_34

/* Tank 2 uses 3 float switches */
#define WATER_LEVEL_2_LOW_GPIO   GPIO_NUM_35
#define WATER_LEVEL_2_MID_GPIO   GPIO_NUM_36
#define WATER_LEVEL_2_HIGH_GPIO  GPIO_NUM_39

static water_level_data_t water_data = {0};
static sensor_data_t water_sensors[2];
static bool initialized = false;

static float read_water_level(gpio_num_t low, gpio_num_t mid, gpio_num_t high)
{
    int level_low  = gpio_get_level(low);
    int level_mid  = gpio_get_level(mid);
    int level_high = gpio_get_level(high);
    
    int active_count = level_low + level_mid + level_high;
    return (active_count / 3.0f) * 100.0f;
}

esp_err_t water_level_sensor_init(void)
{
    if (initialized) return ESP_OK;
    
    ESP_LOGI(TAG, "Initializing water level sensors");
    
    /* Tank 1 GPIOs */
    gpio_set_direction(WATER_LEVEL_1_LOW_GPIO, GPIO_MODE_INPUT);
    gpio_set_direction(WATER_LEVEL_1_MID_GPIO, GPIO_MODE_INPUT);
    gpio_set_direction(WATER_LEVEL_1_HIGH_GPIO, GPIO_MODE_INPUT);
    
    /* Tank 2 GPIOs */
    gpio_set_direction(WATER_LEVEL_2_LOW_GPIO, GPIO_MODE_INPUT);
    gpio_set_direction(WATER_LEVEL_2_MID_GPIO, GPIO_MODE_INPUT);
    gpio_set_direction(WATER_LEVEL_2_HIGH_GPIO, GPIO_MODE_INPUT);
    
    water_sensors[0].id = 40;
    strncpy(water_sensors[0].name, "Water_Level_1", sizeof(water_sensors[0].name) - 1);
    water_sensors[0].type = SENSOR_TYPE_WATER_LEVEL;
    water_sensors[0].status = SENSOR_STATUS_OK;
    water_sensors[0].value = 0.0f;
    water_sensors[0].min_value = 0.0f;
    water_sensors[0].max_value = 100.0f;
    water_sensors[0].threshold_min = 20.0f;
    water_sensors[0].threshold_max = 100.0f;
    water_sensors[0].enabled = true;
    water_sensors[0].alarm_enabled = true;
    
    water_sensors[1].id = 41;
    strncpy(water_sensors[1].name, "Water_Level_2", sizeof(water_sensors[1].name) - 1);
    water_sensors[1].type = SENSOR_TYPE_WATER_LEVEL;
    water_sensors[1].status = SENSOR_STATUS_OK;
    water_sensors[1].value = 0.0f;
    water_sensors[1].min_value = 0.0f;
    water_sensors[1].max_value = 100.0f;
    water_sensors[1].threshold_min = 20.0f;
    water_sensors[1].threshold_max = 100.0f;
    water_sensors[1].enabled = true;
    water_sensors[1].alarm_enabled = true;
    
    initialized = true;
    return ESP_OK;
}

esp_err_t water_level_sensor_read(float *level, float *percentage)
{
    if (!initialized) return ESP_ERR_INVALID_STATE;
    
    *percentage = read_water_level(WATER_LEVEL_1_LOW_GPIO, WATER_LEVEL_1_MID_GPIO, WATER_LEVEL_1_HIGH_GPIO);
    *level = *percentage / 100.0f * 10.0f;  /* Assume 10 liter tank */
    
    water_data.level = *level;
    water_data.percentage = *percentage;
    water_data.valid = true;
    
    return ESP_OK;
}

esp_err_t water_level_sensor_read_all(void)
{
    if (!initialized) return ESP_ERR_INVALID_STATE;
    
    /* Read both tanks from their respective GPIOs */
    water_sensors[0].value = read_water_level(WATER_LEVEL_1_LOW_GPIO, WATER_LEVEL_1_MID_GPIO, WATER_LEVEL_1_HIGH_GPIO);
    water_sensors[1].value = read_water_level(WATER_LEVEL_2_LOW_GPIO, WATER_LEVEL_2_MID_GPIO, WATER_LEVEL_2_HIGH_GPIO);
    
    water_data.level = water_sensors[0].value / 100.0f * 10.0f;
    water_data.percentage = water_sensors[0].value;
    water_data.valid = true;
    
    water_sensors[0].status = SENSOR_STATUS_OK;
    water_sensors[1].status = SENSOR_STATUS_OK;
    
    return ESP_OK;
}

water_level_data_t water_level_sensor_get_data(void)
{
    return water_data;
}

sensor_data_t* get_water_level_sensors(uint8_t *count)
{
    *count = 2;
    return water_sensors;
}
