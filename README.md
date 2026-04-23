# Robotics Projects Collection

This repository contains two Arduino-based robotics projects: an autonomous line-following car and a remote-controlled car with servo manipulation capabilities.

## Projects Overview

### 1. Autonomous Line-Following Car (`autonomous_car.ino`)
A smart car that follows black lines using infrared sensors and includes obstacle avoidance with ultrasonic sensor.

### 2. Remote-Controlled Car with Servo Arm (`remote_controlled_car.ino`)
An ESP32-based remote-controlled car with a 4-servo robotic arm, controllable via web interface.

---

## Project 1: Autonomous Line-Following Car

### Features
- **Line Following**: Uses 5 IR sensors for precise line tracking
- **Obstacle Avoidance**: Ultrasonic sensor detects obstacles within 15cm
- **Smart Navigation**: Handles intersections, turns, and dead ends
- **Motor Control**: Dual motor system with PWM speed control

### Hardware Requirements
- Arduino Uno/Nano
- 5x IR sensors (A0-A4)
- HC-SR04 ultrasonic sensor
- 2x DC motors with motor driver
- Jumper wires and breadboard

### Pin Configuration
```cpp
// Ultrasonic Sensor
const int trig = 10;
const int echo = 11;

// Motor Control
const int motorA1 = 9;
const int motorA2 = 6;
const int motorB1 = 5;
const int motorB2 = 3;

// IR Sensors
const int Pin_ss1 = A0;
const int Pin_ss2 = A1;
const int Pin_ss3 = A2;
const int Pin_ss4 = A3;
const int Pin_ss5 = A4;
```

### Key Functions
- `In_SenSor()`: Reads all IR sensors and returns binary pattern
- `forward(speed)`: Moves car forward at specified speed
- `Turn_right_90(speed)`: 90-degree right turn
- `Turn_left_90(speed)`: 90-degree left turn
- `Robot_stop()`: Emergency stop

### Line Detection Patterns
- `0b00000100`: Go straight
- `0b00000110`: Slight right turn
- `0b00001100`: Left turn
- `0b00011111`: Stop (end of line)

---

## Project 2: Remote-Controlled Car with Servo Arm

### Features
- **WiFi Control**: Creates its own hotspot for remote control
- **Web Interface**: Responsive HTML interface for all controls
- **4-Servo Robotic Arm**: 
  - Gripper (open/close)
  - Arm extension (extend/retract)
  - Base rotation (left/right)
  - Vertical movement (up/down)
- **Motor Control**: Differential drive system
- **Speed Control**: Variable speed with web slider
- **Safety Features**: Servo movement protection and timeout handling

### Hardware Requirements
- ESP32 development board
- 4x Servo motors (SG90 or similar)
- 2x DC motors with L298N motor driver
- Power supply (7.4V recommended)
- Jumper wires

### Pin Configuration
```cpp
// Servo Motors
GPIO 24: Gripper servo
GPIO 25: Arm servo
GPIO 26: Rotation servo
GPIO 27: Vertical servo

// Motor Driver
ENA (GPIO 15): Left motor speed
ENB (GPIO 5): Right motor speed
IN1 (GPIO 2): Left motor direction 1
IN2 (GPIO 4): Left motor direction 2
IN3 (GPIO 16): Right motor direction 1
IN4 (GPIO 17): Right motor direction 2
```

### WiFi Configuration
- **SSID**: `Robocar WebServer ESP32`
- **Password**: `Robocar@esp32`
- **Default IP**: `192.168.4.1`

### Web Interface Controls

#### Movement Controls
- **Forward/Backward**: Directional movement
- **Left/Right**: Turning controls
- **Speed Slider**: Adjustable speed (0-255)
- **Stop**: Emergency stop

#### Servo Controls
- **Gripper**: Open (`/mogap`) / Close (`/donggap`)
- **Arm**: Extend (`/duoi`) / Retract (`/co`)
- **Rotation**: Left (`/xoaytrai`) / Right (`/xoayphai`)
- **Vertical**: Up (`/servo4len`) / Down (`/servo4xuong`)

#### System Controls
- **Reset**: Return all servos to center position
- **Status**: Check current system state

### API Endpoints

| Endpoint | Function | Parameters |
|----------|----------|------------|
| `/` | Main web interface | None |
| `/tien` | Move forward | None |
| `/lui` | Move backward | None |
| `/trai` | Turn left | None |
| `/phai` | Turn right | None |
| `/stop` | Stop movement | None |
| `/speed` | Set speed | `value` (0-255) |
| `/mogap` | Open gripper | None |
| `/donggap` | Close gripper | None |
| `/co` | Retract arm | None |
| `/duoi` | Extend arm | None |
| `/xoaytrai` | Rotate left | None |
| `/xoayphai` | Rotate right | None |
| `/servo4len` | Move up | None |
| `/servo4xuong` | Move down | None |
| `/reset` | Reset all servos | None |
| `/status` | Get system status | None |

---

## Installation and Setup

### For Autonomous Car
1. Install Arduino IDE
2. Connect hardware according to pin configuration
3. Upload `autonomous_car.ino`
4. Place car on black line track
5. Power on and watch it follow the line

### For Remote-Controlled Car
1. Install Arduino IDE with ESP32 board support
2. Install required libraries:
   ```
   ESP32Servo library
   WiFi library (built-in)
   WebServer library (built-in)
   ```
3. Connect hardware according to pin configuration
4. Upload `remote_controlled_car.ino`
5. Connect to WiFi hotspot "Robocar WebServer ESP32"
6. Open browser and navigate to `192.168.4.1`

---

## Troubleshooting

### Autonomous Car Issues
- **Car doesn't follow line**: Check IR sensor connections and calibration
- **Motors not working**: Verify motor driver connections and power supply
- **Erratic behavior**: Adjust speed values (`td1`, `td2`, `td3`)

### Remote-Controlled Car Issues
- **Can't connect to WiFi**: Check ESP32 power and antenna
- **Servos not responding**: Verify servo power supply (separate 5V recommended)
- **Web interface not loading**: Ensure you're connected to the ESP32 hotspot
- **Servo movements jerky**: Check power supply capacity and servo connections

---

## Safety Considerations

1. **Power Supply**: Use appropriate voltage and current ratings
2. **Servo Protection**: Built-in movement delays prevent servo damage
3. **Emergency Stop**: Always accessible through web interface
4. **Heat Management**: Monitor motor driver and ESP32 temperatures
5. **Mechanical Limits**: Ensure servo movements don't exceed physical constraints

---

## Future Enhancements

### Possible Upgrades
- Camera integration for FPV control
- Bluetooth control as backup
- Battery level monitoring
- GPS navigation for autonomous mode
- Voice control integration
- Mobile app development

### Code Improvements
- PID control for smoother line following
- Machine learning for better obstacle detection
- OTA (Over-The-Air) firmware updates
- Data logging and analytics

---

## Contributing

Feel free to contribute to this project by:
1. Reporting bugs and issues
2. Suggesting new features
3. Submitting pull requests
4. Improving documentation

---

## License

This project is open-source and available under the MIT License.

---

## Contact

For questions, suggestions, or collaboration opportunities, please create an issue in this repository.

**Happy Building!**
