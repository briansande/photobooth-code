# PhotoboothShutter

Reusable positional-servo shutter for Arduino ESP32 (the pinned PlatformIO
espressif32 6.13.0 / Arduino 2.x LEDC API). Reserves LEDC channel 0 by default.
Call begin(pin), then shoot(openAngle, closedAngle, durationMs), move(open, angle),
or status(). Check begin's result before using the other methods. Objects must
live for the application and begin must be called only once.

No pulses are sent until an explicit move/shoot. Angles are nominal 0..180,
mapped to 1000..2000 us at 50 Hz. Shots accept 1..60000 ms. Overlapping shoots
return false; move cancels an active shot. Timer-start failure closes immediately.

An esp_timer task callback performs automatic closure independently of the web
loop. A mutex serializes short PWM/state updates; network and serial work never
hold it. A deadline check prevents stale callbacks from closing a later shot.
See Espressif's [timer documentation](https://docs.espressif.com/projects/esp-idf/en/v4.4.1/esp32c3/api-reference/system/esp_timer.html).
This is command timing, not measured mechanical exposure timing.

See [hardware notes](../../hardware/shutter.md) for wiring, pulse range and limits,
and [shutter-test](../../firmware/shutter-test/README.md) for the web application.
