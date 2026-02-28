#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>
#include <esp_log.h>
#include <esp_chip_info.h>
#include <esp_flash.h>
#include <nvs_flash.h>

#include "sensors/sensor_manager.h"
#include "actuators/actuator_manager.h"
#include "control/control_system.h"
#include "monitoring/monitoring.h"
#include "communication/communication.h"
#include "utils/config.h"

static const char *TAG = "MAIN";

void app_main(void)
{
    ESP_LOGI(TAG, "Smart Poultry System v1.0.0");
    ESP_LOGI(TAG, "Starting initialization...");

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    config_init();
    
    sensor_manager_init();
    actuator_manager_init();
    control_system_init();
    monitoring_init();
    communication_init();

    control_system_start();
    monitoring_start();
    communication_start();

    ESP_LOGI(TAG, "All systems initialized successfully");
    ESP_LOGI(TAG, "Free heap size: %lu bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "CPU cores: %d", portNUM_PROCESSORS);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
