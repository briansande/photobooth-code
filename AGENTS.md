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

The connected development board is a SuperMini ESP32-C3, hardware marking
HW-466AB. The known-good PlatformIO example is in `esp32-c3-blink/`.

### Hardware and project settings

- The microcontroller is an ESP32-C3 with 4 MB flash and native USB
  Serial/JTAG.
- Use PlatformIO board `esp32-c3-devkitm-1` with the Arduino framework.
- Use DIO flash mode, 115200 upload/monitor speed, and enable native USB with
  `ARDUINO_USB_MODE=1` and `ARDUINO_USB_CDC_ON_BOOT=1`. These settings are
  already recorded in `esp32-c3-blink/platformio.ini`.
- The onboard blue LED is on GPIO8 and is active-low: write `LOW` to turn it
  on and `HIGH` to turn it off. The red LED is the power indicator.

### Build, upload, and monitor

Run commands from the repository root in PowerShell. PlatformIO is installed
in the user's local PlatformIO environment:

```powershell
$pio = "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe"
& $pio run --project-dir esp32-c3-blink
& $pio device list
& $pio run --project-dir esp32-c3-blink --target upload --upload-port COM5
& $pio device monitor --port COM5 --baud 115200
```

Do not assume the board will always be COM5. Run `device list` and use the port
whose hardware ID is the Espressif USB device (previously `VID:PID=303A:1001`).
Close any serial monitor or process holding the port before uploading. A
successful upload ends with the image hash verified and a hard reset. When the
firmware has serial logging, confirm several consecutive messages at 115200
baud after upload; also ask the user to confirm physical LED or peripheral
behavior when it cannot be observed in software.

Uploading replaces the firmware currently on the board. If the task requests
new board behavior, update the source and documentation, build, upload, verify,
and commit the relevant files. Keep generated `.pio/` output untracked.

If upload cannot connect, hold **BOOT**, tap and release **RESET**, release
**BOOT**, and retry the upload. Tap **RESET** afterward if the program does not
start. If no COM port appears, check that the USB-C cable supports data.

For Arduino IDE use `ESP32C3 Dev Module`, enable `USB CDC On Boot`, select 4 MB
flash and DIO flash mode, and use the detected COM port.
