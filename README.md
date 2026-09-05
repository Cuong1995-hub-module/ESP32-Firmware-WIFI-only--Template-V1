# ESP32 Firmware WiFi-Only Template

A reusable and modular WiFi firmware template for ESP32-based IoT projects.

This project provides a standalone `WiFiManager` designed to handle
WiFi configuration, connection management, automatic reconnection,
AP fallback, and captive portal setup.

## Features

- WiFi STA mode
- Saved WiFi credentials using NVS
- Automatic WiFi reconnection
- Connection timeout and recovery
- AP fallback when WiFi connection fails
- Captive Portal for WiFi configuration
- WiFi network scanning
- RSSI display
- Web-based configuration interface
- Hardware BOOT button reset
- Non-blocking connection state machine
- Board-specific configuration
- Modular architecture for reuse across projects

## Architecture

```text
ESP32 Firmware
│
├── WiFiManager
│   ├── STA
│   ├── AP Fallback
│   ├── Captive Portal
│   ├── NVS Configuration
│   ├── Auto Reconnect
│   └── Hardware Reset
│
└── Application
    ├── MQTT
    ├── HTTP
    ├── Sensors
    ├── RFID
    └── Other Application Logic
```

The `WiFiManager` is responsible only for network connectivity.
Application-level protocols and device logic remain separated from
the WiFi layer.

## Supported Boards

Currently tested:

- ESP32
- ESP32-S3

Board-specific settings are handled through:

```text
include/BoardConfig.h
```

This allows additional ESP32 variants to be added without modifying
the core WiFiManager logic.

## WiFi Recovery Flow

```text
             Start
               │
               ▼
        Load WiFi config
               │
               ▼
       Connect to saved AP
          │          │
       Success      Timeout
          │          │
          ▼          ▼
      Connected   Start AP
                     │
                     ▼
              Captive Portal
                     │
                     ▼
             Enter WiFi config
                     │
                     ▼
              Try connection
                     │
             ┌───────┴───────┐
             ▼               ▼
          Success           Failed
             │               │
             ▼               ▼
         Connected       Retry / AP
```

## Hardware Reset

Holding the BOOT button for 5 seconds clears the saved WiFi
configuration and restarts the device into setup mode.

This provides a hardware recovery mechanism when the saved WiFi
configuration is no longer valid.

## Project Structure

```text
ESP32-Firmware-WIFI-only--Template
│
├── include/
│   └── BoardConfig.h
│
├── lib/
│   └── WiFiManager/
│       ├── WiFiManager.cpp
│       └── WiFiManager.h
│
├── src/
│   └── main.cpp
│
├── test/
├── platformio.ini
└── .gitignore
```

## Usage

Create a `WiFiManager` instance and initialize it in the application:

```cpp
#include "WiFiManager.h"

WiFiManager wifiManager;

void setup()
{
    wifiManager.begin();
}

void loop()
{
    wifiManager.loop();
}
```

The application can then build additional functionality on top of
the network layer.

## Design Philosophy

The project follows a modular firmware architecture:

```text
WiFiManager
     │
     ▼
Network Connectivity
     │
     ├── OTAManager
```

Each module has a clearly defined responsibility, making the firmware
easier to reuse, test, maintain, and extend.

## Roadmap

### V1.0.0

- [x] WiFiManager
- [x] STA mode
- [x] AP fallback
- [x] Captive Portal
- [x] NVS configuration
- [x] Automatic reconnection
- [x] Hardware reset
- [x] WiFi scanning
- [x] Multi-board configuration

### V2.0.0

- [ ] OTAManager
- [ ] OTA firmware update


## Version

Current version:

**v1.0.0 — WiFiManager**

WiFiManager V1 is considered a stable foundation for future ESP32
IoT firmware projects.
