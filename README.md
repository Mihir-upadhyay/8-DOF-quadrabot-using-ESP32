# 8-DOF-quadrabot-using-ESP32
# 🕷️ Eight-Servo Quadruped Robot

A portable, battery-powered, wirelessly controlled **quadruped (4-legged) robot** with **8 servo motors (2 DOF per leg)**. An **ESP32** runs the gait engine and drives the servos through a **PCA9685** PWM driver, while a handheld **NodeMCU (ESP8266) + joystick** controller sends commands over **ESP-NOW**.

![Platform](https://img.shields.io/badge/platform-ESP32%20%7C%20ESP8266-blue)
![Protocol](https://img.shields.io/badge/wireless-ESP--NOW-green)
![DOF](https://img.shields.io/badge/DOF-8-orange)
![Status](https://img.shields.io/badge/status-working%20prototype-brightgreen)

<!-- Add a photo or GIF of the robot walking here -->
<!-- ![Quadruped robot](docs/images/robot.jpg) -->

📄 **Full engineering report:** [`docs/quadruped_robot_report.pdf`](docs/quadruped_robot_report.pdf)

---

## ✨ Features

- 🦿 **8 servos, 2 DOF per leg**: hip (swing) and knee (lift) joints
- 📡 **ESP-NOW control**: no router, no pairing, about **3.2 ms** average latency
- 🎛️ **PCA9685 PWM driver** over I²C: jitter-free 50 Hz servo signals, so the ESP32 CPU stays free
- 🚶 **Crawl gait** (static walk) with forward, backward, turn-left and turn-right motions
- 🔋 **Fully portable**: 2S Li-Po powered, about **42 min** runtime
- 🧩 **Modular design**: locomotion node and controller node are independent

---

## 🧱 System Architecture

```
┌──────────────────────────────┐         ESP-NOW (2.4 GHz)        ┌──────────────────────────────────┐
│       HANDHELD CONTROLLER    │ ───────────────────────────────▶ │          LOCOMOTION NODE         │
│  Joystick ─▶ NodeMCU (ESP8266)│        4-byte packet @ 20 Hz     │  ESP32 ─ I²C ─▶ PCA9685 ─▶ 8×SG90 │
└──────────────────────────────┘                                  └──────────────────────────────────┘
```

| Subsystem | Hardware | Role |
|---|---|---|
| Locomotion node | ESP32-WROOM-32 + PCA9685 + 8 servos | Receives commands, runs gait engine, drives servos |
| Controller node | NodeMCU v2 (ESP8266) + analog joystick | Reads joystick, applies deadband, transmits commands |
| Power | 2S Li-Po (7.4 V), 7805 (5 V), AMS1117-3.3 | Separate logic and servo rails, common ground |

---

## 🔧 Hardware

| Component | Qty | Notes |
|---|---|---|
| ESP32 WROOM-32 | 1 | Main controller, I²C master, ESP-NOW receiver |
| NodeMCU v2 (ESP8266) | 1 | Handheld controller, ESP-NOW transmitter |
| PCA9685 16-ch 12-bit PWM driver | 1 | I²C address `0x40`, channels 0–7 used |
| SG90 micro servo | 8 | 2 per leg (MG90S recommended as an upgrade) |
| Dual-axis analog joystick | 1 | ±10 % firmware deadband |
| Li-Po 2S 2000 mAh | 1 | 7.4 V |
| 3 mm acrylic sheet (or 3D-printed PLA) | 1 | Chassis |
| M3 screws & nuts, jumper wires, USB-UART cable | – | Assembly and programming |

**Estimated cost: ₹2,605.** See the report (Section 10) for the full Bill of Materials.

### Servo channel map

| Leg | Hip | Knee |
|---|---|---|
| Front-Left | CH 0 | CH 1 |
| Front-Right | CH 2 | CH 3 |
| Rear-Left | CH 4 | CH 5 |
| Rear-Right | CH 6 | CH 7 |

### Wiring: ESP32 ↔ PCA9685

| ESP32 | PCA9685 | Notes |
|---|---|---|
| GPIO 21 | SDA | 4.7 kΩ pull-up to 3.3 V |
| GPIO 22 | SCL | 4.7 kΩ pull-up to 3.3 V |
| 3.3 V | VCC | Logic supply |
| GND | GND | Common ground |
| 5–6 V rail | V+ | Servo power |
| GPIO 4 | OE (optional) | Active-low output enable |

### Wiring: NodeMCU ↔ Joystick

| NodeMCU | Joystick | Notes |
|---|---|---|
| A0 | VRx | X axis |
| D5 (GPIO 14) | VRy | Y axis (the ESP8266 has only one ADC, see limitations) |
| D6 (GPIO 12) | SW | Button, 10 kΩ pull-up |
| 3.3 V / GND | VCC / GND | – |

---

## 💾 Software

- **Frameworks:** Arduino Core for ESP32 and ESP8266
- **IDE:** Arduino IDE 2.x or VS Code + PlatformIO
- **Libraries:** `Wire.h`, [Adafruit PCA9685 PWM Servo Driver](https://github.com/adafruit/Adafruit-PWM-Servo-Driver-Library), `esp_now.h` (ESP32) / `espnow.h` (ESP8266)

### Control flow

1. The joystick X/Y values are read on the NodeMCU, mapped to −100…+100, and a deadband is applied.
2. A 4-byte `JoystickPacket` (`x`, `y`, `button`, `checksum`) is sent over ESP-NOW at 20 Hz.
3. The ESP32 receive callback stores the command, and the main loop updates the gait state machine.
4. The gait engine computes the 8 servo angles and writes them to the PCA9685 as 12-bit tick values (205 ticks = 0°, 410 ticks = 180°).

### Gait state machine

| State | Trigger | Action |
|---|---|---|
| `IDLE` | x = 0, y = 0 | Hold home position |
| `WALK_FWD` | y > threshold | Forward crawl gait |
| `WALK_BWD` | y < −threshold | Reverse crawl gait |
| `TURN_LEFT` | x < −threshold | Left legs back, right legs forward |
| `TURN_RIGHT` | x > threshold | Right legs back, left legs forward |

### Leg geometry (2-link planar leg)

Upper leg **L1 = 50 mm**, lower leg **L2 = 60 mm**. Inverse kinematics:

```
cos(θ₂) = (x² + z² − L1² − L2²) / (2·L1·L2)
θ₁      = atan2(z, x) − atan2(L2·sin θ₂, L1 + L2·cos θ₂)
```

---

## 🚀 Getting Started

### 1. Get the ESP32's MAC address

Flash a small sketch to the ESP32 that prints `WiFi.macAddress()` to the Serial Monitor (115200 baud).

### 2. Flash the controller (NodeMCU)

Put the ESP32's MAC address into the controller firmware:

```cpp
uint8_t esp32Mac[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};  // replace with your ESP32's MAC
```

Upload it to the NodeMCU.

### 3. Flash the robot (ESP32)

Install the Adafruit PCA9685 library, upload the locomotion firmware, and power the robot. The servos move to the home pose, and the robot is ready in about 2 s.

### 4. Drive

Push the joystick forward or back to walk, and left or right to turn.

> ⚠️ **Power note:** SG90 servos are rated for 4.8–6 V. Use a regulated 5 V rail for the servos (not raw 7.4 V), and share a common ground between all boards. All 8 servos stalled can draw about 1.6 A.

---

## 📊 Test Results

| Test | Result |
|---|---|
| PCA9685 detected on I²C (`0x40`) | ✅ Pass |
| All 8 servos sweep 0°–180° | ✅ Pass |
| Supply under full load | ✅ 3.29 V logic / 4.97 V servo |
| ESP-NOW average latency | ✅ 3.2 ms |
| Wireless range | ✅ Reliable at 8 m, degraded at 12 m through walls |
| Forward walk | ✅ 31 cm in 4.8 s |
| Backward walk | ✅ 28 cm in 5.1 s |
| Turn | ✅ 87° in 8 s |
| Battery runtime | ✅ 42 min (2000 mAh Li-Po) |

**Max speed:** about 6 cm/s (crawl gait). All 10 planned tests passed.

### Known limitations

- The ESP8266 has a single ADC, so reading both joystick axes needs a multiplexer or a workaround.
- ESP-NOW broadcast mode is best-effort, so occasional dropped packets can cause brief hesitation.
- Servo channel 5 oscillates at extreme angles and is limited to 10°–170° in firmware.
- The crawl gait is slow. A trot gait could roughly double the speed at the cost of stability.

---

## 🛣️ Roadmap

- [ ] Metal-gear MG90S servos
- [ ] OLED on the controller (battery level, mode)
- [ ] Unicast ESP-NOW with delivery confirmation
- [ ] Trot gait
- [ ] IMU (MPU-6050) for tilt correction
- [ ] Ultrasonic obstacle avoidance (HC-SR04)
- [ ] Mobile app controller
- [ ] 3-DOF legs (12 servos)
- [ ] ROS 2 + simulation (Gazebo / PyBullet), reinforcement-learning gaits

---

## 📁 Suggested Repository Structure

```
.
├── README.md
├── docs/
│   ├── quadruped_robot_report.pdf
│   └── images/              # photos, wiring diagrams, demo GIF
├── firmware/
│   ├── locomotion_esp32/    # gait engine + PCA9685 + ESP-NOW receiver
│   └── controller_nodemcu/  # joystick + ESP-NOW transmitter
├── hardware/
│   └── chassis/             # acrylic cut files / STL
└── LICENSE
```

---

## 📚 References

- Espressif Systems, *ESP32 Technical Reference Manual* and *ESP-NOW User Guide*
- NXP Semiconductors, *PCA9685 16-channel, 12-bit PWM I²C-bus controller*
- Adafruit PCA9685 PWM Servo Driver Library
- Siciliano et al., *Robotics: Modelling, Planning and Control*; Craig, *Introduction to Robotics*

---

## 📜 License

Add a license of your choice (for example MIT) and place it in a `LICENSE` file.
