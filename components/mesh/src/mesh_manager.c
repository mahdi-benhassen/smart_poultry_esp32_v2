#include "mesh/mesh_manager.h"
#include <esp_log.h>
#include <esp_mac.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char *TAG = "MESH_MGR";

#define MESH_PREFIX "PoultryFarm"
#define MESH_PASSWORD "poultry2024"
#define MESH_ID {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}
#define MESH_MAX_NODES 50

static volatile bool initialized = false;
static volatile bool running = false;
static mesh_status_t mesh_status = {0};
static mesh_data_callback_t data_callback = NULL;
static mesh_event_callback_t event_callback = NULL;
static mesh_node_info_t mesh_nodes[MESH_MAX_NODES];
static uint8_t mesh_node_count = 0;

static void mesh_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    switch (event_id) {
        case MESH_EVENT_STARTED:
            ESP_LOGI(TAG, "Mesh network started");
            mesh_status.is_root_elected = false;
            break;
            
        case MESH_EVENT_STOPPED:
            ESP_LOGI(TAG, "Mesh network stopped");
            running = false;
            break;
            
        case MESH_EVENT_CHILD_CONNECTED: {
            mesh_event_child_connected_t *child_conn = (mesh_event_child_connected_t *)event_data;
            ESP_LOGI(TAG, "Child connected: " MACSTR, MAC2STR(child_conn->mac));
            if (mesh_node_count < MESH_MAX_NODES) {
                memcpy(mesh_nodes[mesh_node_count].mac, child_conn->mac, 6);
                mesh_nodes[mesh_node_count].is_connected = true;
                mesh_node_count++;
            }
            mesh_status.connected_nodes = mesh_node_count;
            if (event_callback) {
                event_callback(&mesh_status);
            }
            break;
        }
            
        case MESH_EVENT_CHILD_DISCONNECTED: {
            mesh_event_child_disconnected_t *child_disconn = (mesh_event_child_disconnected_t *)event_data;
            ESP_LOGI(TAG, "Child disconnected: " MACSTR, MAC2STR(child_disconn->mac));
            for (int i = 0; i < mesh_node_count; i++) {
                if (memcmp(mesh_nodes[i].mac, child_disconn->mac, 6) == 0) {
                    mesh_nodes[i].is_connected = false;
                    break;
                }
            }
            mesh_status.connected_nodes = mesh_node_count;
            if (event_callback) {
                event_callback(&mesh_status);
            }
            break;
        }
            
        case MESH_EVENT_LAYER_CHANGE: {
            mesh_event_layer_change_t *layer_change = (mesh_event_layer_change_t *)event_data;
            mesh_status.current_layer = layer_change->new_layer;
            ESP_LOGI(TAG, "Layer changed to %d", mesh_status.current_layer);
            if (event_callback) {
                event_callback(&mesh_status);
            }
            break;
        }
            
        case IP_EVENT_STA_GOT_IP: {
            ip_event_got_ip_t *got_ip = (ip_event_got_ip_t *)event_data;
            ESP_LOGI(TAG, "Root got IP: " IPSTR, IP2STR(&got_ip->ip_info.ip));
            mesh_status.is_root_elected = true;
            mesh_status.role = MESH_ROLE_ROOT;
            if (event_callback) {
                event_callback(&mesh_status);
            }
            break;
        }
            
        case IP_EVENT_STA_LOST_IP:
            ESP_LOGI(TAG, "Root lost IP");
            mesh_status.is_root_elected = false;
            break;
            
        case MESH_EVENT_NO_PARENT_FOUND:
            ESP_LOGW(TAG, "No parent found");
            break;
            
        default:
            break;
    }
}

esp_err_t mesh_manager_init(const char *ssid, const char *password, uint8_t max_layer)
{
    memset(mesh_nodes, 0, sizeof(mesh_nodes));
    mesh_node_count = 0;
    
    strncpy(mesh_status.mesh_ssid, ssid ? ssid : MESH_PREFIX, sizeof(mesh_status.mesh_ssid) - 1);
    strncpy(mesh_status.mesh_password, password ? password : MESH_PASSWORD, sizeof(mesh_status.mesh_password) - 1);
    mesh_status.max_layer = max_layer > 0 ? max_layer : 4;
    mesh_status.topology = MESH_TOPO_TREE;
    mesh_status.role = MESH_ROLE_CHILD;
    mesh_status.current_layer = 0;
    mesh_status.connected_nodes = 0;
    mesh_status.is_root_elected = false;
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    ESP_ERROR_CHECK(esp_event_handler_register(MESH_EVENT, ESP_EVENT_ANY_ID, &mesh_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &mesh_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_LOST_IP, &mesh_event_handler, NULL));
    
    mesh_cfg_t mesh_config = MESH_INIT_CONFIG_DEFAULT();
    mesh_config.channel = 0;
    uint8_t mesh_id[6] = MESH_ID;
    memcpy(mesh_config.mesh_id.addr, mesh_id, 6);
    mesh_config.router.ssid_len = strlen(mesh_status.mesh_ssid);
    strncpy((char *)mesh_config.router.ssid, mesh_status.mesh_ssid, sizeof(mesh_config.router.ssid));
    strncpy((char *)mesh_config.router.password, mesh_status.mesh_password, sizeof(mesh_config.router.password));
    mesh_config.mesh_ap.max_connection = 6;
    mesh_config.mesh_ap.nonmesh_max_connection = 4;
    strncpy((char *)mesh_config.mesh_ap.password, mesh_status.mesh_password, sizeof(mesh_config.mesh_ap.password));
    
    ESP_ERROR_CHECK(esp_mesh_set_config(&mesh_config));
    ESP_ERROR_CHECK(esp_mesh_set_vote_percentage(1));
    ESP_ERROR_CHECK(esp_mesh_set_xon_qsize(128));
    
    initialized = true;
    ESP_LOGI(TAG, "Mesh manager initialized");
    ESP_LOGI(TAG, "Mesh SSID: %s, Max Layer: %d", mesh_status.mesh_ssid, mesh_status.max_layer);
    
    return ESP_OK;
}

esp_err_t mesh_manager_start(void)
{
    if (!initialized) return ESP_ERR_INVALID_STATE;
    if (running) return ESP_OK;
    
    ESP_LOGI(TAG, "Starting mesh network");
    
    ESP_ERROR_CHECK(esp_mesh_start());
    
    running = true;
    ESP_LOGI(TAG, "Mesh network started successfully");
    
    return ESP_OK;
}

esp_err_t mesh_manager_stop(void)
{
    if (!running) return ESP_OK;
    
    ESP_LOGI(TAG, "Stopping mesh network");
    
    ESP_ERROR_CHECK(esp_mesh_stop());
    
    running = false;
    mesh_node_count = 0;
    
    ESP_LOGI(TAG, "Mesh network stopped");
    
    return ESP_OK;
}

esp_err_t mesh_manager_send_to_parent(uint8_t *data, uint16_t len)
{
    if (!running || !data || len == 0) return ESP_ERR_INVALID_ARG;
    
    mesh_data_t mesh_data = {
        .data = data,
        .size = len,
        .proto = MESH_PROTO_BIN,
        .tos = MESH_TOS_P2P
    };
    
    mesh_addr_t parent_addr = {0};
    esp_err_t err = esp_mesh_send(&parent_addr, &mesh_data, 0, NULL, 0);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Send to parent failed: %s", esp_err_to_name(err));
    }
    
    return err;
}

esp_err_t mesh_manager_broadcast(uint8_t *data, uint16_t len)
{
    if (!running || !data || len == 0) return ESP_ERR_INVALID_ARG;
    
    mesh_data_t mesh_data = {
        .data = data,
        .size = len,
        .proto = MESH_PROTO_BIN,
        .tos = MESH_TOS_P2P
    };
    
    esp_err_t err = esp_mesh_send(NULL, &mesh_data, MESH_DATA_P2P, NULL, 0);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Broadcast failed: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "Broadcast success: %d bytes", len);
    }
    
    return err;
}

esp_err_t mesh_manager_send_to_node(mesh_addr_t *target, uint8_t *data, uint16_t len)
{
    if (!running || !target || !data || len == 0) return ESP_ERR_INVALID_ARG;
    
    mesh_data_t mesh_data = {
        .data = data,
        .size = len,
        .proto = MESH_PROTO_BIN,
        .tos = MESH_TOS_P2P
    };
    
    esp_err_t err = esp_mesh_send(target, &mesh_data, 0, NULL, 0);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Send to node failed: %s", esp_err_to_name(err));
    }
    
    return err;
}

esp_err_t mesh_manager_get_status(mesh_status_t *status)
{
    if (!initialized) return ESP_ERR_INVALID_STATE;
    
    /* Update live values BEFORE copying to caller */
    mesh_status.current_layer = esp_mesh_get_layer();
    mesh_status.connected_nodes = esp_mesh_get_routing_table_size();
    
    memcpy(status, &mesh_status, sizeof(mesh_status_t));
    
    return ESP_OK;
}

esp_err_t mesh_manager_get_nodes(mesh_node_info_t *nodes, uint8_t *count)
{
    if (!initialized) return ESP_ERR_INVALID_STATE;
    
    if (nodes && count) {
        memcpy(nodes, mesh_nodes, sizeof(mesh_node_info_t) * mesh_node_count);
        *count = mesh_node_count;
    }
    
    return ESP_OK;
}

esp_err_t mesh_manager_force_root(void)
{
    if (!initialized) return ESP_ERR_INVALID_STATE;
    
    ESP_LOGI(TAG, "Forcing this node as root");
    
    esp_err_t err = esp_mesh_set_type(MESH_ROOT);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set root: %s", esp_err_to_name(err));
    }
    
    return err;
}

esp_err_t mesh_manager_set_parent(const char *parent_mac)
{
    if (!initialized || !parent_mac) return ESP_ERR_INVALID_ARG;
    
    wifi_config_t parent = {0};
    mesh_addr_t parent_mesh_id = {0};
    
    int mac[6];
    sscanf(parent_mac, "%02x:%02x:%02x:%02x:%02x:%02x", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
    
    parent.sta.bssid_set = 1;
    for (int i = 0; i < 6; i++) {
        parent.sta.bssid[i] = (uint8_t)mac[i];
    }
    
    ESP_LOGI(TAG, "Setting parent to: %s", parent_mac);
    
    return esp_mesh_set_parent(&parent, &parent_mesh_id, MESH_NODE, 1);
}

esp_err_t mesh_manager_set_topology(esp_mesh_topology_t topo)
{
    if (!initialized) return ESP_ERR_INVALID_STATE;
    
    mesh_status.topology = topo;
    
    wifi_ps_type_t ps_type = (topo == MESH_TOPO_CHAIN) ? WIFI_PS_NONE : WIFI_PS_MIN_MODEM;
    
    return esp_wifi_set_ps(ps_type);
}

esp_err_t mesh_manager_set_data_callback(mesh_data_callback_t callback)
{
    data_callback = callback;
    return ESP_OK;
}

esp_err_t mesh_manager_set_event_callback(mesh_event_callback_t callback)
{
    event_callback = callback;
    return ESP_OK;
}

esp_err_t mesh_manager_heal_network(void)
{
    if (!running) return ESP_ERR_INVALID_STATE;
    
    ESP_LOGI(TAG, "Healing mesh network");
    
    return ESP_OK;
}

esp_err_t mesh_manager_get_routing_table(mesh_addr_t *table, uint16_t *count)
{
    if (!running) return ESP_ERR_INVALID_STATE;
    
    if (!table || !count) return ESP_ERR_INVALID_ARG;
    
    int size = 0;
    esp_err_t err = esp_mesh_get_routing_table(table, MESH_MAX_NODES * 6, &size);
    *count = (uint16_t)size;
    return err;
}
