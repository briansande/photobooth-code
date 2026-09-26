#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>

#include <cstdlib>

namespace {

constexpr uint8_t kLedPin = 8;
constexpr char kAccessPointName[] = "ESP32-Blink-Test";
constexpr char kAccessPointPassword[] = "blinktest";
constexpr uint32_t kMinimumDurationMs = 10;
constexpr uint32_t kMaximumDurationMs = 60000;

WebServer server(80);
uint32_t onDurationMs = 100;
uint32_t offDurationMs = 100;
uint32_t lastTransitionMs = 0;
uint32_t lastStatusMs = 0;
bool ledIsOn = false;

void setLed(bool turnOn) {
  ledIsOn = turnOn;
  digitalWrite(kLedPin, turnOn ? LOW : HIGH);
}

bool parseDuration(const String &text, uint32_t &duration) {
  if (text.isEmpty()) {
    return false;
  }

  char *end = nullptr;
  const unsigned long parsed = strtoul(text.c_str(), &end, 10);
  if (end == text.c_str() || *end != '\0' || parsed < kMinimumDurationMs ||
      parsed > kMaximumDurationMs) {
    return false;
  }

  duration = static_cast<uint32_t>(parsed);
  return true;
}

String makeControlPage() {
  String page = F(R"HTML(<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP32 Blink Control</title>
  <style>
    :root { color-scheme: light dark; font-family: system-ui, sans-serif; }
    body { margin: 0; min-height: 100vh; display: grid; place-items: center; background: #17202a; }
    main { box-sizing: border-box; width: min(92vw, 420px); padding: 28px; border-radius: 18px; background: #243447; box-shadow: 0 16px 50px #0006; }
    h1 { margin: 0 0 8px; font-size: 1.6rem; }
    p { margin: 0 0 22px; color: #c9d6e2; line-height: 1.45; }
    label { display: block; margin: 16px 0; font-weight: 650; }
    input { box-sizing: border-box; width: 100%; margin-top: 7px; padding: 12px; border: 1px solid #698096; border-radius: 9px; font: inherit; background: #152331; }
    button { width: 100%; margin-top: 10px; padding: 13px; border: 0; border-radius: 9px; background: #38bdf8; color: #062435; font: inherit; font-weight: 750; cursor: pointer; }
    .status { padding: 12px; border-radius: 9px; background: #192a39; color: #d8e8f5; }
  </style>
</head>
<body>
  <main>
    <h1>ESP32 Blink Control</h1>
    <p>Set how long the onboard blue LED stays on and off.</p>
    <div class="status">Current: {{ON_MS}} ms on / {{OFF_MS}} ms off</div>
    <form method="post" action="/settings">
      <label>LED on time (ms)
        <input name="on_ms" type="number" min="10" max="60000" value="{{ON_MS}}" required>
      </label>
      <label>LED off time (ms)
        <input name="off_ms" type="number" min="10" max="60000" value="{{OFF_MS}}" required>
      </label>
      <button type="submit">Apply</button>
    </form>
  </main>
</body>
</html>)HTML");

  page.replace("{{ON_MS}}", String(onDurationMs));
  page.replace("{{OFF_MS}}", String(offDurationMs));
  return page;
}

void handleRoot() {
  server.send(200, "text/html", makeControlPage());
}

void handleSettings() {
  uint32_t requestedOnMs = 0;
  uint32_t requestedOffMs = 0;

  if (!server.hasArg("on_ms") || !server.hasArg("off_ms") ||
      !parseDuration(server.arg("on_ms"), requestedOnMs) ||
      !parseDuration(server.arg("off_ms"), requestedOffMs)) {
    server.send(400, "text/plain",
                "Both durations must be whole milliseconds from 10 to 60000.");
    return;
  }

  onDurationMs = requestedOnMs;
  offDurationMs = requestedOffMs;
  setLed(true);
  lastTransitionMs = millis();
  lastStatusMs = millis();

  Serial.printf("Blink timing updated: on=%lu ms, off=%lu ms\n",
                static_cast<unsigned long>(onDurationMs),
                static_cast<unsigned long>(offDurationMs));

  server.sendHeader("Location", "/");
  server.send(303);
}

}  // namespace

void setup() {
  digitalWrite(kLedPin, HIGH);
  pinMode(kLedPin, OUTPUT);

  Serial.begin(115200);
  delay(250);

  WiFi.mode(WIFI_AP);
  if (!WiFi.softAP(kAccessPointName, kAccessPointPassword)) {
    Serial.println("ERROR: Wi-Fi access point failed to start");
    return;
  }

  server.on("/", HTTP_GET, handleRoot);
  server.on("/settings", HTTP_POST, handleSettings);
  server.onNotFound([]() { server.send(404, "text/plain", "Not found"); });
  server.begin();

  setLed(true);
  lastTransitionMs = millis();

  Serial.println("WEB BLINK TEST ready");
  Serial.printf("Wi-Fi: %s\n", kAccessPointName);
  Serial.printf("Open: http://%s/\n", WiFi.softAPIP().toString().c_str());
  Serial.printf("Default timing: on=%lu ms, off=%lu ms\n",
                static_cast<unsigned long>(onDurationMs),
                static_cast<unsigned long>(offDurationMs));
}

void loop() {
  server.handleClient();

  const uint32_t now = millis();
  const uint32_t currentDurationMs = ledIsOn ? onDurationMs : offDurationMs;
  if (now - lastTransitionMs >= currentDurationMs) {
    setLed(!ledIsOn);
    lastTransitionMs = now;
  }

  if (now - lastStatusMs >= 5000) {
    Serial.printf("WEB BLINK TEST running: on=%lu ms, off=%lu ms, clients=%u\n",
                  static_cast<unsigned long>(onDurationMs),
                  static_cast<unsigned long>(offDurationMs),
                  WiFi.softAPgetStationNum());
    lastStatusMs = now;
  }
}
