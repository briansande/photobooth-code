#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  delay(250);
  Serial.println("PAPER FEED TEST scaffold: no hardware outputs configured");
}

void loop() {
  delay(1000);
}
