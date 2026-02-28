#include "mesh/mesh_manager.h"
#include <esp_log.h>
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
    mesh_event_t *event = (mesh_event_t *)event_data;
    
    switch (event_id) {
        case MESH_EVENT_STARTED:
            ESP_LOGI(TAG, "Mesh network started");
            mesh_status.is_root_elected = false;
            break;
            
        case MESH_EVENT_STOPPED:
            ESP_LOGI(TAG, "Mesh network stopped");
            running = false;
            break;
            
        case MESH_EVENT_CHILD_CONNECTED:
            ESP_LOGI(TAG, "Child connected: " MACSTR, MAC2STR(event->info.child_connected.mac));
            if (mesh_node_count < MESH_MAX_NODES) {
                memcpy(mesh_nodes[mesh_node_count].mac, event->info.child_connected.mac, 6);
                mesh_nodes[mesh_node_count].is_connected = true;
                mesh_node_count++;
            }
            mesh_status.connected_nodes = mesh_node_count;
            if (event_callback) {
                event_callback(&mesh_status);
            }
            break;
            
        case MESH_EVENT_CHILD_DISCONNECTED:
            ESP_LOGI(TAG, "Child disconnected: " MACSTR, MAC2STR(event->info.child_disconnected.mac));
            for (int i = 0; i < mesh_node_count; i++) {
                if (memcmp(mesh_nodes[i].mac, event->info.child_disconnected.mac, 6) == 0) {
                    mesh_nodes[i].is_connected = false;
                    break;
                }
            }
            mesh_status.connected_nodes = mesh_node_count;
            if (event_callback) {
                event_callback(&mesh_status);
            }
            break;
            
        case MESH_EVENT_LAYER_CHANGE:
            mesh_status.current_layer = event->info.layer_change.new_layer;
            ESP_LOGI(TAG, "Layer changed to %d", mesh_status.current_layer);
            if (event_callback) {
                event_callback(&mesh_status);
            }
            break;
            
        case MESH_EVENT_ROOT_GOT_IP:
            ESP_LOGI(TAG, "Root got IP address");
            mesh_status.is_root_elected = true;
            mesh_status.role = MESH_ROLE_ROOT;
            if (event_callback) {
                event_callback(&mesh_status);
            }
            break;
            
        case MESH_EVENT_ROOT_LOST_IP:
            ESP_LOGI(TAG, "Root lost IP");
            mesh_status.is_root_elected = false;
            mesh_status.role = MESH_ROLE_CHILD;
            break;
            
        case MESH_EVENT_NO_PARENT_FOUND:
            ESP_LOGW(TAG, "No parent found");
            break;
            
        case MESH_EVENT_VOTE_STARTED:
            ESP_LOGI(TAG, "Vote started for new root");
            break;
            
        case MESH_EVENT_VOTE_STOPPED:
            ESP_LOGI(TAG, "Vote stopped");
            break;
            
        case MESH_EVENT_ROOT_SWITCH_REQ:
            ESP_LOGI(TAG, "Root switch request received");
            break;
            
        case MESH_EVENT_ROOT_SWITCH_ACK:
            ESP_LOGI(TAG, "Root switch acknowledged");
            mesh_status.is_root_elected = true;
            mesh_status.role = MESH_ROLE_ROOT;
            break;
            
        default:
            break;
    }
}

static void mesh_recv_handler(mesh_addr_t *addr, mesh_data_t *data, int flags)
{
    if (data == NULL || data->data == NULL) {
        return;
    }
    
    ESP_LOGI(TAG, "Received %d bytes from " MACSTR, data->size, MAC2STR(addr->addr));
    
    if (data_callback) {
        data_callback(addr, data->data, data->size);
    }
}

esp_err_t mesh_manager_init(const char *ssid, const char *password, uint8_t max_layer)
{
    if (initialized) {
        ESP_LOGW(TAG, "Mesh manager already initialized");
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Initializing mesh manager");
    
    memset(&mesh_status, 0, sizeof(mesh_status));
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
    
    mesh_cfg_t mesh_config = {0};
    mesh_config.channel = 0;
    mesh_config.mesh_id.len = 6;
    uint8_t mesh_id[6] = MESH_ID;
    memcpy(mesh_config.mesh_id.addr, mesh_id, 6);
    mesh_config.mesh_type = MESH_IDLE; /* Auto-elect root via voting */
    mesh_config.router.ssid_len = strlen(mesh_status.mesh_ssid);
    strncpy((char *)mesh_config.router.ssid, mesh_status.mesh_ssid, sizeof(mesh_config.router.ssid));
    strncpy((char *)mesh_config.router.password, mesh_status.mesh_password, sizeof(mesh_config.router.password));
    mesh_config.router.bssid_set = false;
    mesh_config.mesh_ap.max_connection = 6;
    mesh_config.mesh_ap.nonmesh_max_connection = 4;
    strncpy((char *)mesh_config.mesh_ap.password, mesh_status.mesh_password, sizeof(mesh_config.mesh_ap.password));
    
    ESP_ERROR_CHECK(esp_mesh_set_config(&mesh_config));
    ESP_ERROR_CHECK(esp_mesh_set_vote_percentage(1));
    ESP_ERROR_CHECK(esp_mesh_set_xon_qsize(128));
    ESP_ERROR_CHECK(esp_mesh_set_aim_threshold(60));
    
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
    
    mesh_cfg_t config = {0};
    esp_mesh_get_config(&config);
    config.mesh_type = DEVICE_TYPE_ROOT;
    
    esp_err_t err = esp_mesh_set_config(&config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set root: %s", esp_err_to_name(err));
    }
    
    return err;
}

esp_err_t mesh_manager_set_parent(const char *parent_mac)
{
    if (!initialized || !parent_mac) return ESP_ERR_INVALID_ARG;
    
    mesh_addr_t parent_addr = {0};
    
    int mac[6];
    sscanf(parent_mac, "%x:%x:%x:%x:%x:%x", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
    
    for (int i = 0; i < 6; i++) {
        parent_addr.addr[i] = (uint8_t)mac[i];
    }
    
    ESP_LOGI(TAG, "Setting parent to: %s", parent_mac);
    
    return esp_mesh_set_parent(&parent_addr, NULL, MESH_ROOT, 0);
}

esp_err_t mesh_manager_set_topology(mesh_topo_t topo)
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
    
    return esp_mesh_switch_channel(0, 0);
}

esp_err_t mesh_manager_get_routing_table(mesh_addr_t *table, uint16_t *count)
{
    if (!running) return ESP_ERR_INVALID_STATE;
    
    if (!table || !count) return ESP_ERR_INVALID_ARG;
    
    return esp_mesh_get_routing_table(table, MESH_MAX_NODES * 6, count);
}
