# ESP32 Firmware WiFi-Only Template

A reusable and modular WiFi firmware template for ESP32-based IoT projects.

This project provides a standalone `WiFiManager` for handling WiFi
configuration, connection management, automatic reconnection, AP
fallback, and captive portal setup.

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
- Connection state machine
- Board-specific configuration
- Modular and reusable architecture

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
├── OTAManager
│   └── Firmware Update
│
└── Application
    └── User Application
```

The `WiFiManager` is responsible only for network connectivity.
Application logic remains separated from the WiFi layer.

`OTAManager` is planned as the next infrastructure module.

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

## Project Structure

```text
ESP32-Firmware-WIFI-only--Template
│
├── include/
│   ├── BoardConfig.h
│   └── README
│
├── lib/
│   ├── README
│   └── WiFiManager/
│       ├── WiFiManager.cpp
│       └── WiFiManager.h
│
├── src/
│   └── main.cpp
│
├── test/
│   └── README
│
├── platformio.ini
├── .gitignore
└── README.md
```

## Getting Started

### 1. Clone the Repository

```bash
git clone https://github.com/Cuong1995-hub-module/ESP32-Firmware-WIFI-only--Template.git
cd ESP32-Firmware-WIFI-only--Template
```

Open the project with VS Code and PlatformIO.

### 2. Select the ESP32 Board

The target board is configured in `platformio.ini`.

Example for a classic ESP32:

```ini
[env:esp32]

platform = https://github.com/pioarduino/platform-espressif32/releases/download/stable/platform-espressif32.zip

board = esp32dev
framework = arduino
monitor_speed = 115200

build_flags =
    -I include
    -D BOARD_ESP32
```

Board-specific settings are defined in:

```text
include/BoardConfig.h
```

### 3. Initialize WiFiManager

The application only needs to create a `WiFiManager` instance and call
`begin()` and `loop()`:

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

WiFiManager then handles the WiFi connection and recovery process.

## How It Works

WiFiManager handles the complete WiFi lifecycle of the ESP32.

### 1. Device Startup

When the ESP32 starts, WiFiManager loads the saved WiFi credentials
from NVS.

If saved credentials are available, the device automatically attempts
to connect to the saved WiFi network.

### 2. First-Time Setup

If no WiFi configuration is stored, or the device enters setup mode,
WiFiManager starts a configuration Access Point.

```text
SSID: ESP32-Setup
```

Connect a phone or computer to this network.

The configuration interface is available at:

```text
http://192.168.4.1
```

The captive portal can also redirect the client to the configuration
page automatically.

### 3. Configure WiFi

From the configuration page, the user can:

1. Scan available WiFi networks.
2. Select the desired SSID.
3. Enter the WiFi password.
4. Press **Connect**.

WiFiManager attempts to connect using the supplied credentials.

If the connection succeeds, the credentials are saved to NVS.

The setup AP is then stopped automatically and the ESP32 continues
operating as a normal WiFi station.

### 4. Normal Operation

After connecting successfully, the application continues running while
WiFiManager monitors the connection state.

The application does not need to implement its own WiFi reconnect logic.

It only needs to keep calling:

```cpp
wifiManager.loop();
```

### 5. WiFi Connection Lost

If the WiFi connection is lost, WiFiManager automatically attempts to
reconnect using the saved credentials.

The application does not need to restart the device or reconfigure WiFi
manually.

### 6. Connection Failure and AP Fallback

If the device cannot reconnect within the configured timeout,
WiFiManager enters its recovery process and starts the setup AP.

The user can then connect to:

```text
ESP32-Setup
```

and open:

```text
http://192.168.4.1
```

The WiFi configuration can be changed without reflashing the firmware.

While the setup AP is active, WiFiManager can continue attempting to
recover the saved WiFi connection in the background.

If the saved network becomes available again and the connection succeeds,
the setup AP is stopped automatically.

### 7. Reset WiFi Configuration

The hardware BOOT button provides a manual recovery mechanism.

Hold the BOOT button for approximately **5 seconds** to clear the saved
WiFi configuration.

The device then restarts and can enter WiFi setup mode again.

This is useful when the saved WiFi credentials are no longer valid.

## WiFi Recovery Flow

```text
                    Start
                      │
                      ▼
             Load WiFi config
                      │
             ┌────────┴────────┐
             │                 │
       Config exists       No config
             │                 │
             ▼                 ▼
      Connect to saved      Start AP
           WiFi                │
             │                 ▼
        ┌────┴────┐      Captive Portal
        │         │           │
    Success     Timeout       ▼
        │         │      Select WiFi
        │         │           │
        │         │      Enter password
        │         │           │
        │         │           ▼
        │         │      Try connection
        │         │       │         │
        │         │    Success     Failed
        │         │       │         │
        ▼         ▼       ▼         ▼
     Connected  Start AP  Save NVS  Retry
        │         │
        │         └──── Background reconnect
        │
        ▼
   Normal operation
        │
        ▼
    WiFi lost
        │
        ▼
   Auto reconnect
        │
   ┌────┴────┐
   │         │
Success    Timeout
   │         │
   ▼         ▼
Connected  AP Fallback
```

## Web Configuration Interface

The built-in web interface provides:

- Available WiFi network scanning
- SSID selection
- Password input
- Connection status
- RSSI information
- Connection state feedback
- WiFi setup through a browser

The interface is intended to make initial configuration and recovery
possible without serial commands.

## Hardware Reset

Holding the BOOT button for approximately **5 seconds** clears the saved
WiFi configuration.

This provides a hardware recovery mechanism when the saved WiFi network
or password is no longer valid.

The exact BOOT pin is selected through `BoardConfig.h` for each supported
board.

## Design Philosophy

The project is intentionally focused on the infrastructure required by
an ESP32 IoT device:

```text
Application
     │
     ▼
WiFiManager
     │
     ▼
Network Connectivity
```

WiFiManager handles network connectivity and recovery, while the
application remains responsible for its own device logic and protocols.

OTA is kept as a separate module:

```text
ESP32 Firmware
     │
     ├── WiFiManager
     │
     ├── OTAManager
     │
     └── Application
```

This separation keeps the WiFi layer reusable and makes future firmware
updates easier to integrate.

## Usage Example

A minimal application can be:

```cpp
#include "WiFiManager.h"

WiFiManager wifiManager;

void setup()
{
    Serial.begin(115200);

    wifiManager.begin();
}

void loop()
{
    wifiManager.loop();

    // Application code
}
```

The application can then add its own functionality on top of the
network layer.

## Roadmap

### V1.0.0

- [x] WiFi STA mode
- [x] NVS WiFi configuration
- [x] AP fallback
- [x] Captive Portal
- [x] WiFi scanning
- [x] RSSI display
- [x] Automatic reconnection
- [x] Connection timeout and recovery
- [x] Hardware BOOT reset
- [x] Multi-board configuration
- [x] Web-based configuration interface

### V2.0.0
add

- [ ] OTAManager
- [ ] OTA firmware update

## Version

**Current version: v1.0.0 — WiFiManager**

WiFiManager V1 is the stable WiFi foundation for future ESP32 IoT
firmware projects.
