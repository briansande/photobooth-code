# Shutter test

Browser-controlled positional servo on GPIO1 of the HW-466AB ESP32-C3 SuperMini.
See [wiring and startup behavior](../../hardware/shutter.md) before operating.

## Controls

- **Shutter open time (seconds):** enter a decimal from 0.001 to 30 seconds,
  with up to six digits after the decimal point (default 1 second). For example,
  `0.5`, `1.25`, and `3`. The firmware converts the value to microseconds.
- **Closed servo angle:** nominal 0 through 180 degrees (default 30).
- **Open servo angle:** nominal 0 through 180 degrees (default 85).
- **Shoot:** commands the open angle, starts the timer, then commands the closed
  angle. The shot captures both angles and the entered time when pressed.
- **Open:** immediately commands the open angle and holds it until Close or Shoot.
- **Close:** immediately commands the closed angle, cancelling any active shot.

During a shot, Shoot is disabled/rejected; Open and Close remain available and cancel the timer.
The test buttons validate only their own angle, so invalid timing cannot block
Close. No servo pulses are generated on startup until a button is pressed.
Settings remain in RAM and reset on reboot. Page reloads show the most recently
used settings. Position status is the commanded position, not sensor feedback.
The page checks status once a second. If a request has no response within four
seconds, it shows a connection error and retries status automatically. The
controls become available again when the board responds; after a lost Shoot
response, check the reported position because the command might have run.

Timing starts when the open command is issued, not when the servo physically
arrives. Servo travel and the 20 ms PWM period limit useful exposure precision;
very short times may close before the servo reaches the open angle. The input
sets command timing, not measured optical exposure. An ESP timer closes independently
of the HTTP loop, including if the browser disconnects or a request stalls.

## Build and upload

From the repository root:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run --project-dir firmware/shutter-test
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\tools\flash.ps1 firmware/shutter-test
```

Uses the shared [PhotoboothWiFi](../../libraries/PhotoboothWiFi/README.md)
credentials. Read the router-assigned URL from USB serial at 115200 baud and open
it on the same network. If router Wi-Fi is unavailable, join `ESP32-Shutter-Test`
(password `shuttertest`) and open `http://192.168.4.1/`. The library keeps retrying
the router. The page is intended for a trusted local network.

Serial emits `SHUTTER TEST running` every five seconds with GPIO1, time,
angles, shot state, Wi-Fi RSSI, free heap and URL. Each accepted action and
completed shot is logged. Close the serial monitor after sampling so the next
upload can open the port.

## Verification

Check initial idle state, both manual positions, automatic closure, rejection of
overlapping shots, cancellation by Open and Close, and invalid/missing/out-of-range input.
Confirm actual direction, travel and mechanical closure on the physical shutter.
The HTTP API is GET `/status` and POST `/action`, using form fields `action`
(`shoot`, `open`, `close`), `open_seconds` (decimal, 0.001..30), `closed_angle`, and `open_angle`.

## If the page loses contact

Check the latest `SHUTTER TEST running` line over USB serial. When its URL is
`http://192.168.4.1/`, the router join failed: connect the browser device to
`ESP32-Shutter-Test` first. When it shows a router address, the browser device
must be able to reach the ESP32 on the local network; different Wi-Fi networks
or client isolation can prevent that even when the ESP32 reports a good signal.
Repeated `AUTH_EXPIRE` or `AUTH_FAIL` messages indicate a router authentication
problem rather than a slow webpage. Check the configured 2.4 GHz network and
router security settings without putting credentials in serial logs or Git.
If failures correlate with servo movement, also check the servo's supply and
shared ground as described in the hardware notes.
