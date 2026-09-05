#include <Arduino.h>
#include "WiFiManager.h"

WiFiManager wifi;

void setup()
{
    Serial.begin(115200);
    wifi.begin();
}

void loop()
{




    
    // DO NOT REMOVE: required for WiFiManager state machine
    wifi.loop();
}