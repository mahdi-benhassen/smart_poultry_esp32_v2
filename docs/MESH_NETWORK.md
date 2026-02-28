# ESP-MESH Network Guide

## Overview

ESP-MESH is a built-in mesh networking protocol for ESP32 that allows multiple devices to connect and communicate in a mesh topology. The Smart Poultry System supports ESP-MESH for distributed sensor/actuator networks across large poultry farms.

## Features

- **Self-Organizing**: Nodes automatically discover and connect to each other
- **Self-Healing**: Network recovers automatically if a node fails
- **Scalable**: Supports up to 1000+ nodes
- **No Central Hub Required**: Any node can serve as root
- **Low Power**: Optimized for battery-powered nodes

## Network Topology

```
                    Internet
                       |
                   [Root Node]
                   (Gateway)
                   /   |   \
                  /    |    \
           [Node 1] [Node 2] [Node 3]
              /        |         \
         [Node 4] [Node 5]   [Node 6]
           ...       ...        ...
```

### Node Types

1. **Root Node**: Connects to external network (WiFi/MQTT)
2. **Intermediate Node**: Acts as parent/child in mesh
3. **Leaf Node**: End device with no children

## Configuration

### Default Settings

| Parameter | Default Value | Description |
|-----------|---------------|-------------|
| Mesh SSID | PoultryFarm | Network name |
| Mesh Password | poultry2024 | Network password |
| Max Layers | 4 | Maximum mesh depth |
| Channel | 0 (Auto) | WiFi channel |
| Max Connections | 6 | Children per node |

### Configuration via Code

```c
// Initialize mesh with custom settings
mesh_manager_init("MyPoultryFarm", "password123", 4);

// Force this node as root
mesh_manager_force_root();

// Set specific parent node
mesh_manager_set_parent("AA:BB:CC:DD:EE:FF");
```

### Configuration via NVS

```c
// Enable mesh
config_set_mesh_enabled(true);

// Set mesh credentials
config_set_mesh_config("PoultryFarm", "password", 4);

// Set as root node
config_set_mesh_as_root(true);
```

## Mesh Data Protocol

### Packet Format

```c
typedef struct {
    uint8_t type;      // Data type (sensor, actuator, status, etc.)
    uint8_t subtype;   // Subtype for specific data
    uint16_t length;   // Data length
    uint8_t data[];    // Payload
} mesh_protocol_header_t;
```

### Data Types

| Type | Value | Description |
|------|-------|-------------|
| SENSOR | 0x01 | Sensor reading data |
| ACTUATOR | 0x02 | Actuator command |
| STATUS | 0x03 | Node status update |
| COMMAND | 0x04 | Control command |
| SYNC | 0x05 | Time synchronization |

## Communication

### Broadcasting

```c
// Send to all nodes
uint8_t data[] = "Sensor data...";
mesh_manager_broadcast(data, sizeof(data));
```

### Unicast

```c
// Send to specific node
mesh_addr_t target = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
mesh_manager_send_to_node(&target, data, len);
```

### Parent Communication

```c
// Send to parent node (for root transmission)
mesh_manager_send_to_parent(data, len);
```

## Data Flow

### Sensor Data Collection

```
[Leaf Node 1] ---> [Parent Node] ---> ... ---> [Root Node] ---> MQTT Broker
[Leaf Node 2] ---> [Parent Node] ---> ... ---> [Root Node] ---> MQTT Broker
[Leaf Node 3] ---> [Parent Node] ---> ... ---> [Root Node] ---> MQTT Broker
```

### Control Commands

```
MQTT Broker ---> [Root Node] ---> [Child Node] ---> [Actuator]
```

## Optimal Network Design

### Layer Planning

- **Layer 1 (Root)**: 1 node - placed near internet gateway
- **Layer 2**: Up to 6 nodes - main coverage area
- **Layer 3**: Up to 36 nodes - extended coverage
- **Layer 4**: Up to 216 nodes - maximum range

### Placement Guidelines

1. Place root node centrally
2. Ensure line-of-sight between parent-child nodes
3. Maintain 10-20m spacing between nodes
4. Avoid interference from metal structures

## API Reference

### Initialization

```c
esp_err_t mesh_manager_init(const char *ssid, const char *password, uint8_t max_layer);
esp_err_t mesh_manager_start(void);
esp_err_t mesh_manager_stop(void);
```

### Data Transfer

```c
esp_err_t mesh_manager_broadcast(uint8_t *data, uint16_t len);
esp_err_t mesh_manager_send_to_node(mesh_addr_t *target, uint8_t *data, uint16_t len);
esp_err_t mesh_manager_send_to_parent(uint8_t *data, uint16_t len);
```

### Network Management

```c
esp_err_t mesh_manager_get_status(mesh_status_t *status);
esp_err_t mesh_manager_get_nodes(mesh_node_info_t *nodes, uint8_t *count);
esp_err_t mesh_manager_force_root(void);
esp_err_t mesh_manager_set_parent(const char *parent_mac);
esp_err_t mesh_manager_heal_network(void);
```

## Troubleshooting

### Common Issues

1. **Nodes Not Connecting**
   - Check power supply
   - Verify password
   - Check distance between nodes
   - Ensure compatible firmware

2. **Root Not Found**
   - Check WiFi configuration
   - Increase max layer
   - Reduce node distance

3. **Network Unstable**
   - Check for interference
   - Add more parent nodes
   - Reduce number of children per node

### Monitoring

```c
// Get network status
mesh_status_t status;
mesh_manager_get_status(&status);

printf("Layer: %d\n", status.current_layer);
printf("Connected Nodes: %d\n", status.connected_nodes);
printf("Is Root: %s\n", status.is_root_elected ? "Yes" : "No");
```

## Power Consumption

| Mode | Current | Description |
|------|---------|-------------|
| Active | 80-100mA | Normal operation |
| RX | 50-60mA | Receiving data |
| TX | 100-150mA | Transmitting data |
| Sleep | 10-20µA | Deep sleep |

## Security

- WPA2-PSK encryption
- MAC address filtering (optional)
- Application-level data encryption

## Advanced Features

### Predictive Routing
```c
// Future enhancement
typedef struct {
    mesh_addr_t next_hop;
    uint8_t hop_count;
    float signal_strength;
    uint32_t latency;
} mesh_route_t;
```

### Group Communication
```c
// Send to specific group
esp_err_t mesh_send_to_group(uint8_t group_id, uint8_t *data, uint16_t len);
```

## Best Practices

1. **Network Planning**
   - Plan layer distribution before deployment
   - Place root node optimally
   - Plan for 20% expansion

2. **Maintenance**
   - Regular network health checks
   - Update firmware OTA
   - Monitor node battery levels

3. **Troubleshooting**
   - Use serial logs for debugging
   - Check RSSI values
   - Monitor routing table

## Comparison: Mesh vs Star Topology

| Feature | Mesh | Star (WiFi) |
|---------|------|-------------|
| Range | Unlimited | Limited by AP |
| Reliability | High | Single point failure |
| Scalability | Excellent | Limited |
| Complexity | High | Low |
| Cost | Medium | Low |

## Future Enhancements

- [ ] ESP-MESH with BLE combo
- [ ] Time-synchronized sampling
- [ ] Distributed data aggregation
- [ ] Multi-gateway support
