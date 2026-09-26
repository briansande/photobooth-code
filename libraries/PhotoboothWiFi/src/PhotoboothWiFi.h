#pragma once

#include <Arduino.h>

namespace PhotoboothWiFi {

// Joins the Wi-Fi network in wifi_secrets.h. If no credentials are configured
// or the connection times out, starts the optional fallback access point.
bool begin(const char *fallbackSsid = nullptr,
           const char *fallbackPassword = nullptr);

// Call regularly from loop() to reconnect if the router connection drops.
void loop();

// Current page address for a web server listening on port 80.
String pageUrl();

}  // namespace PhotoboothWiFi
