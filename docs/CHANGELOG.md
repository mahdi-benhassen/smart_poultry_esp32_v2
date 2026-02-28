# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.1.0] - 2026-02-28

### Added
- **ESP-MESH Networking**
  - Self-organizing mesh network
  - Support for 1000+ nodes
  - Self-healing topology
  - Root node gateway functionality
  - Broadcast and unicast communication
  - Mesh data protocol
  - Automatic parent selection
  - Network healing
  - Node discovery

### Components Updated
- **Communication Module**
  - Added COMM_MODE_MESH
  - Mesh initialization and management
  - Mesh event handling
  - Data broadcast/send functions
  
- **Configuration**
  - Mesh SSID/password configuration
  - Max layer settings
  - Root node configuration

### Documentation
- Added MESH_NETWORK.md
- Network topology diagrams
- API reference for mesh functions
- Troubleshooting guide

## [1.0.0] - 2026-02-28

### Added
- **Initial Release**
- Complete ESP32-based smart poultry farming system

### Components
- **Sensors Module**
  - DHT22 temperature/humidity sensor support
  - BME280 environmental sensor (temperature, humidity, pressure)
  - MQ series gas sensors (MQ-135, MQ-7, MQ-2)
  - Weight sensors with HX711
  - Water level sensors
  - Sensor manager with alarm detection
  - Maximum 32 sensors supported

- **Actuators Module**
  - Fan control (4 units)
  - Heater control (2 units)
  - Light control (4 units)
  - Feeder control (2 units)
  - Water pump control (2 units)
  - PWM speed control support
  - Emergency stop functionality

- **Control System**
  - Automatic temperature control
  - Automatic humidity control
  - Gas/Ammonia ventilation control
  - Light scheduling
  - Water level management
  - Multiple control modes (Auto, Manual, Scheduled, Adaptive)
  - Hysteresis-based control algorithms

- **Monitoring System**
  - Real-time environmental monitoring
  - Alarm detection and logging
  - System status tracking
  - Event logging with severity levels
  - Data export functionality

- **Communication**
  - WiFi connectivity
  - MQTT protocol support
  - JSON data format
  - Topic-based publishing
  - Remote control commands

- **Configuration**
  - NVS-based persistent storage
  - Configurable thresholds
  - System parameters customization

### Documentation
- README.md - Project overview and installation
- HARDWARE_SETUP.md - Hardware wiring guide
- API_REFERENCE.md - Complete API documentation
- CONTROL_ALGORITHMS.md - Control logic explanation
- CHANGELOG.md - Version history

### Architecture
- Modular component-based design
- Extensible sensor/actuator system
- Task-based concurrent operations
- ESP-IDF framework

## [Unreleased]

### Planned Features
- [ ] Multi-zone support for large farms
- [ ] Cloud dashboard integration
- [ ] Mobile app support
- [ ] SMS notification system
- [ ] Data analytics
- [ ] Predictive maintenance
- [ ] OTA firmware updates
- [ ] LoRa WAN communication option
- [ ] Ethernet connectivity
- [ ] Additional sensor types

### Known Issues
- LSP errors shown in editor are expected - code compiles with ESP-IDF
- Gas sensor calibration requires manual process
- Weight sensor may need calibration per sensor

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0.0 | 2026-02-28 | Initial release |

## Contributing

Contributions are welcome! Please read the contributing guidelines before submitting pull requests.

## License

MIT License - See LICENSE file for details
