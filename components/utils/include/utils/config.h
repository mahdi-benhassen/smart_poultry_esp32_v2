#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>

#define CONFIG_VERSION "1.0.0"

/* Use Kconfig values if available, otherwise provide fallback defaults.
 * These are configurable via: idf.py menuconfig -> Smart Poultry System Configuration
 */
#ifndef CONFIG_MAX_SENSORS
#define CONFIG_MAX_SENSORS 32
#endif

#ifndef CONFIG_MAX_ACTUATORS
#define CONFIG_MAX_ACTUATORS 32
#endif

#ifndef CONFIG_SENSOR_READ_INTERVAL_MS
#define CONFIG_SENSOR_READ_INTERVAL_MS 5000
#endif

#ifndef CONFIG_CONTROL_LOOP_INTERVAL_MS
#define CONFIG_CONTROL_LOOP_INTERVAL_MS 2000
#endif

#ifndef CONFIG_MONITORING_INTERVAL_MS
#define CONFIG_MONITORING_INTERVAL_MS 5000
#endif

#define CONFIG_WIFI_RECONNECT_DELAY_MS 5000
#define CONFIG_MQTT_PUBLISH_INTERVAL_MS 60000
#define CONFIG_MESH_PUBLISH_INTERVAL_MS 30000

typedef struct {
    float temp_min;
    float temp_max;
    float temp_optimal;
    float humidity_min;
    float humidity_max;
    float humidity_optimal;
    float ammonia_max;
    float co2_max;
    float co_max;
    bool auto_control_enabled;
    bool notifications_enabled;
} poultry_config_t;

typedef struct {
    char device_name[64];
    char wifi_ssid[64];
    char wifi_password[64];
    char mqtt_broker[128];
    uint16_t mqtt_port;
    char mqtt_topic[128];
    uint8_t fan_count;
    uint8_t heater_count;
    uint8_t light_count;
    uint8_t feeder_count;
    uint8_t pump_count;
    uint8_t gas_sensor_count;
} system_config_t;

typedef struct {
    bool mesh_enabled;
    char mesh_ssid[32];
    char mesh_password[64];
    uint8_t mesh_max_layer;
    bool mesh_is_root;
    uint8_t mesh_channel;
    bool mesh_auto_join;
    uint8_t mesh_node_id;
} mesh_config_t;

extern poultry_config_t poultry_config;
extern system_config_t system_config;
extern mesh_config_t mesh_config;

void config_init(void);
void config_load(void);
void config_save(void);
void config_reset(void);
void config_update(poultry_config_t *config);
esp_err_t config_set_temperature_range(float min, float max, float optimal);
esp_err_t config_set_humidity_range(float min, float max, float optimal);
esp_err_t config_set_gas_limits(float ammonia, float co2, float co);
esp_err_t config_set_mesh_enabled(bool enabled);
esp_err_t config_set_mesh_config(const char *ssid, const char *password, uint8_t max_layer);
esp_err_t config_set_mesh_as_root(bool is_root);

#endif
