# Smart Poultry System - API Reference

## Configuration API

### Functions

#### `config_init()`
Initialize the configuration system.

```c
void config_init(void);
```

#### `config_load()`
Load configuration from NVS storage.

```c
void config_load(void);
```

#### `config_save()`
Save current configuration to NVS storage.

```c
void config_save(void);
```

#### `config_reset()`
Reset configuration to default values.

```c
void config_reset(void);
```

#### `config_update()`
Update configuration with new values.

```c
void config_update(poultry_config_t *config);
```

#### `config_set_temperature_range()`
Set temperature thresholds.

```c
esp_err_t config_set_temperature_range(float min, float max, float optimal);
```

**Parameters:**
- `min`: Minimum temperature (°C)
- `max`: Maximum temperature (°C)
- `optimal`: Optimal temperature (°C)

#### `config_set_humidity_range()`
Set humidity thresholds.

```c
esp_err_t config_set_humidity_range(float min, float max, float optimal);
```

#### `config_set_gas_limits()`
Set gas concentration limits.

```c
esp_err_t config_set_gas_limits(float ammonia, float co2, float co);
```

**Parameters:**
- `ammonia`: Maximum ammonia (ppm)
- `co2`: Maximum CO2 (ppm)
- `co`: Maximum CO (ppm)

---

## Sensor Manager API

### Types

```c
typedef enum {
    SENSOR_TYPE_TEMPERATURE,
    SENSOR_TYPE_HUMIDITY,
    SENSOR_TYPE_AMMONIA,
    SENSOR_TYPE_CO2,
    SENSOR_TYPE_CO,
    SENSOR_TYPE_LIGHT,
    SENSOR_TYPE_SOUND,
    SENSOR_TYPE_WATER_LEVEL,
    SENSOR_TYPE_WEIGHT,
    SENSOR_TYPE_MOTION,
    SENSOR_TYPE_DOOR
} sensor_type_t;

typedef enum {
    SENSOR_STATUS_OK,
    SENSOR_STATUS_ERROR,
    SENSOR_STATUS_CALIBRATING,
    SENSOR_STATUS_OFFLINE
} sensor_status_t;

typedef struct {
    uint8_t id;
    char name[64];
    sensor_type_t type;
    sensor_status_t status;
    float value;
    float min_value;
    float max_value;
    float threshold_min;
    float threshold_max;
    uint32_t last_read_time;
    bool enabled;
    bool alarm_enabled;
} sensor_data_t;
```

### Functions

#### `sensor_manager_init()`
Initialize the sensor manager and all sensors.

```c
esp_err_t sensor_manager_init(void);
```

#### `sensor_register()`
Register a new sensor.

```c
esp_err_t sensor_register(uint8_t id, const char *name, sensor_type_t type, 
                          float min_val, float max_val, float threshold_min, float threshold_max);
```

#### `sensor_read()`
Read a specific sensor value.

```c
esp_err_t sensor_read(uint8_t id, float *value);
```

#### `sensor_read_all()`
Read all sensor values.

```c
esp_err_t sensor_read_all(sensor_data_t **sensors, uint8_t *count);
```

#### `sensor_trigger_read()`
Trigger a single sensor read.

```c
esp_err_t sensor_trigger_read(uint8_t id);
```

#### `sensor_trigger_read_all()`
Trigger reads for all sensors.

```c
esp_err_t sensor_trigger_read_all(void);
```

#### `sensor_check_alarm()`
Check if a sensor has triggered an alarm.

```c
bool sensor_check_alarm(uint8_t id);
```

#### `sensor_check_all_alarms()`
Check all sensors for alarm conditions.

```c
bool sensor_check_all_alarms(void);
```

---

## Actuator Manager API

### Types

```c
typedef enum {
    ACTUATOR_TYPE_FAN,
    ACTUATOR_TYPE_HEATER,
    ACTUATOR_TYPE_LIGHT,
    ACTUATOR_TYPE_FEEDER,
    ACTUATOR_TYPE_PUMP,
    ACTUATOR_TYPE_SERVO,
    ACTUATOR_TYPE_VALVE
} actuator_type_t;

typedef enum {
    ACTUATOR_STATE_OFF,
    ACTUATOR_STATE_ON,
    ACTUATOR_STATE_AUTO,
    ACTUATOR_STATE_ERROR
} actuator_state_t;

typedef struct {
    uint8_t id;
    char name[64];
    actuator_type_t type;
    actuator_state_t state;
    uint8_t pin;
    uint8_t duty_cycle;
    bool enabled;
    bool manual_override;
    uint32_t last_activation_time;
    uint32_t total_runtime;
    uint32_t activation_count;
} actuator_data_t;
```

### Functions

#### `actuator_manager_init()`
Initialize the actuator manager.

```c
esp_err_t actuator_manager_init(void);
```

#### `actuator_register()`
Register a new actuator.

```c
esp_err_t actuator_register(uint8_t id, const char *name, actuator_type_t type, uint8_t pin);
```

#### `actuator_set_state()`
Set actuator state.

```c
esp_err_t actuator_set_state(uint8_t id, actuator_state_t state);
```

#### `actuator_set_duty_cycle()`
Set PWM duty cycle for variable speed actuators.

```c
esp_err_t actuator_set_duty_cycle(uint8_t id, uint8_t duty_cycle);
```

#### `actuator_get_all()`
Get all actuator data.

```c
esp_err_t actuator_get_all(actuator_data_t **actuators, uint8_t *count);
```

#### `actuator_emergency_stop_all()`
Emergency stop all actuators.

```c
esp_err_t actuator_emergency_stop_all(void);
```

---

## Control System API

### Types

```c
typedef enum {
    CONTROL_MODE_MANUAL,
    CONTROL_MODE_AUTO,
    CONTROL_MODE_SCHEDULED,
    CONTROL_MODE_ADAPTIVE
} control_mode_t;

typedef struct {
    control_mode_t mode;
    bool auto_fan_enabled;
    bool auto_heater_enabled;
    bool auto_light_enabled;
    bool auto_feeder_enabled;
    bool auto_pump_enabled;
    bool emergency_stop;
    uint32_t last_control_time;
    uint32_t control_interval_ms;
} control_state_t;
```

### Functions

#### `control_system_init()`
Initialize the control system.

```c
esp_err_t control_system_init(void);
```

#### `control_system_start()`
Start the control loop task.

```c
esp_err_t control_system_start(void);
```

#### `control_system_stop()`
Stop the control loop task.

```c
esp_err_t control_system_stop(void);
```

#### `control_system_set_mode()`
Set control mode.

```c
esp_err_t control_system_set_mode(control_mode_t mode);
```

#### `control_system_update()`
Manually trigger a control update.

```c
esp_err_t control_system_update(void);
```

#### `control_system_emergency_stop()`
Trigger emergency stop.

```c
esp_err_t control_system_emergency_stop(void);
```

#### `control_system_reset_emergency()`
Reset emergency stop state.

```c
esp_err_t control_system_reset_emergency(void);
```

---

## Monitoring API

### Types

```c
typedef struct {
    uint32_t timestamp;
    float temperature_avg;
    float humidity_avg;
    float ammonia_max;
    float co2_max;
    uint16_t alarm_count;
    uint16_t actuator_activations;
    uint8_t system_status;
} system_status_t;

typedef struct {
    uint32_t timestamp;
    char sensor_name[64];
    float value;
    float threshold_min;
    float threshold_max;
    bool alarm_triggered;
} alarm_event_t;

typedef struct {
    char message[256];
    uint32_t timestamp;
    uint8_t severity;
} log_event_t;
```

### Functions

#### `monitoring_init()`
Initialize the monitoring system.

```c
esp_err_t monitoring_init(void);
```

#### `monitoring_start()`
Start the monitoring task.

```c
esp_err_t monitoring_start(void);
```

#### `monitoring_stop()`
Stop the monitoring task.

```c
esp_err_t monitoring_stop(void);
```

#### `monitoring_update()`
Manually trigger monitoring update.

```c
esp_err_t monitoring_update(void);
```

#### `monitoring_get_status()`
Get current system status.

```c
esp_err_t monitoring_get_status(system_status_t *status);
```

#### `monitoring_log_event()`
Log a system event.

```c
esp_err_t monitoring_log_event(const char *message, uint8_t severity);
```

#### `monitoring_check_alarms()`
Check and process alarm conditions.

```c
esp_err_t monitoring_check_alarms(void);
```

---

## Communication API

### Types

```c
typedef enum {
    COMM_MODE_WIFI,
    COMM_MODE_ETHERNET,
    COMM_MODE_LORA
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
} comm_info_t;
```

### Functions

#### `communication_init()`
Initialize the communication system.

```c
esp_err_t communication_init(void);
```

#### `communication_start()`
Start communication services.

```c
esp_err_t communication_start(void);
```

#### `communication_connect_wifi()`
Connect to WiFi network.

```c
esp_err_t communication_connect_wifi(const char *ssid, const char *password);
```

#### `communication_send_data()`
Send data to MQTT topic.

```c
esp_err_t communication_send_data(const char *topic, const char *data);
```

#### `communication_set_mqtt_config()`
Configure MQTT broker settings.

```c
esp_err_t communication_set_mqtt_config(const char *broker, uint16_t port, const char *topic);
```

#### `communication_publish_sensor_data()`
Publish all sensor data to MQTT.

```c
esp_err_t communication_publish_sensor_data(void);
```

---

## Example Usage

### Reading Sensors

```c
// Get all sensor data
sensor_data_t *sensors;
uint8_t count;
sensor_read_all(&sensors, &count);

for (int i = 0; i < count; i++) {
    printf("Sensor: %s, Value: %.2f, Status: %d\n", 
           sensors[i].name, 
           sensors[i].value, 
           sensors[i].status);
}
```

### Controlling Actuators

```c
// Turn on a fan
actuator_set_state(0, ACTUATOR_STATE_ON);

// Set fan speed (0-100%)
actuator_set_duty_cycle(0, 75);

// Enable manual override
actuator_set_manual_override(0, true);
```

### Configuration

```c
// Set temperature range
config_set_temperature_range(18.0f, 30.0f, 24.0f);

// Set humidity range
config_set_humidity_range(40.0f, 80.0f, 60.0f);

// Set gas limits
config_set_gas_limits(25.0f, 3000.0f, 50.0f);
```

### MQTT Integration

```c
// Configure MQTT
communication_set_mqtt_config("mqtt.example.com", 1883, "poultry/farm");

// Publish sensor data
communication_publish_sensor_data();

// Send custom message
communication_send_data("poultry/farm/status", "System running normally");
```
