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
bool eventLoggerRegistered = false;

}  // namespace

bool begin(const char *fallbackSsid, const char *fallbackPassword) {
#if PHOTOBOOTH_HAS_WIFI_CREDENTIALS
  if (kWifiSsid[0] != '\0') {
    if (!eventLoggerRegistered) {
      WiFi.onEvent([](arduino_event_id_t event, arduino_event_info_t info) {
        const auto reason = static_cast<wifi_err_reason_t>(
            info.wifi_sta_disconnected.reason);
        Serial.printf("Wi-Fi disconnected: %s (%u)\n",
                      WiFi.disconnectReasonName(reason),
                      static_cast<unsigned int>(reason));
      }, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
      eventLoggerRegistered = true;
    }

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

    const int connectionStatus = static_cast<int>(WiFi.status());
    Serial.printf("Wi-Fi join timed out (status=%d)\n", connectionStatus);
    WiFi.disconnect();

    const int networkCount = WiFi.scanNetworks();
    bool networkSeen = false;
    for (int index = 0; index < networkCount; ++index) {
      if (WiFi.SSID(index) == kWifiSsid) {
        networkSeen = true;
        Serial.printf("Configured network visible, signal=%d dBm\n",
                      WiFi.RSSI(index));
        break;
      }
    }
    WiFi.scanDelete();
    if (!networkSeen) {
      Serial.println("Configured network not found in 2.4 GHz scan");
    }
    Serial.println("Starting fallback access point");
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
