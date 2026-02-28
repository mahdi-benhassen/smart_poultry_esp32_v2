#include <stdio.h>
#include <string.h>
#include <esp_err.h>
#include <esp_log.h>
#include <nvs.h>
#include "utils/config.h"

static const char *TAG = "CONFIG";

poultry_config_t poultry_config = {
    .temp_min = 18.0f,
    .temp_max = 30.0f,
    .temp_optimal = 24.0f,
    .humidity_min = 40.0f,
    .humidity_max = 80.0f,
    .humidity_optimal = 60.0f,
    .ammonia_max = 25.0f,
    .co2_max = 3000.0f,
    .co_max = 50.0f,
    .auto_control_enabled = 1,
    .notifications_enabled = 1
};

system_config_t system_config = {
    .device_name = "SmartPoultry_001",
    .wifi_ssid = "",
    .wifi_password = "",
    .mqtt_broker = "mqtt://localhost",
    .mqtt_port = 1883,
    .mqtt_topic = "poultry/farm",
    .fan_count = 4,
    .heater_count = 2,
    .light_count = 2,
    .feeder_count = 2,
    .pump_count = 2,
    .gas_sensor_count = 3
};

mesh_config_t mesh_config = {
    .mesh_enabled = 0,
    .mesh_ssid = "PoultryFarm",
    .mesh_password = "poultry2024",
    .mesh_max_layer = 4,
    .mesh_is_root = 0,
    .mesh_channel = 0,
    .mesh_auto_join = 1,
    .mesh_node_id = 1
};

static nvs_handle_t nvs_handle;

static esp_err_t save_float(const char *key, float value)
{
    uint32_t val_as_u32;
    memcpy(&val_as_u32, &value, sizeof(float));
    return nvs_set_u32(nvs_handle, key, val_as_u32);
}

static esp_err_t get_float(const char *key, float *value)
{
    uint32_t val_as_u32;
    esp_err_t err = nvs_get_u32(nvs_handle, key, &val_as_u32);
    if (err == ESP_OK) {
        memcpy(value, &val_as_u32, sizeof(float));
    }
    return err;
}

void config_init(void)
{
    ESP_LOGI(TAG, "Initializing configuration");
    esp_err_t err = nvs_open("config", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
        return;
    }
    config_load();
}

void config_load(void)
{
    ESP_LOGI(TAG, "Loading configuration from NVS");
    
    size_t len = sizeof(system_config.device_name);
    nvs_get_str(nvs_handle, "device_name", system_config.device_name, &len);
    
    len = sizeof(system_config.wifi_ssid);
    nvs_get_str(nvs_handle, "wifi_ssid", system_config.wifi_ssid, &len);
    
    len = sizeof(system_config.wifi_password);
    nvs_get_str(nvs_handle, "wifi_password", system_config.wifi_password, &len);
    
    len = sizeof(system_config.mqtt_broker);
    nvs_get_str(nvs_handle, "mqtt_broker", system_config.mqtt_broker, &len);
    
    nvs_get_u16(nvs_handle, "mqtt_port", &system_config.mqtt_port);
    
    len = sizeof(system_config.mqtt_topic);
    nvs_get_str(nvs_handle, "mqtt_topic", system_config.mqtt_topic, &len);
    
    get_float("temp_min", &poultry_config.temp_min);
    get_float("temp_max", &poultry_config.temp_max);
    get_float("temp_optimal", &poultry_config.temp_optimal);
    get_float("humidity_min", &poultry_config.humidity_min);
    get_float("humidity_max", &poultry_config.humidity_max);
    get_float("humidity_optimal", &poultry_config.humidity_optimal);
    get_float("ammonia_max", &poultry_config.ammonia_max);
    get_float("co2_max", &poultry_config.co2_max);
    get_float("co_max", &poultry_config.co_max);
    
    uint8_t auto_ctrl = 1;
    nvs_get_u8(nvs_handle, "auto_control", &auto_ctrl);
    poultry_config.auto_control_enabled = (auto_ctrl == 1);
    
    uint8_t notif = 1;
    nvs_get_u8(nvs_handle, "notifications", &notif);
    poultry_config.notifications_enabled = (notif == 1);
    
    uint8_t mesh_en = 0;
    nvs_get_u8(nvs_handle, "mesh_enabled", &mesh_en);
    mesh_config.mesh_enabled = (mesh_en == 1);
    
    len = sizeof(mesh_config.mesh_ssid);
    nvs_get_str(nvs_handle, "mesh_ssid", mesh_config.mesh_ssid, &len);
    
    len = sizeof(mesh_config.mesh_password);
    nvs_get_str(nvs_handle, "mesh_password", mesh_config.mesh_password, &len);
    
    nvs_get_u8(nvs_handle, "mesh_max_layer", &mesh_config.mesh_max_layer);
    uint8_t mesh_is_root_u8 = 0;
    nvs_get_u8(nvs_handle, "mesh_is_root", &mesh_is_root_u8);
    mesh_config.mesh_is_root = (mesh_is_root_u8 == 1);
    
    ESP_LOGI(TAG, "Configuration loaded successfully");
}

void config_save(void)
{
    ESP_LOGI(TAG, "Saving configuration to NVS");
    
    nvs_set_str(nvs_handle, "device_name", system_config.device_name);
    nvs_set_str(nvs_handle, "wifi_ssid", system_config.wifi_ssid);
    nvs_set_str(nvs_handle, "wifi_password", system_config.wifi_password);
    nvs_set_str(nvs_handle, "mqtt_broker", system_config.mqtt_broker);
    nvs_set_u16(nvs_handle, "mqtt_port", system_config.mqtt_port);
    nvs_set_str(nvs_handle, "mqtt_topic", system_config.mqtt_topic);
    
    save_float("temp_min", poultry_config.temp_min);
    save_float("temp_max", poultry_config.temp_max);
    save_float("temp_optimal", poultry_config.temp_optimal);
    save_float("humidity_min", poultry_config.humidity_min);
    save_float("humidity_max", poultry_config.humidity_max);
    save_float("humidity_optimal", poultry_config.humidity_optimal);
    save_float("ammonia_max", poultry_config.ammonia_max);
    save_float("co2_max", poultry_config.co2_max);
    save_float("co_max", poultry_config.co_max);
    nvs_set_u8(nvs_handle, "auto_control", poultry_config.auto_control_enabled ? 1 : 0);
    nvs_set_u8(nvs_handle, "notifications", poultry_config.notifications_enabled ? 1 : 0);
    
    nvs_set_u8(nvs_handle, "mesh_enabled", mesh_config.mesh_enabled ? 1 : 0);
    nvs_set_str(nvs_handle, "mesh_ssid", mesh_config.mesh_ssid);
    nvs_set_str(nvs_handle, "mesh_password", mesh_config.mesh_password);
    nvs_set_u8(nvs_handle, "mesh_max_layer", mesh_config.mesh_max_layer);
    nvs_set_u8(nvs_handle, "mesh_is_root", mesh_config.mesh_is_root ? 1 : 0);
    
    nvs_commit(nvs_handle);
    ESP_LOGI(TAG, "Configuration saved successfully");
}

void config_reset(void)
{
    ESP_LOGI(TAG, "Resetting configuration to defaults");
    nvs_erase_all(nvs_handle);
    nvs_commit(nvs_handle);
    config_load();
}

void config_update(poultry_config_t *config)
{
    if (config == NULL) return;
    memcpy(&poultry_config, config, sizeof(poultry_config_t));
    config_save();
}

esp_err_t config_set_temperature_range(float min, float max, float optimal)
{
    if (min > max || optimal < min || optimal > max) {
        return ESP_ERR_INVALID_ARG;
    }
    poultry_config.temp_min = min;
    poultry_config.temp_max = max;
    poultry_config.temp_optimal = optimal;
    config_save();
    return ESP_OK;
}

esp_err_t config_set_humidity_range(float min, float max, float optimal)
{
    if (min > max || optimal < min || optimal > max) {
        return ESP_ERR_INVALID_ARG;
    }
    poultry_config.humidity_min = min;
    poultry_config.humidity_max = max;
    poultry_config.humidity_optimal = optimal;
    config_save();
    return ESP_OK;
}

esp_err_t config_set_gas_limits(float ammonia, float co2, float co)
{
    if (ammonia < 0 || co2 < 0 || co < 0) {
        return ESP_ERR_INVALID_ARG;
    }
    poultry_config.ammonia_max = ammonia;
    poultry_config.co2_max = co2;
    poultry_config.co_max = co;
    config_save();
    return ESP_OK;
}

esp_err_t config_set_mesh_enabled(bool enabled)
{
    mesh_config.mesh_enabled = enabled;
    config_save();
    return ESP_OK;
}

esp_err_t config_set_mesh_config(const char *ssid, const char *password, uint8_t max_layer)
{
    if (ssid) strncpy(mesh_config.mesh_ssid, ssid, sizeof(mesh_config.mesh_ssid) - 1);
    if (password) strncpy(mesh_config.mesh_password, password, sizeof(mesh_config.mesh_password) - 1);
    if (max_layer > 0 && max_layer <= 10) mesh_config.mesh_max_layer = max_layer;
    config_save();
    return ESP_OK;
}

esp_err_t config_set_mesh_as_root(bool is_root)
{
    mesh_config.mesh_is_root = is_root;
    config_save();
    return ESP_OK;
}
