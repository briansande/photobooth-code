#include "PhotoboothWiFi.h"

#include <WiFi.h>

#if __has_include("wifi_secrets.h")
#include "wifi_secrets.h"
#define PHOTOBOOTH_HAS_WIFI_CREDENTIALS 1
#else
#define PHOTOBOOTH_HAS_WIFI_CREDENTIALS 0
#endif

namespace PhotoboothWiFi {
namespace {

constexpr uint32_t kConnectTimeoutMs = 15000;
constexpr uint32_t kReconnectIntervalMs = 10000;
uint32_t lastReconnectAttemptMs = 0;

}  // namespace

bool begin(const char *fallbackSsid, const char *fallbackPassword) {
#if PHOTOBOOTH_HAS_WIFI_CREDENTIALS
  if (kWifiSsid[0] != '\0') {
    WiFi.mode(WIFI_STA);
    WiFi.begin(kWifiSsid, kWifiPassword);
    Serial.printf("Joining Wi-Fi: %s\n", kWifiSsid);

    const uint32_t startedAt = millis();
    while (WiFi.status() != WL_CONNECTED &&
           millis() - startedAt < kConnectTimeoutMs) {
      delay(250);
    }

    if (WiFi.status() == WL_CONNECTED) {
      lastReconnectAttemptMs = millis();
      Serial.printf("Open: %s\n", pageUrl().c_str());
      return true;
    }

    Serial.println("Wi-Fi join timed out; starting fallback access point");
    WiFi.disconnect();
  }
#endif

  if (fallbackSsid == nullptr || fallbackSsid[0] == '\0') {
    Serial.println("ERROR: no Wi-Fi credentials or fallback access point");
    return false;
  }

  WiFi.mode(WIFI_AP);
  if (!WiFi.softAP(fallbackSsid, fallbackPassword)) {
    Serial.println("ERROR: Wi-Fi access point failed to start");
    return false;
  }

  Serial.printf("Wi-Fi: %s\n", fallbackSsid);
  Serial.printf("Open: %s\n", pageUrl().c_str());
  return true;
}

void loop() {
#if PHOTOBOOTH_HAS_WIFI_CREDENTIALS
  if (WiFi.getMode() == WIFI_STA && WiFi.status() != WL_CONNECTED &&
      millis() - lastReconnectAttemptMs >= kReconnectIntervalMs) {
    Serial.println("Wi-Fi disconnected; reconnecting");
    WiFi.reconnect();
    lastReconnectAttemptMs = millis();
  }
#endif
}

String pageUrl() {
  const IPAddress ip = WiFi.getMode() == WIFI_STA ? WiFi.localIP()
                                                : WiFi.softAPIP();
  return "http://" + ip.toString() + "/";
}

}  // namespace PhotoboothWiFi
