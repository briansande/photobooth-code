#include <Arduino.h>

// SuperMini HW-466AB: the onboard blue LED is active-low on GPIO8.
constexpr uint8_t kLedPin = 8;
constexpr unsigned long kBlinkIntervalMs = 500;

void setup() {
  digitalWrite(kLedPin, HIGH);
  pinMode(kLedPin, OUTPUT);
  Serial.begin(115200);
  // Do not wait for a serial monitor: blink even on USB power alone.
}

void loop() {
  static unsigned long blinkCount = 0;

  digitalWrite(kLedPin, LOW);
  delay(kBlinkIntervalMs);
  digitalWrite(kLedPin, HIGH);
  delay(kBlinkIntervalMs);

  Serial.printf("SuperMini blink test: %lu\n", ++blinkCount);
}
