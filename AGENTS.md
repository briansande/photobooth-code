# Repository instructions

This repository contains a solo developer's photobooth code.

## Branch policy

- Work only on the `main` branch.
- Do not create or use feature branches, bug-fix branches, or separate worktrees.
- Before making changes, verify that the current branch is `main`. If it is not, switch to `main` without discarding existing work. If switching would risk losing work, ask the user how to proceed.

## Commit policy

- Always commit completed bug fixes and feature updates on `main` before finishing the task.
- Run checks appropriate to the change before committing, and report any checks that could not be completed.
- Use a clear commit message describing the fix or feature.
- Stage only files relevant to the task. Do not include unrelated user changes or secrets.
- If a commit cannot be created, explain the blocker explicitly rather than silently leaving the work uncommitted.

## ESP32-C3 SuperMini workflow

The development boards are SuperMini ESP32-C3 boards with hardware marking
HW-466AB. This repository can contain multiple boards and independent firmware
projects. Every directory under `firmware/` that contains `platformio.ini` is a
separate PlatformIO project. Do not assume that two connected boards run the
same firmware or use the same COM port.

The known-good board tests are:

- `firmware/board-tests/esp32-c3-blink-fast/` (100 ms on, 100 ms off)
- `firmware/board-tests/esp32-c3-blink-slow/` (200 ms on, 200 ms off)
- `firmware/web-blink-test/` (browser-configurable GPIO8 timing)

Component-specific prototypes belong in `firmware/<component>-test/`. The
eventual integrated application belongs in `firmware/photobooth-controller/`.
Put reusable component logic in `libraries/` and wiring notes in `hardware/`.
Use `libraries/PhotoboothWiFi/` for Wi-Fi in any project that needs it: add
`lib_extra_dirs = ../../libraries` to a direct child of `firmware/` (or
`../../../libraries` beneath `firmware/board-tests/`), include
`<PhotoboothWiFi.h>`, call `PhotoboothWiFi::begin(...)` in setup and
`PhotoboothWiFi::loop()` in loop. See the library README for the short example.

### Hardware and project settings

- The microcontroller is an ESP32-C3 with 4 MB flash and native USB
  Serial/JTAG.
- Use PlatformIO board `esp32-c3-devkitm-1` with the Arduino framework.
- Use DIO flash mode, 115200 upload/monitor speed, and enable native USB with
  `ARDUINO_USB_MODE=1` and `ARDUINO_USB_CDC_ON_BOOT=1`. These settings are
  already recorded in each current project's `platformio.ini`.
- The onboard blue LED is on GPIO8 and is active-low: write `LOW` to turn it
  on and `HIGH` to turn it off. The red LED is the power indicator.

### Build, upload, and monitor

Run commands from the repository root in PowerShell. The preferred entry point
is `tools/flash.ps1`, which validates the project, finds a connected Espressif
USB device, builds the selected project, and uploads it:

```powershell
.\tools\flash.ps1 firmware/board-tests/esp32-c3-blink-fast
.\tools\flash.ps1 firmware/board-tests/esp32-c3-blink-slow
```

If local PowerShell policy blocks direct script execution, use:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\tools\flash.ps1 firmware/board-tests/esp32-c3-blink-fast
```

The helper automatically selects the board only when exactly one device with
USB ID `VID:PID=303A:1001` is present. If multiple boards are connected, first
identify them and pass the intended port explicitly:

```powershell
$pio = "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe"
& $pio device list
.\tools\flash.ps1 firmware/shutter-test -Port COM5
```

Do not assume the board will always be COM5. Do not ask the user for a port when
exactly one matching board can be detected. A user request such as "load the
fast blink firmware" authorizes building and uploading that named firmware to
the single connected board. Uploading intentionally replaces the firmware that
was previously on that board.

The two board tests identify themselves over USB serial at 115200 baud:

| Firmware | LED timing | Expected serial line | Message period |
| --- | --- | --- | --- |
| `esp32-c3-blink-fast` | 100 ms on, 100 ms off | `FAST blink test: N` | About 200 ms |
| `esp32-c3-blink-slow` | 200 ms on, 200 ms off | `SLOW blink test: N` | About 400 ms |

The web blink test uses the shared Wi-Fi library and can join the user's 2.4 GHz
home Wi-Fi in station mode, like the [older Shutter-Test project](https://github.com/briansande/Shutter-Test/tree/main).
The single Git-ignored `libraries/PhotoboothWiFi/src/wifi_secrets.h` is present
locally with empty values. Ask the user to fill in `kWifiSsid` and
`kWifiPassword`; on another checkout, copy the example header beside it.
Never commit or print those values. Build and upload after the private file is
filled in. Read the router-assigned URL (often `http://10.0.0.x/`) from the
`Open:` serial line; the browser device must be on the same network. If the
private file is missing or connection times out, firmware instead creates
`ESP32-Blink-Test` (password `blinktest`) at `http://192.168.4.1/`. Do not
mistake that fallback address for the router-assigned one. The shared library
keeps retrying the router while the fallback AP is active, and logs a new
`Wi-Fi connected. Open:` URL if it joins later. The firmware emits
`WEB BLINK TEST running` with its current URL every five seconds. Browser timing
changes are reported as `Blink timing updated: on=N ms, off=N ms`.

An upload is fully verified only after PlatformIO reports success and several
consecutive serial messages contain the expected firmware label and timing.
Sample the output and then close the serial connection; an open monitor holds
the COM port and prevents the next upload. Ask the user to confirm the physical
LED or peripheral behavior when it cannot be observed in software.

For direct PlatformIO use, the executable is
`$env:USERPROFILE\.platformio\penv\Scripts\pio.exe`. The equivalent upload
command is:

```powershell
$pio = "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe"
& $pio run --project-dir firmware/board-tests/esp32-c3-blink-fast --target upload --upload-port COM5
```

If the task requests new board behavior, update the source and documentation,
build, upload, verify, and commit the relevant files. An upload-only request
does not change tracked files and does not need a new commit. Keep generated
`.pio/` output untracked. Do not record which firmware happens to be loaded as
permanent repository state; it changes whenever a board is reflashed.

Do not assign actuator pins or energize motors, relays, or solenoids in scaffold
projects until their wiring and safe startup behavior are documented. Keep
component logic non-blocking where practical so it can later be reused by the
integrated controller.

If upload cannot connect, hold **BOOT**, tap and release **RESET**, release
**BOOT**, and retry the upload. Tap **RESET** afterward if the program does not
start. If the native USB port temporarily disappears after reset, wait briefly
and run `device list` again. If no COM port appears, check that the USB-C cable
supports data.

For Arduino IDE use `ESP32C3 Dev Module`, enable `USB CDC On Boot`, select 4 MB
flash and DIO flash mode, and use the detected COM port.
