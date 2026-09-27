# Servo shutter wiring

Board: SuperMini ESP32-C3, hardware marking HW-466AB. The user identifies the
servo signal connection as pin 1, interpreted here as **GPIO1**, not connector
position 1. This prototype assumes a positional RC servo, not continuous rotation.

| Connection | Wiring |
| --- | --- |
| Servo signal | GPIO1, 3.3 V logic, 50 Hz active-high servo pulses |
| Servo power | Regulated supply at the servo manufacturer's rated voltage |
| Servo ground | Supply ground and ESP32 GND joined together |

The exact servo model, rated voltage, stall current, and linkage limits have not
been supplied. Do not power the servo from GPIO1 or the board's 3.3 V pin. Use a
supply sized for the servo's stall current; signal compatibility with 3.3 V must
match the servo specification. No separate driver is used for a compatible RC
servo signal input. Do not connect external servo supply voltage to GPIO1.

## Startup and calibration

GPIO1 is held low with PWM duty zero after setup; no position is commanded until
Open, Close, or Shoot is pressed. During reset, GPIO can be high impedance; a
10 kOhm signal-to-ground pull-down can keep the signal low during reset. Firmware
cannot guarantee a servo's power-on behavior or mechanically close during loss
of power/reset. No pulse also means no commanded holding torque at startup.

Defaults are closed=30 and open=85, based on the user's shutter calibration;
they are not measured shaft positions. Begin with the linkage free to move
and adjust in small steps. The nominal 0..180 input maps to 500..2500 us pulses
at 50 Hz (14-bit LEDC channel 0), a common full-travel RC servo range. The earlier
1000..2000 us mapping produced about half the expected travel on the connected
servo, so the range was widened at the user's request. Actual angle and safe
endpoints depend on the servo and linkage. Check for hard stops and reduce the
range for a servo that cannot accept these pulses. GPIO8 is not used by this prototype.

Timed shots allow the 1/1000..30s dial speeds and return to the captured closed angle. Manual Open
holds indefinitely for calibration; Close cancels the timer. Pulses continue to
hold the last commanded position after closure. Servo travel is not sensed;
open time measures between commands and includes movement time. Test physical
closure before relying on exposure timing. There are no limit switches or
position sensors configured.
