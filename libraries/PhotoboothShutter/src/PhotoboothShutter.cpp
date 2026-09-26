#include "PhotoboothShutter.h"

bool PhotoboothShutter::begin(uint8_t pin, uint8_t channel) {
  digitalWrite(pin, LOW);
  pinMode(pin, OUTPUT);
  channel_ = channel;
  mutex_ = xSemaphoreCreateMutex();
  if (!mutex_) return false;
  esp_timer_create_args_t arguments = {};
  arguments.callback = onTimeout;
  arguments.arg = this;
  arguments.dispatch_method = ESP_TIMER_TASK;
  arguments.name = "shutter-close";
  if (esp_timer_create(&arguments, &timer_) != ESP_OK) return false;
  if (ledcSetup(channel_, 50, 14) == 0) return false;
  ledcWrite(channel_, 0);  // No pulses until the first explicit command.
  ledcAttachPin(pin, channel_);
  return true;
}

void PhotoboothShutter::writeAngle(uint16_t angle) {
  // Nominal 0..180 maps to 1000..2000 us at 50 Hz. Calibrate actual travel.
  const uint32_t pulseUs = 1000 + (static_cast<uint32_t>(angle) * 1000 + 90) / 180;
  ledcWrite(channel_, (pulseUs * 16384UL + 10000) / 20000);
  angle_ = angle;
}

bool PhotoboothShutter::shoot(uint16_t openAngle, uint16_t closedAngle,
                             uint32_t durationMs) {
  if (openAngle > 180 || closedAngle > 180 || durationMs < 1 || durationMs > 60000) return false;
  xSemaphoreTake(mutex_, portMAX_DELAY);
  if (shooting_) { xSemaphoreGive(mutex_); return false; }
  esp_timer_stop(timer_);
  closedAngle_ = closedAngle;
  writeAngle(openAngle);
  position_ = Position::Open;
  shooting_ = true;
  deadlineUs_ = esp_timer_get_time() + static_cast<int64_t>(durationMs) * 1000;
  const bool started = esp_timer_start_once(timer_, durationMs * 1000ULL) == ESP_OK;
  if (!started) {
    writeAngle(closedAngle_);
    position_ = Position::Closed;
    shooting_ = false;
  }
  xSemaphoreGive(mutex_);
  return started;
}

void PhotoboothShutter::move(bool open, uint16_t angle) {
  if (angle > 180) return;
  xSemaphoreTake(mutex_, portMAX_DELAY);
  esp_timer_stop(timer_);
  shooting_ = false;
  writeAngle(angle);
  position_ = open ? Position::Open : Position::Closed;
  xSemaphoreGive(mutex_);
}

void PhotoboothShutter::onTimeout(void *argument) {
  auto *self = static_cast<PhotoboothShutter *>(argument);
  // Only bounded state/PWM work holds this mutex, never web or serial I/O.
  // The deadline check also rejects callbacks queued before a manual override.
  xSemaphoreTake(self->mutex_, portMAX_DELAY);
  if (self->shooting_ && esp_timer_get_time() >= self->deadlineUs_) {
    self->writeAngle(self->closedAngle_);
    self->position_ = Position::Closed;
    self->shooting_ = false;
    ++self->completedShots_;
  }
  xSemaphoreGive(self->mutex_);
}

PhotoboothShutter::Status PhotoboothShutter::status() {
  xSemaphoreTake(mutex_, portMAX_DELAY);
  const int64_t remainingUs = deadlineUs_ - esp_timer_get_time();
  Status result = {position_, shooting_, angle_,
                   shooting_ && remainingUs > 0
                       ? static_cast<uint32_t>((remainingUs + 999) / 1000) : 0,
                   completedShots_};
  xSemaphoreGive(mutex_);
  return result;
}
