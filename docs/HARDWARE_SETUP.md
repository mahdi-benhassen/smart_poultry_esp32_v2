# Smart Poultry System - Hardware Setup Guide

## Overview

This guide covers the complete hardware setup for the Smart Poultry System based on ESP32.

## Power Requirements

### Power Supply
- **Main Power**: 12V DC 5A (for fans, pumps)
- **ESP32 Power**: 5V USB or 5V regulator from 12V
- **Heater Power**: 220V AC (via relay modules)

### Power Distribution
```
12V DC Power Supply
    ├── 12V → Fans (4x)
    ├── 12V → Water Pumps (2x)
    ├── 5V Regulator → ESP32
    └── 5V → Sensors (DHT22, BME280)
```

## Sensor Connections

### 1. DHT22 Temperature & Humidity Sensor

**Pinout:**
```
DHT22 Pin    →    ESP32 Pin
---------------------------
VCC         →    3.3V or 5V
GND         →    GND
DATA        →    GPIO 4
```

**Wiring Notes:**
- Use 4.7KΩ pull-up resistor on DATA line
- Keep wire length under 20 meters
- Avoid near heat sources

### 2. BME280 Environmental Sensor (I2C)

**Pinout:**
```
BME280 Pin   →    ESP32 Pin
---------------------------
VCC         →    3.3V
GND         →    GND
SDA         →    GPIO 21
SCL         →    GPIO 22
```

**I2C Address:** 0x76 (default)

### 3. MQ Series Gas Sensors

**MQ-135 (Ammonia/CO2):**
```
MQ-135 Pin   →    ESP32 Pin
---------------------------
VCC         →    5V
GND         →    GND
A0 (Analog) →    GPIO 4 (ADC1_CH4)
D0 (Digital)→    Optional
```

**MQ-7 (CO):**
```
MQ-7 Pin    →    ESP32 Pin
---------------------------
VCC         →    5V
GND         →    GND
A0 (Analog) →    GPIO 5 (ADC1_CH5)
```

**MQ-2 (Methane/LPG):**
```
MQ-2 Pin    →    ESP32 Pin
---------------------------
VCC         →    5V
GND         →    GND
A0 (Analog) →    GPIO 3 (ADC1_CH3)
```

**Important:**
- Pre-heat sensors for 24-48 hours before calibration
- Use voltage divider for 5V to 3.3V if needed
- Calibrate in clean air (R0 determination)

### 4. Weight Sensor (HX711)

**Pinout:**
```
HX711       →    ESP32 Pin    →    Load Cell
-------------------------------------------------
VCC         →    5V
GND         →    GND
DT (Data)   →    GPIO 26
SCK (Clock) →    GPIO 27
```

**Load Cell Wiring:**
- Red: E+ (Excitation+)
- Black: E- (Excitation-)
- White: A+ (Signal+)
- Green: A- (Signal-)

### 5. Water Level Sensors

**Float Switch Type:**
```
Float Sensor →    ESP32 Pin
---------------------------
VCC         →    3.3V
GND         →    GND
NO (Normally Open) → GPIO 32 (Low level)
NO (Normally Open) → GPIO 33 (Mid level)
NO (Normally Open) → GPIO 34 (High level)
```

**Ultrasonic Type (HC-SR04):**
```
HC-SR04     →    ESP32 Pin
---------------------------
VCC         →    5V
GND         →    GND
Trig        →    GPIO 32
Echo        →    GPIO 33
```

## Actuator Connections

### 1. DC Fans (12V)

**Wiring Diagram:**
```
12V Power → Fan → N-Channel MOSFET → ESP32
                            |
                           GND

ESP32 Pin  → MOSFET Gate
Source     → GND
Drain      → Fan Negative
```

**PWM Control:**
- GPIO 2, 4, 5, 18 for 4 fans
- Use MOSFET (IRF540N recommended)
- PWM frequency: 25kHz

### 2. Heaters (220V AC)

**Wiring:**
```
220V AC → Relay Module → ESP32 → Heater
                  |
                 GND

ESP32 Pin  → GPIO 19, 21
VCC        → 5V
GND        → GND
NO         → Heater Live
COM        → AC Live
```

**Relay Module:** 5V 10A Relay (Songle SRD-05VDC)

### 3. LED Lights

**Wiring:**
```
12V Power → LED Strip → MOSFET → ESP32
                              |
                             GND

ESP32 Pin  → GPIO 22, 23, 25, 26
```

### 4. Servo Motors (Feeders)

**Wiring:**
```
Servo       →    ESP32 Pin
---------------------------
VCC         →    5V
GND         →    GND
Signal      →    GPIO 25, 26
```

### 5. Water Pumps (12V DC)

**Wiring:**
```
12V Power → Pump → Relay → ESP32
                     |
                    GND

ESP32 Pin  → GPIO 27, 14
```

## Complete Pin Assignment Table

| GPIO | Function | Device |
|------|----------|--------|
| 2 | Digital Output | Fan 1 PWM |
| 3 | ADC1_CH3 | MQ-2 Gas |
| 4 | ADC1_CH4 / Digital | DHT22 Data / MQ-135 |
| 5 | ADC1_CH5 | MQ-7 Gas |
| 6 | ADC1_CH6 | Weight Sensor |
| 14 | Digital Output | Water Pump 2 |
| 18 | Digital Output | Fan 4 PWM |
| 19 | Digital Output | Heater 1 |
| 21 | Digital Output | I2C SDA / Heater 2 |
| 22 | Digital Output | I2C SCL / Light 1 |
| 23 | Digital Output | Light 2 |
| 25 | Digital Output | Servo 1 / Light 3 |
| 26 | Digital Output | HX711 DT / Servo 2 / Light 4 |
| 27 | Digital Output | HX711 SCK / Water Pump 1 |
| 32 | Digital Input | Water Level Low |
| 33 | Digital Input | Water Level Mid |
| 34 | Digital Input | Water Level High |

## Enclosure Recommendations

### Dimensions
- Minimum: 150mm x 100mm x 50mm
- Recommended: 200mm x 150mm x 80mm

### Ventilation
- Include ventilation slots
- Keep ESP32 away from heat-generating components
- Separate high-voltage and low-voltage sections

### Mounting
- Use DIN rail mount for industrial applications
- PCB standoffs for mounting
- Cable glands for sensor/actuator wiring

## Safety Considerations

1. **Electrical Safety**
   - Use proper grounding
   - Include fuse protection
   - Isolate 220V components

2. **Fire Prevention**
   - Use fire-rated enclosures
   - Keep heating elements away from flammables
   - Include thermal fuses

3. **Water Protection**
   - Use waterproof enclosures for outdoor sensors
   - Proper cable glands with IP65+ rating
   - Keep electronics above water level

## Testing

### Initial Power-On Checklist
- [ ] Verify all 5V/3.3V rails
- [ ] Check ESP32 boots without errors
- [ ] Test each sensor individually
- [ ] Test each actuator
- [ ] Verify WiFi connectivity

### Sensor Calibration
- Temperature: Compare with calibrated thermometer
- Gas sensors: Measure in known clean air
- Weight: Use calibrated weights

## Troubleshooting

### No Sensor Reading
- Check power supply
- Verify wiring connections
- Test with multimeter

### Actuator Not Working
- Check relay module power
- Verify GPIO output
- Test with LED first

### WiFi Connection Issues
- Check antenna connection
- Verify 2.4GHz network
- Check power supply stability
