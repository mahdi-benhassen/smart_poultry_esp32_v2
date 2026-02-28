# Smart Poultry System - Control Algorithms

## Overview

The Smart Poultry System implements intelligent control algorithms to maintain optimal environmental conditions for poultry farming. The control system uses a modular approach with specialized logic for each environmental parameter.

## Control Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    Control System                            │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐    │
│  │  Temperature │  │   Humidity   │  │    Gas       │    │
│  │   Control    │  │   Control    │  │   Control    │    │
│  └──────────────┘  └──────────────┘  └──────────────┘    │
│                                                             │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐    │
│  │    Light     │  │   Feeder     │  │    Water     │    │
│  │   Control    │  │   Control    │  │   Control    │    │
│  └──────────────┘  └──────────────┘  └──────────────┘    │
│                                                             │
└─────────────────────────────────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────┐
│                   Actuator Manager                           │
├─────────────────────────────────────────────────────────────┤
│  Fans  │  Heaters  │  Lights  │  Feeders  │  Pumps       │
└─────────────────────────────────────────────────────────────┘
```

## Temperature Control

### Algorithm: Hysteresis Control

```
        Temperature
             │
    30 ──────┼───────────────────────────────────►
             │        ┌──────────┐
             │        │          │
             │        │  HEATERS │  ON
    24 ──────┤────────┤   OFF    ├────────────────►
             │        │          │
             │        │          │
    18 ──────┼────────┴──────────┴─────────────────
             │
             └──────────────────────────────────────────► Time
```

### Implementation

```c
void control_temperature_logic(float temperature, float humidity)
{
    if (temperature > poultry_config.temp_max) {
        // Too hot - activate cooling (fans)
        for (int i = 0; i < 4; i++) {
            actuator_set_state(i, ACTUATOR_STATE_ON);
        }
    } else if (temperature < poultry_config.temp_min) {
        // Too cold - activate heating
        for (int i = 4; i < 6; i++) {
            actuator_set_state(i, ACTUATOR_STATE_ON);
        }
    } else {
        // Optimal range - no action needed
        // All temperature actuators off
    }
}
```

### Parameters
| Parameter | Default | Range | Description |
|-----------|---------|-------|-------------|
| temp_min | 18°C | 10-25°C | Minimum acceptable temperature |
| temp_max | 30°C | 25-40°C | Maximum acceptable temperature |
| temp_optimal | 24°C | 18-30°C | Target temperature |

## Humidity Control

### Algorithm: Dual-Threshold Control

```c
void control_humidity_logic(float humidity, float temperature)
{
    if (humidity > poultry_config.humidity_max) {
        // High humidity - activate ventilation
        for (int i = 0; i < 4; i++) {
            actuator_set_state(i, ACTUATOR_STATE_ON);
        }
    } else if (humidity < poultry_config.humidity_min) {
        // Low humidity - optional humidifier
        // (Not implemented in base version)
    }
}
```

### Parameters
| Parameter | Default | Range | Description |
|-----------|---------|-------|-------------|
| humidity_min | 40% | 20-50% | Minimum humidity |
| humidity_max | 80% | 60-90% | Maximum humidity |
| humidity_optimal | 60% | 40-70% | Target humidity |

## Gas Control (Ammonia/CO2)

### Algorithm: Priority-Based Emergency Control

```
Gas Level
    │
    │           ┌─────────────────────────┐
    │           │                         │
    │           │     ALL FANS ON         │
    │           │    (Emergency Vent)      │
 MAX┼───────────┴─────────────────────────┘
    │
    │    ┌─────────────────────────┐
    │    │                         │
    │    │      PARTIAL FANS      │
    │    │      (Normal Vent)     │
    │    └─────────────────────────┘
    │
    │    ┌─────────────────────────┐
    │    │                         │
    │    │      VENTILATION        │
    │    │         OFF             │
  0 ──┴────┴─────────────────────────┴──────────► Time
```

### Implementation

```c
void control_gas_logic(float ammonia, float co2, float co)
{
    // Ammonia control
    if (ammonia > poultry_config.ammonia_max) {
        // Critical level - maximum ventilation
        for (int i = 0; i < 4; i++) {
            actuator_set_state(i, ACTUATOR_STATE_ON);
        }
    }
    
    // CO2 control
    if (co2 > poultry_config.co2_max) {
        // High CO2 - activate fans
        for (int i = 0; i < 4; i++) {
            actuator_set_state(i, ACTUATOR_STATE_ON);
        }
    }
    
    // CO control (critical - immediate action)
    if (co > poultry_config.co_max) {
        // Emergency - all fans on + alert
        for (int i = 0; i < 4; i++) {
            actuator_set_state(i, ACTUATOR_STATE_ON);
        }
        // Trigger emergency notification
    }
}
```

### Parameters
| Parameter | Default | Range | Description |
|-----------|---------|-------|-------------|
| ammonia_max | 25 ppm | 10-50 ppm | Maximum ammonia |
| co2_max | 3000 ppm | 1000-5000 ppm | Maximum CO2 |
| co_max | 50 ppm | 10-100 ppm | Maximum CO |

## Light Control

### Algorithm: Time-Based Scheduling

```
Light Level
    │
 100%┼──┐                                    ┌──────────
     │  │                                    │          │
     │  │                                    │          │
  50%┼──┤                                    │          │
     │  │                                    │          │
     │  │                                    │          │
   0%┼──┴────────────────────────────────────┴──────────┘
     │
     └──────────────────────────────────────────────────► Time
     6:00     12:00           18:00        24:00
```

### Implementation

```c
void control_light_logic(float light_level, uint8_t hour)
{
    if (hour >= 6 && hour <= 18) {
        // Daytime - maintain light level
        if (light_level < 300.0f) {
            // Too dim - turn on lights
            for (int i = 6; i < 8; i++) {
                actuator_set_state(i, ACTUATOR_STATE_ON);
            }
        } else {
            // Sufficient light - turn off
            for (int i = 6; i < 8; i++) {
                actuator_set_state(i, ACTUATOR_STATE_OFF);
            }
        }
    } else {
        // Nighttime - lights off
        for (int i = 6; i < 8; i++) {
            actuator_set_state(i, ACTUATOR_STATE_OFF);
        }
    }
}
```

## Water Management

### Algorithm: Level-Based Control

```c
void control_water_logic(float water_level)
{
    if (water_level < 30.0f) {
        // Low water - activate pumps
        actuator_set_state(10, ACTUATOR_STATE_ON);  // Pump 1
        actuator_set_state(11, ACTUATOR_STATE_ON);  // Pump 2
    } else if (water_level > 80.0f) {
        // Water level OK - deactivate pumps
        actuator_set_state(10, ACTUATOR_STATE_OFF);
        actuator_set_state(11, ACTUATOR_STATE_OFF);
    }
    // Between 30-80%: Maintain current state
}
```

## Control Modes

### Auto Mode
- System automatically adjusts based on sensor readings
- All control algorithms active
- Recommended for normal operation

### Manual Mode
- Manual control via MQTT/API
- Control algorithms bypassed
- Individual actuators can be controlled

### Scheduled Mode
- Time-based control schedules
- Override auto behavior at specific times
- Useful for feeding schedules

### Adaptive Mode
- Machine learning-based control
- Adjusts parameters based on historical data
- Requires additional configuration

## Emergency Procedures

### Emergency Stop
```c
esp_err_t control_system_emergency_stop(void)
{
    // Immediately deactivate all actuators
    actuator_emergency_stop_all();
    
    // Log emergency event
    monitoring_log_event("EMERGENCY STOP ACTIVATED", 1);
    
    // Set emergency flag
    control_state.emergency_stop = true;
    
    return ESP_OK;
}
```

### Emergency Conditions
1. **Critical Gas Levels**: CO > 100 ppm
2. **Temperature > 40°C**: Heat stroke risk
3. **Fire Detection**: Smoke sensor trigger
4. **Manual Emergency**: User-initiated

## Advanced Features

### Predictive Control
```c
// Future enhancement: Kalman filter for prediction
typedef struct {
    float predicted_value;
    float confidence;
    uint32_t update_time;
} prediction_t;
```

### Multi-Zone Control
```c
// Support for multiple zones in larger farms
typedef struct {
    uint8_t zone_id;
    char zone_name[32];
    poultry_config_t config;
    sensor_data_t *sensors;
} control_zone_t;
```

## Tuning Guidelines

### Temperature Tuning
1. Start with conservative ranges
2. Monitor bird behavior
3. Adjust based on mortality rates
4. Seasonal adjustments required

### Humidity Tuning
1. Keep between 50-70% for best results
2. High humidity increases ammonia problems
3. Low humidity causes respiratory issues

### Gas Tuning
1. Ammonia is primary concern
2. Lower is always better
3. Ventilation is primary solution

## Troubleshooting

### Oscillation
- **Symptom**: Fans turn on/off frequently
- **Cause**: Threshold too close together
- **Solution**: Increase hysteresis gap

### Delayed Response
- **Symptom**: Slow reaction to changes
- **Cause**: Control interval too long
- **Solution**: Reduce `control_interval_ms`

### Over-Correction
- **Symptom**: Large swings in conditions
- **Cause**: Aggressive control parameters
- **Solution**: Use gradual ramp-up
