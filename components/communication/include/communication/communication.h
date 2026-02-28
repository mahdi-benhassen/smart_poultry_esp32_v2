#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>

typedef enum {
    COMM_MODE_WIFI,
    COMM_MODE_ETHERNET,
    COMM_MODE_LORA,
    COMM_MODE_MESH
} comm_mode_t;

typedef enum {
    COMM_STATUS_DISCONNECTED,
    COMM_STATUS_CONNECTING,
    COMM_STATUS_CONNECTED,
    COMM_STATUS_ERROR
} comm_status_t;

typedef struct {
    comm_mode_t mode;
    comm_status_t status;
    char ip_address[16];
    int32_t rssi;
    uint32_t bytes_sent;
    uint32_t bytes_received;
    uint32_t last_update;
    bool mesh_enabled;
    uint8_t mesh_layer;
    uint8_t mesh_node_count;
    uint8_t mesh_max_layer;
} comm_info_t;

esp_err_t communication_init(void);
esp_err_t communication_start(void);
esp_err_t communication_stop(void);
esp_err_t communication_connect_wifi(const char *ssid, const char *password);
esp_err_t communication_disconnect(void);
esp_err_t communication_get_status(comm_info_t *info);
esp_err_t communication_send_data(const char *topic, const char *data);
esp_err_t communication_subscribe(const char *topic);
esp_err_t communication_set_mqtt_config(const char *broker, uint16_t port, const char *topic);
esp_err_t communication_publish_sensor_data(void);
esp_err_t communication_init_mesh(const char *mesh_ssid, const char *mesh_password, uint8_t max_layer);
esp_err_t communication_mesh_send(uint8_t *data, uint16_t len);
esp_err_t communication_mesh_broadcast(uint8_t *data, uint16_t len);
esp_err_t communication_mesh_get_nodes(uint8_t *node_count);
esp_err_t communication_set_mesh_parent(const char *parent_mac);

#endif
