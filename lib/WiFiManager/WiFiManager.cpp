#include "WiFiManager.h"
#include <WiFi.h>
#include <WebServer.h>

// ============================================================
// WiFiManager - Configuration
// ============================================================

#define WIFI_MANAGER_VERSION "1.0.0"
#define WIFI_MANAGER_AUTHOR  "CuongPham_ICTU"
#define INITIAL_CONNECT_TIMEOUT 15000

// ============================================================
// Global Objects
// ============================================================

static WebServer server(80);

// ============================================================
// Initialization
// ============================================================

void WiFiManager::begin()
{
    Serial.println();
    Serial.println("================================");
    Serial.println("ESP32 WiFi Manager");
    Serial.print("Version : ");
    Serial.println(WIFI_MANAGER_VERSION);
    Serial.print("Author  : ");
    Serial.println(WIFI_MANAGER_AUTHOR);
    Serial.println("================================");

    pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);

    // WiFiManager owns the reconnect state machine.
    // Disable Arduino automatic reconnect to avoid competing
    // connection attempts with our 10-second retry logic.
    WiFi.setAutoReconnect(false);

    loadWiFiConfig();

    if (savedSSID.length() > 0)
    {
        Serial.println("Saved WiFi found");
        Serial.println("Starting STA connection...");

        state = WiFiState::CONNECTING;
        connectStartTime = millis();

        WiFi.mode(WIFI_STA);

        WiFi.begin(
            savedSSID.c_str(),
            savedPassword.c_str()
        );
    }
    else
    {
        Serial.println("No WiFi configuration");
        startAP();
    }

    server.on("/", [this]()
    {
        handleRoot();
    });

    server.on("/scan", [this]()
    {
        handleScan();
    });

    server.on("/connect", HTTP_POST, [this]()
    {
        handleConnect();
    });

    server.on("/status", [this]()
    {
        handleStatus();
    });

    server.on("/generate_204", [this]()
    {
        server.sendHeader("Location", "/", true);
        server.send(302, "text/plain", "");
    });

    server.on("/hotspot-detect.html", [this]()
    {
        server.sendHeader("Location", "/", true);
        server.send(302, "text/plain", "");
    });

    server.on("/connecttest.txt", [this]()
    {
        server.sendHeader("Location", "/", true);
        server.send(302, "text/plain", "");
    });

    server.on("/ncsi.txt", [this]()
    {
        server.sendHeader("Location", "/", true);
        server.send(302, "text/plain", "");
    });

    server.onNotFound([this]()
    {
        server.sendHeader("Location", "/", true);
        server.send(302, "text/plain", "");
    });
    server.begin();

    Serial.println("Web server started");
}

// ============================================================
// Main Loop / State Machine
// ============================================================

void WiFiManager::loop()
{
    server.handleClient();
    dnsServer.processNextRequest();

    handleResetButton();

    // Initial STA connection
    if (state == WiFiState::CONNECTING)
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            state = WiFiState::CONNECTED;

            dnsServer.stop();
            WiFi.softAPdisconnect(true);
            WiFi.mode(WIFI_STA);

            Serial.println();
            Serial.println("WiFi connected!");

            Serial.print("SSID: ");
            Serial.println(WiFi.SSID());

            Serial.print("IP: ");
            Serial.println(WiFi.localIP().toString());

            Serial.println("Setup AP stopped");
        }
        else if (millis() - connectStartTime >= INITIAL_CONNECT_TIMEOUT)
        {
            Serial.println();
            Serial.println("WiFi connection failed");
            Serial.println("Switching to AP setup...");

            WiFi.disconnect();

            state = WiFiState::FAILED;

            startAP();

            // Start background reconnect timer
            reconnectStartTime = millis();
            lastReconnectAttempt = millis();
        }
    }

    // Monitor an established STA connection
    else if (state == WiFiState::CONNECTED)
    {
        if (WiFi.status() != WL_CONNECTED)
        {
            Serial.println();
            Serial.println("WiFi connection lost");
            Serial.println("Starting reconnect...");
            Serial.println("Waiting up to 60 seconds...");

            state = WiFiState::RECONNECTING;

            reconnectStartTime = millis();
            lastReconnectAttempt = millis();
        }
    }

    // Reconnect a previously established STA connection
    else if (state == WiFiState::RECONNECTING)
    {
        // The STA connection recovered before the retry window expired.
        if (WiFi.status() == WL_CONNECTED)
        {
            state = WiFiState::CONNECTED;

            Serial.println();
            Serial.println("WiFi reconnected!");

            Serial.print("SSID: ");
            Serial.println(WiFi.SSID());

            Serial.print("IP: ");
            Serial.println(WiFi.localIP().toString());

            // The setup AP is no longer needed after STA reconnects.
            dnsServer.stop();
            WiFi.softAPdisconnect(true);
            WiFi.mode(WIFI_STA);

            Serial.println("Setup AP stopped");
        }

        // Keep reconnecting in the background while the setup portal is available.
        else if (millis() - reconnectStartTime >= RECONNECT_TIMEOUT)
        {
            Serial.println();
            Serial.println("WiFi reconnect timeout");
            Serial.println("No connection for 60 seconds");
            Serial.println("Starting AP setup...");
            Serial.println("Background reconnect remains active");


            state = WiFiState::FAILED;

            // Keep STA available while the AP provides WiFi setup.
            startAP();

            // Restart the background reconnect timer.
            reconnectStartTime = millis();
            lastReconnectAttempt = millis();
        }

        // Retry at the configured interval instead of competing with auto reconnect.
        else if (millis() - lastReconnectAttempt >= RECONNECT_INTERVAL)
        {
            lastReconnectAttempt = millis();

            Serial.println("Retrying WiFi connection...");

            WiFi.reconnect();
        }
    }

    // Keep the setup AP running while reconnecting in the background.
    else if (state == WiFiState::FAILED)
    {
        // A saved STA connection recovered while the setup AP was active.
        if (WiFi.status() == WL_CONNECTED)
        {
            state = WiFiState::CONNECTED;

            Serial.println();
            Serial.println("WiFi reconnected!");
            Serial.println("Stopping Setup AP...");

            dnsServer.stop();
            WiFi.softAPdisconnect(true);
            WiFi.mode(WIFI_STA);

            Serial.print("SSID: ");
            Serial.println(WiFi.SSID());

            Serial.print("IP: ");
            Serial.println(WiFi.localIP().toString());

            Serial.println("Setup AP stopped");
        }

        // Retry the saved network at the configured interval.
        else if (millis() - lastReconnectAttempt >= RECONNECT_INTERVAL)
        {
            lastReconnectAttempt = millis();

            Serial.println("Retrying saved WiFi connection...");

            WiFi.reconnect();
        }
    }
}

// ============================================================
// Access Point / Captive Portal
// ============================================================

void WiFiManager::startAP()
{
    // Keep STA active so ESP32 can reconnect
    // while the setup AP is running.
    WiFi.mode(WIFI_AP_STA);

    WiFi.softAP("ESP32-Setup");

    IPAddress apIP = WiFi.softAPIP();

    dnsServer.start(
        53,
        "*",
        apIP
    );

    Serial.println("AP started");
    Serial.print("AP IP: ");
    Serial.println(apIP);
    Serial.println("Captive Portal started");
}

// ============================================================
// Web Interface
// ============================================================

void WiFiManager::handleRoot()
{
    String html = R"rawliteral(
<!DOCTYPE html>
<html>

<head>

<meta name="viewport"
      content="width=device-width, initial-scale=1">

<title>ESP32 WiFi Setup</title>

<style>

body
{
    margin: 0;
    font-family: Arial, sans-serif;
    background: linear-gradient(135deg, #101820, #1d3557);
    min-height: 100vh;

    display: flex;
    justify-content: center;
    align-items: center;
}

.card
{
    width: 90%;
    max-width: 420px;

    background: white;
    padding: 25px;

    border-radius: 18px;

    box-shadow:
        0 10px 40px rgba(0,0,0,0.35);
}

h2
{
    margin-top: 0;
    color: #1d3557;
}

label
{
    display: block;
    margin-top: 15px;
    margin-bottom: 6px;
    font-weight: bold;
}

select,
input
{
    width: 100%;
    box-sizing: border-box;

    padding: 12px;

    border: 1px solid #ccc;
    border-radius: 10px;

    font-size: 16px;
}

button
{
    width: 100%;

    margin-top: 15px;

    padding: 12px;

    border: none;
    border-radius: 10px;

    background: #1d3557;
    color: white;

    font-size: 16px;
    font-weight: bold;

    cursor: pointer;
}

button:hover
{
    background: #457b9d;
}

#status
{
    margin-top: 15px;
    text-align: center;
    font-weight: bold;
    line-height: 1.6;
}

.footer
{
    margin-top: 22px;
    padding-top: 14px;
    border-top: 1px solid #e5e5e5;
    text-align: center;
    color: #777;
    font-size: 12px;
    line-height: 1.6;
}

</style>

</head>

<body>

<div class="card">

<h2>ESP32 WiFi Setup</h2>

<form id="wifiForm">

    <label>WiFi Network</label>

    <select id="ssid" name="ssid">
        <option>Press Scan</option>
    </select>

    <button type="button" onclick="scanWiFi()">
        Scan WiFi
    </button>

    <p id="status"></p>

    <label>Password</label>

    <input
        id="password"
        name="password"
        type="password"
        placeholder="WiFi Password">

   <button id="connectButton" type="submit">
        Connect
    </button>

</form>

    <div class="footer">
        WiFiManager v1.0.0<br>
        Made by CuongPham_ICTU
    </div>

</div>

<script>

function scanWiFi()
{
    document.getElementById("status").innerText =
        "Scanning WiFi...";

    fetch("/scan")
    .then(response => response.json())
    .then(data =>
    {
        let select =
            document.getElementById("ssid");

        select.innerHTML = "";

        data.forEach(network =>
        {
            let option =
                document.createElement("option");

            option.value = network.ssid;

            option.text =
                network.ssid + " (" + network.rssi + " dBm)";

            select.appendChild(option);
        });

        document.getElementById("status").innerText =
            "Scan complete";
    })
    .catch(() =>
    {
        document.getElementById("status").innerText =
            "Scan failed";
    });
}

document.getElementById("wifiForm").addEventListener("submit", function(event)
{
    event.preventDefault();

    const form = event.target;
    const button = document.getElementById("connectButton");
    const status = document.getElementById("status");

    button.disabled = true;
    button.innerText = "Connecting...";
    status.innerText = "Connecting to WiFi...";

    fetch("/connect",
    {
        method: "POST",
        body: new FormData(form)
    })
    .then(response =>
    {
        if (!response.ok)
            throw new Error("Connection request failed");

        checkConnectionStatus();
    })
    .catch(() =>
    {
        status.innerText = "Connection request failed";
        button.disabled = false;
        button.innerText = "Connect";
    });
});


function checkConnectionStatus()
{
    fetch("/status")
    .then(response => response.json())
    .then(data =>
    {
        if (data.status === "connected")
        {
            document.getElementById("status").innerText =
                "WiFi connected!";

            return;
        }

        if (data.status === "failed")
        {
            window.location.href = "/?error=wifi";
            return;
        }

        setTimeout(checkConnectionStatus, 500);
    })
    .catch(() =>
    {
        setTimeout(checkConnectionStatus, 500);
    });
}


const params = new URLSearchParams(window.location.search);

if (params.get("error") === "wifi")
{
    document.getElementById("status").innerText =
        "WiFi connection failed. Please check your password and try again.";
}
</script>

</body>

</html>
)rawliteral";

    server.send(
        200,
        "text/html",
        html
    );
}

void WiFiManager::handleScan()
{
    int count = WiFi.scanNetworks();

    String json = "[";

    for (int i = 0; i < count; i++)
    {
        if (i > 0)
            json += ",";

        json += "{";
        json += "\"ssid\":\"";
        json += WiFi.SSID(i);
        json += "\",";
        json += "\"rssi\":";
        json += String(WiFi.RSSI(i));
        json += "}";
    }

    json += "]";

    server.send(
        200,
        "application/json",
        json
    );

    WiFi.scanDelete();
}

void WiFiManager::handleConnect()
{
    String ssid =
        server.arg("ssid");

    String password =
        server.arg("password");

    if (ssid.length() == 0)
    {
        server.send(
            400,
            "text/plain",
            "SSID is empty"
        );

        return;
    }

    Serial.println();
    Serial.println("Connecting to:");
    Serial.println(ssid);

    state = WiFiState::CONNECTING;

    connectStartTime = millis();

    saveWiFiConfig(ssid, password);

    // Keep the newly submitted credentials in memory for this attempt.
    savedSSID = ssid;
    savedPassword = password;

    // Cancel any previous STA connection attempt before
    // applying the new credentials.
    WiFi.disconnect(false, false);
    delay(100);

    // Keep AP + STA mode while applying new credentials.
    // This allows the user to continue using the setup portal
    // until the new WiFi connection succeeds.
    WiFi.mode(WIFI_AP_STA);

    WiFi.begin(
        savedSSID.c_str(),
        savedPassword.c_str()
    );

    server.send(
        200,
        "text/plain",
        "OK"
    );
}

void WiFiManager::handleStatus()
{
    String status;

    switch (state)
    {
        case WiFiState::DISCONNECTED:
            status = "disconnected";
            break;
        case WiFiState::CONNECTING:
            status = "connecting";
            break;
        case WiFiState::CONNECTED:
            status = "connected";
            break;
        case WiFiState::RECONNECTING:
            status = "reconnecting";
            break;
        case WiFiState::FAILED:
            status = "failed";
            break;
    }

    String json = "{";

    json += "\"status\":\"";
    json += status;
    json += "\"";

    if (state == WiFiState::CONNECTED)
    {
        json += ",\"ssid\":\"";
        json += WiFi.SSID();
        json += "\"";

        json += ",\"ip\":\"";
        json += WiFi.localIP().toString();
        json += "\"";
    }

    json += "}";

    server.send(
        200,
        "application/json",
        json
    );
}

// ============================================================
// WiFi Configuration / NVS
// ============================================================

void WiFiManager::loadWiFiConfig()
{
    preferences.begin("wifi", true);

    savedSSID =
        preferences.getString("ssid", "");

    savedPassword =
        preferences.getString("password", "");

    preferences.end();

    Serial.println();
    Serial.println("Loading WiFi configuration...");

    if (savedSSID.length() > 0)
    {
        Serial.print("Saved SSID: ");
        Serial.println(savedSSID);
    }
    else
    {
        Serial.println("No saved WiFi configuration");
    }
}

void WiFiManager::saveWiFiConfig(
    const String& ssid,
    const String& password
)
{
    preferences.begin("wifi", false);

    preferences.putString("ssid", ssid);
    preferences.putString("password", password);

    preferences.end();

    Serial.println();
    Serial.println("WiFi configuration saved");
}

void WiFiManager::clearWiFiConfig()
{
    preferences.begin("wifi", false);

    preferences.clear();

    preferences.end();

    Serial.println();
    Serial.println("WiFi configuration cleared");
}

// ============================================================
// Hardware Input
// ============================================================

void WiFiManager::handleResetButton()
{
    bool pressed = (digitalRead(RESET_BUTTON_PIN) == LOW);

    // Detect the start of a button press.
    if (pressed && !resetButtonPressed)
    {
        resetButtonPressed = true;
        resetTriggered = false;
        resetButtonStartTime = millis();

        Serial.println();
        Serial.println("Reset button pressed");
        Serial.println("Hold for 5 seconds to reset WiFi");
    }

    // Confirm the reset only after the button is held long enough.
    if (pressed && resetButtonPressed && !resetTriggered)
    {
        unsigned long elapsed =
            millis() - resetButtonStartTime;

        if (elapsed >= RESET_HOLD_TIME)
        {
            Serial.println();
            Serial.println("WiFi reset confirmed!");

            clearWiFiConfig();

            resetTriggered = true;

            Serial.println("Release BOOT button");
        }
    }

    // Apply the confirmed reset only after the button is released.
    if (!pressed && resetButtonPressed)
    {
        resetButtonPressed = false;

        if (resetTriggered)
        {
            Serial.println("Reset button released");
            Serial.println("Restarting...");

            // Allow a later button press to start a new reset sequence.
            resetTriggered = false;

            ESP.restart();
        }
        else
        {
            Serial.println("Reset cancelled");
        }
    }
}
