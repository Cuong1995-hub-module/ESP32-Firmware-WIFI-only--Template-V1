#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>
#include <DNSServer.h>
#include "BoardConfig.h"

// ============================================================
// Configuration / Constants
// ============================================================

#define RESET_HOLD_TIME 5000

#define RECONNECT_INTERVAL 10000
#define RECONNECT_TIMEOUT 60000

// ============================================================
// WiFi State
// ============================================================

enum class WiFiState
{
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    RECONNECTING,
    FAILED
};

// ============================================================
// WiFiManager Class
// ============================================================

class WiFiManager
{
public:
    void begin();
    void loop();

private:
    // Access Point / Captive Portal
    void startAP();

    // Web Interface
    void handleRoot();
    void handleScan();
    void handleConnect();
    void handleStatus();

    // WiFi Configuration / NVS
    void loadWiFiConfig();
    void saveWiFiConfig(
        const String& ssid,
        const String& password
    );
    void clearWiFiConfig();

    // Hardware Input
    void handleResetButton();

    DNSServer dnsServer;

    // WiFi state and timing
    WiFiState state = WiFiState::DISCONNECTED;
    unsigned long connectStartTime = 0;
    unsigned long reconnectStartTime = 0;
    unsigned long lastReconnectAttempt = 0;

    // Reset button state
    bool resetButtonPressed = false;
    bool resetTriggered = false;
    unsigned long resetButtonStartTime = 0;

    // Persisted WiFi configuration
    Preferences preferences;
    String savedSSID;
    String savedPassword;
};

#endif
