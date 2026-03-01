#include "communication/communication.h"
#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_mac.h>
#include <esp_event.h>
#include <esp_netif.h>
#include <esp_mesh.h>
#include <esp_mesh_internal.h>
#include <nvs_flash.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include "sensors/sensor_manager.h"
#include "actuators/actuator_manager.h"
#include "utils/config.h"

#ifdef CONFIG_ESP_MQTT_ENABLED
#include <mqtt_client.h>
#endif

static const char *TAG = "COMMUNICATION";

#define MESH_TAG "POULTRY_MESH"
#define MESH_PREFIX "PoultryFarm"
#define MESH_PASSWORD "poultry2024"
#define MESH_MAX_LAYER 4
#define MESH_ID {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}

/* WiFi connection event bits */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
#define WIFI_MAX_RETRY     5

static comm_info_t comm_info = {0};
static volatile bool initialized = false;
static volatile bool connected = false;
static volatile bool mesh_initialized = false;
static char mqtt_broker[128] = "mqtt://localhost";
static uint16_t mqtt_port = 1883;
static char mqtt_topic[128] = "poultry/farm";

static EventGroupHandle_t wifi_event_group = NULL;
static int wifi_retry_count = 0;
static esp_netif_t *sta_netif = NULL;

#ifdef CONFIG_ESP_MQTT_ENABLED
static esp_mqtt_client_handle_t mqtt_client = NULL;
#endif

static mesh_addr_t mesh_mac = {0};
static uint8_t mesh_layer = 0;
static uint8_t mesh_node_count = 0;

/* WiFi event handler */
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        connected = false;
        comm_info.status = COMM_STATUS_DISCONNECTED;
        if (wifi_retry_count < WIFI_MAX_RETRY) {
            esp_wifi_connect();
            wifi_retry_count++;
            ESP_LOGI(TAG, "Retrying WiFi connection (attempt %d/%d)", wifi_retry_count, WIFI_MAX_RETRY);
        } else {
            if (wifi_event_group) xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);
            ESP_LOGW(TAG, "WiFi connection failed after %d retries", WIFI_MAX_RETRY);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        snprintf(comm_info.ip_address, sizeof(comm_info.ip_address),
                 IPSTR, IP2STR(&event->ip_info.ip));
        ESP_LOGI(TAG, "Got IP: %s", comm_info.ip_address);
        connected = true;
        comm_info.status = COMM_STATUS_CONNECTED;
        wifi_retry_count = 0;
        if (wifi_event_group) xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

/* Mesh event handler */
static void mesh_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    switch (event_id) {
        case MESH_EVENT_STARTED:
            ESP_LOGI(MESH_TAG, "Mesh network started");
            comm_info.status = COMM_STATUS_CONNECTED;
            comm_info.mesh_enabled = true;
            break;
            
        case MESH_EVENT_STOPPED:
            ESP_LOGI(MESH_TAG, "Mesh network stopped");
            comm_info.status = COMM_STATUS_DISCONNECTED;
            comm_info.mesh_enabled = false;
            break;
            
        case MESH_EVENT_CHILD_CONNECTED: {
            mesh_event_child_connected_t *child = (mesh_event_child_connected_t *)event_data;
            ESP_LOGI(MESH_TAG, "Child connected: " MACSTR, MAC2STR(child->mac));
            mesh_node_count++;
            comm_info.mesh_node_count = mesh_node_count;
            break;
        }
            
        case MESH_EVENT_CHILD_DISCONNECTED: {
            mesh_event_child_disconnected_t *child = (mesh_event_child_disconnected_t *)event_data;
            ESP_LOGI(MESH_TAG, "Child disconnected: " MACSTR, MAC2STR(child->mac));
            if (mesh_node_count > 0) mesh_node_count--;
            comm_info.mesh_node_count = mesh_node_count;
            break;
        }
            
        case MESH_EVENT_LAYER_CHANGE: {
            mesh_event_layer_change_t *layer = (mesh_event_layer_change_t *)event_data;
            mesh_layer = layer->new_layer;
            comm_info.mesh_layer = mesh_layer;
            ESP_LOGI(MESH_TAG, "Layer changed to %d", mesh_layer);
            break;
        }
            
        case IP_EVENT_STA_GOT_IP: {
            ip_event_got_ip_t *got_ip = (ip_event_got_ip_t *)event_data;
            snprintf(comm_info.ip_address, sizeof(comm_info.ip_address),
                     IPSTR, IP2STR(&got_ip->ip_info.ip));
            ESP_LOGI(MESH_TAG, "Root got IP: %s", comm_info.ip_address);
            comm_info.status = COMM_STATUS_CONNECTED;
            break;
        }
            
        case IP_EVENT_STA_LOST_IP:
            ESP_LOGI(MESH_TAG, "Root lost IP");
            break;
            
        case MESH_EVENT_NO_PARENT_FOUND:
            ESP_LOGW(MESH_TAG, "No parent found");
            break;
            
        default:
            break;
    }
}

#ifdef CONFIG_ESP_MQTT_ENABLED
static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                               int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT connected");
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "MQTT disconnected");
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT data received on topic: %.*s", event->topic_len, event->topic);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT error");
            break;
        default:
            break;
    }
}
#endif

esp_err_t communication_init(void)
{
    if (initialized) return ESP_OK;
    
    ESP_LOGI(TAG, "Initializing communication system");
    
    comm_info.mode = COMM_MODE_WIFI;
    comm_info.status = COMM_STATUS_DISCONNECTED;
    comm_info.bytes_sent = 0;
    comm_info.bytes_received = 0;
    comm_info.last_update = 0;
    comm_info.rssi = 0;
    comm_info.mesh_enabled = false;
    comm_info.mesh_layer = 0;
    comm_info.mesh_node_count = 0;
    comm_info.mesh_max_layer = MESH_MAX_LAYER;
    strncpy(comm_info.ip_address, "0.0.0.0", sizeof(comm_info.ip_address) - 1);
    
    /* Initialize TCP/IP stack and event loop */
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    sta_netif = esp_netif_create_default_wifi_sta();
    
    wifi_event_group = xEventGroupCreate();
    
    initialized = true;
    ESP_LOGI(TAG, "Communication system initialized");
    
    return ESP_OK;
}

esp_err_t communication_start(void)
{
    if (!initialized) return ESP_ERR_INVALID_STATE;
    
    if (strlen(system_config.wifi_ssid) > 0) {
        communication_connect_wifi(system_config.wifi_ssid, system_config.wifi_password);
    }
    
    return ESP_OK;
}

esp_err_t communication_stop(void)
{
    if (!initialized) return ESP_ERR_INVALID_STATE;
    
    if (mesh_initialized) {
        esp_mesh_stop();
        mesh_initialized = false;
    }
    
#ifdef CONFIG_ESP_MQTT_ENABLED
    if (mqtt_client) {
        esp_mqtt_client_stop(mqtt_client);
        esp_mqtt_client_destroy(mqtt_client);
        mqtt_client = NULL;
    }
#endif
    
    communication_disconnect();
    
    return ESP_OK;
}

esp_err_t communication_connect_wifi(const char *ssid, const char *password)
{
    if (!initialized || strlen(ssid) == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Connecting to WiFi: %s", ssid);
    comm_info.status = COMM_STATUS_CONNECTING;
    wifi_retry_count = 0;
    
    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    esp_err_t err = esp_wifi_init(&wifi_init_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "WiFi init failed: %s", esp_err_to_name(err));
        comm_info.status = COMM_STATUS_ERROR;
        return err;
    }
    
    /* Register event handlers */
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler, NULL, NULL));
    
    wifi_config_t wifi_config = {0};
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    
    err = esp_wifi_set_mode(WIFI_MODE_STA);
    if (err != ESP_OK) {
        comm_info.status = COMM_STATUS_ERROR;
        return err;
    }
    
    err = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (err != ESP_OK) {
        comm_info.status = COMM_STATUS_ERROR;
        return err;
    }
    
    err = esp_wifi_start();
    if (err != ESP_OK) {
        comm_info.status = COMM_STATUS_ERROR;
        return err;
    }
    
    /* Wait for connection result (non-blocking if no event group) */
    if (wifi_event_group) {
        EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE, pdFALSE, pdMS_TO_TICKS(15000));
        
        if (bits & WIFI_CONNECTED_BIT) {
            ESP_LOGI(TAG, "WiFi connected. IP: %s", comm_info.ip_address);
        } else {
            ESP_LOGW(TAG, "WiFi connection timed out or failed");
            comm_info.status = COMM_STATUS_ERROR;
            return ESP_ERR_TIMEOUT;
        }
    }
    
    return ESP_OK;
}

esp_err_t communication_disconnect(void)
{
    if (!connected) return ESP_OK;
    
    esp_wifi_disconnect();
    connected = false;
    comm_info.status = COMM_STATUS_DISCONNECTED;
    strncpy(comm_info.ip_address, "0.0.0.0", sizeof(comm_info.ip_address) - 1);
    
    ESP_LOGI(TAG, "WiFi disconnected");
    
    return ESP_OK;
}

esp_err_t communication_get_status(comm_info_t *info)
{
    memcpy(info, &comm_info, sizeof(comm_info_t));
    return ESP_OK;
}

esp_err_t communication_send_data(const char *topic, const char *data)
{
    if (!connected) {
        return ESP_ERR_INVALID_STATE;
    }
    
#ifdef CONFIG_ESP_MQTT_ENABLED
    if (mqtt_client) {
        int msg_id = esp_mqtt_client_publish(mqtt_client, topic, data, 0, 1, 0);
        if (msg_id < 0) {
            ESP_LOGE(TAG, "MQTT publish failed");
            return ESP_FAIL;
        }
        ESP_LOGI(TAG, "MQTT published to %s (msg_id=%d)", topic, msg_id);
        comm_info.bytes_sent += strlen(data);
        return ESP_OK;
    }
#endif
    
    ESP_LOGI(TAG, "Sending data to %s: %s", topic, data);
    comm_info.bytes_sent += strlen(data);
    
    return ESP_OK;
}

esp_err_t communication_subscribe(const char *topic)
{
    if (!connected) {
        return ESP_ERR_INVALID_STATE;
    }
    
#ifdef CONFIG_ESP_MQTT_ENABLED
    if (mqtt_client) {
        int msg_id = esp_mqtt_client_subscribe(mqtt_client, topic, 0);
        if (msg_id < 0) {
            ESP_LOGE(TAG, "MQTT subscribe failed");
            return ESP_FAIL;
        }
        ESP_LOGI(TAG, "MQTT subscribed to %s (msg_id=%d)", topic, msg_id);
        return ESP_OK;
    }
#endif
    
    ESP_LOGI(TAG, "Subscribing to topic: %s", topic);
    
    return ESP_OK;
}

esp_err_t communication_set_mqtt_config(const char *broker, uint16_t port, const char *topic)
{
    strncpy(mqtt_broker, broker, sizeof(mqtt_broker) - 1);
    mqtt_port = port;
    strncpy(mqtt_topic, topic, sizeof(mqtt_topic) - 1);
    
#ifdef CONFIG_ESP_MQTT_ENABLED
    /* Create and start MQTT client if WiFi is connected */
    if (connected && mqtt_client == NULL) {
        esp_mqtt_client_config_t mqtt_cfg = {
            .broker.address.uri = mqtt_broker,
        };
        mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
        if (mqtt_client) {
            esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
            esp_mqtt_client_start(mqtt_client);
            ESP_LOGI(TAG, "MQTT client started");
        }
    }
#endif
    
    ESP_LOGI(TAG, "MQTT config set - Broker: %s, Port: %d, Topic: %s", broker, port, topic);
    
    return ESP_OK;
}

esp_err_t communication_publish_sensor_data(void)
{
    if (!connected && !mesh_initialized) return ESP_ERR_INVALID_STATE;
    
    uint8_t sensor_count = 0;
    sensor_data_t *sensors = NULL;
    sensor_read_all(&sensors, &sensor_count);
    
    char json_buffer[1024];
    int offset = 0;
    offset += snprintf(json_buffer + offset, sizeof(json_buffer) - offset, "{");
    offset += snprintf(json_buffer + offset, sizeof(json_buffer) - offset, "\"timestamp\":%lu,",
                       (unsigned long)(xTaskGetTickCount() * portTICK_PERIOD_MS));
    offset += snprintf(json_buffer + offset, sizeof(json_buffer) - offset, "\"node_id\":\"");
    offset += snprintf(json_buffer + offset, sizeof(json_buffer) - offset, MACSTR, MAC2STR(mesh_mac.addr));
    offset += snprintf(json_buffer + offset, sizeof(json_buffer) - offset, "\",");
    offset += snprintf(json_buffer + offset, sizeof(json_buffer) - offset, "\"layer\":%d,", mesh_layer);
    offset += snprintf(json_buffer + offset, sizeof(json_buffer) - offset, "\"sensors\":[");
    
    for (int i = 0; i < sensor_count && i < 20; i++) {
        if ((size_t)offset >= sizeof(json_buffer) - 80) break; /* guard against overflow */
        if (i > 0) offset += snprintf(json_buffer + offset, sizeof(json_buffer) - offset, ",");
        offset += snprintf(json_buffer + offset, sizeof(json_buffer) - offset,
            "{\"name\":\"%s\",\"value\":%.2f}",
            sensors[i].name, sensors[i].value);
    }
    
    offset += snprintf(json_buffer + offset, sizeof(json_buffer) - offset, "]}");
    
    if (mesh_initialized) {
        communication_mesh_broadcast((uint8_t *)json_buffer, strlen(json_buffer));
    } else {
        communication_send_data(mqtt_topic, json_buffer);
    }
    
    comm_info.last_update = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    return ESP_OK;
}

esp_err_t communication_init_mesh(const char *mesh_ssid, const char *mesh_password, uint8_t max_layer)
{
    if (!initialized) return ESP_ERR_INVALID_STATE;
    
    ESP_LOGI(MESH_TAG, "Initializing ESP-MESH network");
    
    /* Initialize mesh */
    ESP_ERROR_CHECK(esp_mesh_init());
    
    /* Configure mesh */
    mesh_cfg_t mesh_config = {0};
    mesh_config.channel = 0;
    mesh_config.router.ssid_len = strlen(mesh_ssid ? mesh_ssid : MESH_PREFIX);
    strncpy((char *)mesh_config.router.ssid, mesh_ssid ? mesh_ssid : MESH_PREFIX,
            sizeof(mesh_config.router.ssid));
    strncpy((char *)mesh_config.router.password, mesh_password ? mesh_password : MESH_PASSWORD,
            sizeof(mesh_config.router.password));
    uint8_t mesh_id[6] = MESH_ID;
    memcpy(mesh_config.mesh_id.addr, mesh_id, 6);
    mesh_config.mesh_ap.max_connection = 6;
    mesh_config.mesh_ap.nonmesh_max_connection = 4;
    strncpy((char *)mesh_config.mesh_ap.password, mesh_password ? mesh_password : MESH_PASSWORD,
            sizeof(mesh_config.mesh_ap.password));
    
    ESP_ERROR_CHECK(esp_mesh_set_config(&mesh_config));
    ESP_ERROR_CHECK(esp_mesh_set_max_layer(max_layer > 0 ? max_layer : MESH_MAX_LAYER));
    ESP_ERROR_CHECK(esp_mesh_set_vote_percentage(1));
    ESP_ERROR_CHECK(esp_mesh_set_xon_qsize(128));
    
    ESP_ERROR_CHECK(esp_event_handler_register(MESH_EVENT, ESP_EVENT_ANY_ID, &mesh_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &mesh_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_LOST_IP, &mesh_event_handler, NULL));
    
    ESP_ERROR_CHECK(esp_mesh_start());
    
    mesh_initialized = true;
    comm_info.mesh_enabled = true;
    comm_info.mode = COMM_MODE_MESH;
    
    ESP_LOGI(MESH_TAG, "Mesh network started successfully");
    
    return ESP_OK;
}

esp_err_t communication_mesh_send(uint8_t *data, uint16_t len)
{
    if (!mesh_initialized) return ESP_ERR_INVALID_STATE;
    if (data == NULL || len == 0) return ESP_ERR_INVALID_ARG;
    
    mesh_data_t mesh_data = {
        .data = data,
        .size = len,
        .proto = MESH_PROTO_BIN,
        .tos = MESH_TOS_P2P
    };
    
    mesh_addr_t target_addr = {0};
    
    esp_err_t err = esp_mesh_send(&target_addr, &mesh_data, 0, NULL, 0);
    
    if (err == ESP_OK) {
        comm_info.bytes_sent += len;
    } else {
        ESP_LOGE(MESH_TAG, "Mesh send failed: %s", esp_err_to_name(err));
    }
    
    return err;
}

esp_err_t communication_mesh_broadcast(uint8_t *data, uint16_t len)
{
    if (!mesh_initialized) return ESP_ERR_INVALID_STATE;
    if (data == NULL || len == 0) return ESP_ERR_INVALID_ARG;
    
    mesh_data_t mesh_data = {
        .data = data,
        .size = len,
        .proto = MESH_PROTO_BIN,
        .tos = MESH_TOS_P2P
    };
    
    esp_err_t err = esp_mesh_send(NULL, &mesh_data, MESH_DATA_P2P, NULL, 0);
    
    if (err == ESP_OK) {
        comm_info.bytes_sent += len;
    } else {
        ESP_LOGE(MESH_TAG, "Mesh broadcast failed: %s", esp_err_to_name(err));
    }
    
    return err;
}

esp_err_t communication_mesh_get_nodes(uint8_t *node_count)
{
    if (!mesh_initialized) return ESP_ERR_INVALID_STATE;
    
    *node_count = mesh_node_count;
    return ESP_OK;
}

esp_err_t communication_set_mesh_parent(const char *parent_mac)
{
    if (!mesh_initialized) return ESP_ERR_INVALID_STATE;
    if (parent_mac == NULL) return ESP_ERR_INVALID_ARG;
    
    mesh_addr_t parent_addr = {0};
    
    unsigned int mac[6];
    sscanf(parent_mac, "%02x:%02x:%02x:%02x:%02x:%02x",
           &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
    
    for (int i = 0; i < 6; i++) {
        parent_addr.addr[i] = (uint8_t)mac[i];
    }
    
    wifi_config_t parent_cfg = {0};
    memcpy(parent_cfg.sta.bssid, parent_addr.addr, 6);
    parent_cfg.sta.bssid_set = true;
    
    esp_err_t err = esp_mesh_set_parent(&parent_cfg, NULL, MESH_ROOT, 0);
    
    if (err == ESP_OK) {
        ESP_LOGI(MESH_TAG, "Parent set to: %s", parent_mac);
    } else {
        ESP_LOGE(MESH_TAG, "Failed to set parent: %s", esp_err_to_name(err));
    }
    
    return err;
}
