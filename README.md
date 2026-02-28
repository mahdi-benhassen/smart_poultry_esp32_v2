# Smart Poultry System

A comprehensive IoT-based smart poultry farming system built on ESP32 with modular architecture for environmental monitoring, automated control, and remote communication.

## Features

- **Multi-Sensor Integration**: Temperature, humidity, gas (ammonia, CO2, CO), pressure, weight, and water level sensors
- **Automated Control**: Intelligent control algorithms for ventilation, heating, lighting, feeding, and water management
- **Real-time Monitoring**: Continuous environmental monitoring with alarm detection
- **Remote Communication**: WiFi connectivity with MQTT protocol for cloud integration
- **ESP-MESH Networking**: Distributed mesh network for large-scale farms with self-healing and auto-organization
- **Extensible Design**: Modular architecture allowing easy addition of new sensors and actuators

## Network Modes

### Standalone Mode
Single ESP32 connected directly to WiFi and MQTT broker.

### Mesh Mode
Multiple ESP32 nodes forming a self-organizing mesh network:
- Up to 1000+ nodes
- Self-healing topology
- Distributed data collection
- Root node acts as gateway

See [MESH_NETWORK.md](docs/MESH_NETWORK.md) for detailed mesh configuration.

## Hardware Requirements

### ESP32 Development Board
- ESP32-WROOM-32 or similar
- 4MB Flash minimum
- WiFi enabled

### Sensors
| Sensor | Model | Quantity | Purpose |
|--------|-------|----------|---------|
| Temperature/Humidity | DHT22 | 2 | Primary ambient monitoring |
| Temperature/Humidity/Pressure | BME280 | 1 | Secondary ambient monitoring |
| Ammonia | MQ-135 | 1 | Air quality monitoring |
| CO2 | MQ-135/MQ-7 | 1 | Carbon dioxide detection |
| CO | MQ-7 | 1 | Carbon monoxide detection |
| Weight | HX711 + Load Cell | 2 | Feed/bird weight monitoring |
| Water Level | Ultrasonic/ Float | 2 | Water tank monitoring |

### Actuators
| Actuator | Type | Quantity | Control |
|----------|------|----------|---------|
| Fan | 12V DC | 4 | PWM speed control |
| Heater | 220V AC Relay | 2 | On/Off control |
| Light | LED/Incandescent | 4 | On/Off + dimming |
| Feeder | Servo Motor | 2 | Scheduled feeding |
| Water Pump | 12V DC | 2 | On/Off control |

## Pin Configuration

```
GPIO Assignments:
- DHT22: GPIO 4
- BME280 I2C: SDA=21, SCL=22
- MQ-2 (Ammonia): GPIO 3 (ADC1_CH3)
- MQ-135 (CO2): GPIO 4 (ADC1_CH4)
- MQ-7 (CO): GPIO 5 (ADC1_CH5)
- Weight Sensor: GPIO 6 (ADC1_CH6)
- Water Level 1: GPIO 32
- Water Level 2: GPIO 33
- Water Level 3: GPIO 34
- Fan 1: GPIO 2
- Fan 2: GPIO 4
- Fan 3: GPIO 5
- Fan 4: GPIO 18
- Heater 1: GPIO 19
- Heater 2: GPIO 21
- Light 1: GPIO 22
- Light 2: GPIO 23
- Feeder 1: GPIO 25
- Feeder 2: GPIO 26
- Water Pump 1: GPIO 27
- Water Pump 2: GPIO 14
```

## Installation

### Prerequisites

1. **ESP-IDF**: Install ESP-IDF v4.4 or later
   ```bash
   # Windows
   C:\Espressif\esp-idf\install.bat
   C:\Espressif\esp-idf\export.bat
   
   # Linux/Mac
   ./install.sh
   . ./export.sh
   ```

2. **Python**: Python 3.8+ required

### Build Instructions

```bash
# Navigate to project directory
cd smart_poultry_system

# Configure project
idf.py menuconfig

# Build project
idf.py build

# Flash to device
idf.py -p COM3 flash monitor
```

### Configuration

After running `idf.py menuconfig`, configure:
- `Poultry System Config`: Sensor thresholds, control parameters
- `ESP32 Specific`: CPU frequency, flash size
- `WiFi Configuration`: SSID and password

## Project Structure

```
smart_poultry_system/
├── components/
│   ├── sensors/           # Sensor drivers and management
│   │   ├── dht22.c/h     # DHT22 temperature/humidity
│   │   ├── mq_sensor.c/h # MQ series gas sensors
│   │   ├── bme280_sensor.c/h
│   │   ├── weight_sensor.c/h
│   │   └── water_level_sensor.c/h
│   ├── actuators/         # Actuator control
│   │   └── actuator_manager.c/h
│   ├── control/          # Control algorithms
│   │   └── control_system.c/h
│   ├── monitoring/       # System monitoring
│   │   └── monitoring.c/h
│   ├── communication/    # WiFi/MQTT
│   │   └── communication.c/h
│   └── utils/           # Utilities
│       └── config.c/h
├── main/
│   └── main.c          # Application entry point
├── CMakeLists.txt
├── sdkconfig.defaults
└── README.md
```

## Usage

### Initial Setup

1. Flash the firmware to ESP32
2. Connect all sensors and actuators according to pin configuration
3. Configure WiFi credentials via serial or menuconfig
4. System will automatically connect and start monitoring

### Control Modes

- **Auto Mode**: System automatically adjusts based on sensor readings
- **Manual Mode**: Manual control via MQTT commands
- **Scheduled Mode**: Time-based control schedules

### MQTT Topics

```
poultry/farm/sensors     # Sensor data publishing
poultry/farm/actuators   # Actuator control commands
poultry/farm/status      # System status
poultry/farm/alarms      # Alarm notifications
```

### Example MQTT Messages

**Sensor Data:**
```json
{
  "timestamp": 1699999999,
  "sensors": [
    {"name": "Temperature_1", "value": 24.5},
    {"name": "Humidity_1", "value": 65.0},
    {"name": "Ammonia_Sensor", "value": 5.2},
    {"name": "CO2_Sensor", "value": 450.0}
  ]
}
```

**Control Command:**
```json
{
  "actuator": "Fan_1",
  "command": "ON"
}
```

## Control Algorithms

### Temperature Control
- Below minimum: Activate heaters
- Above maximum: Activate fans
- Optimal range: No action needed

### Humidity Control
- Above maximum: Activate ventilation
- Below minimum: Optional humidifier

### Gas Control (Ammonia/CO2)
- Above threshold: Maximum ventilation
- Emergency: All fans on

### Light Control
- Daytime (6 AM - 6 PM): Maintain optimal light levels
- Nighttime: Lights off

### Water Management
- Low level: Activate pumps
- High level: Deactivate pumps

## Troubleshooting

### Common Issues

1. **WiFi Connection Failed**
   - Check SSID/password correctness
   - Ensure 2.4GHz network (not 5GHz)
   - Check signal strength

2. **Sensor Reading Errors**
   - Verify wiring connections
   - Check sensor power supply
   - Calibrate gas sensors if needed

3. **Actuator Not Responding**
   - Check GPIO pin assignments
   - Verify relay/power supply
   - Check manual override status

### Debug Logging

Enable debug output:
```bash
idf.py menuconfig
# ESP32 Specific -> Log Level -> Debug
```

## Maintenance

### Regular Tasks
- Clean sensors monthly
- Check actuator functionality weekly
- Verify calibration monthly
- Backup configuration

### Calibration
- Temperature sensors: Use calibrated thermometer
- Gas sensors: Calibrate in fresh air (R0 calibration)
- Weight sensors: Tare before use

## License

MIT License

## Author

Smart Poultry System v1.0.0
