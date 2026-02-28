#ifndef MESH_MANAGER_H
#define MESH_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>
#include <esp_mesh.h>
#include <esp_mesh_internal.h>

typedef enum {
    MESH_ROLE_ROOT,
    MESH_ROLE_CHILD,
    MESH_ROLE_LEAF
} mesh_role_t;


typedef struct {
    uint8_t mac[6];
    uint8_t layer;
    bool is_root;
    bool is_connected;
    int32_t rssi;
} mesh_node_info_t;

typedef struct {
    mesh_role_t role;
    mesh_topology_t topology;
    uint8_t max_layer;
    uint8_t current_layer;
    uint8_t connected_nodes;
    bool is_root_elected;
    mesh_addr_t parent_addr;
    char mesh_ssid[32];
    char mesh_password[64];
} mesh_status_t;

typedef void (*mesh_data_callback_t)(mesh_addr_t *sender, uint8_t *data, uint16_t len);
typedef void (*mesh_event_callback_t)(mesh_status_t *status);

esp_err_t mesh_manager_init(const char *ssid, const char *password, uint8_t max_layer);
esp_err_t mesh_manager_start(void);
esp_err_t mesh_manager_stop(void);
esp_err_t mesh_manager_send_to_parent(uint8_t *data, uint16_t len);
esp_err_t mesh_manager_broadcast(uint8_t *data, uint16_t len);
esp_err_t mesh_manager_send_to_node(mesh_addr_t *target, uint8_t *data, uint16_t len);
esp_err_t mesh_manager_get_status(mesh_status_t *status);
esp_err_t mesh_manager_get_nodes(mesh_node_info_t *nodes, uint8_t *count);
esp_err_t mesh_manager_force_root(void);
esp_err_t mesh_manager_set_parent(const char *parent_mac);
esp_err_t mesh_manager_set_topology(mesh_topology_t topo);
esp_err_t mesh_manager_set_data_callback(mesh_data_callback_t callback);
esp_err_t mesh_manager_set_event_callback(mesh_event_callback_t callback);
esp_err_t mesh_manager_heal_network(void);
esp_err_t mesh_manager_get_routing_table(mesh_addr_t *table, uint16_t *count);

#endif
