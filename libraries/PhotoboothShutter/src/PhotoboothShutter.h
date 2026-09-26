#pragma once
#include <Arduino.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// Reserve one LEDC channel per instance. Instances live for the application.
class PhotoboothShutter {
 public:
  enum class Position { Idle, Open, Closed };
  struct Status {
    Position position;
    bool shooting;
    uint16_t angle;
    uint32_t remainingMs;
    uint32_t completedShots;
  };
  bool begin(uint8_t pin, uint8_t channel = 0);
  bool shoot(uint16_t openAngle, uint16_t closedAngle, uint32_t durationMs);
  void move(bool open, uint16_t angle);
  Status status();
 private:
  static void onTimeout(void *argument);
  void writeAngle(uint16_t angle);
  SemaphoreHandle_t mutex_ = nullptr;
  esp_timer_handle_t timer_ = nullptr;
  uint8_t channel_ = 0;
  Position position_ = Position::Idle;
  bool shooting_ = false;
  uint16_t angle_ = 90;
  uint16_t closedAngle_ = 90;
  int64_t deadlineUs_ = 0;
  uint32_t completedShots_ = 0;
};
