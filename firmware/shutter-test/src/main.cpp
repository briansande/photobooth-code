#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  delay(250);
  Serial.println("SHUTTER TEST scaffold: no hardware outputs configured");
}

void loop() {
  delay(1000);
}
