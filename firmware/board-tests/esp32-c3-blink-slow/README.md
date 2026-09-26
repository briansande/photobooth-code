# ESP32-C3 SuperMini slow blink

Half-speed companion to the fast blink firmware. The onboard blue GPIO8 LED is
on for 200 ms and off for 200 ms: 2.5 complete blinks per second. USB serial
prints `SLOW blink test: N` at 115200 baud after each blink.

From the repository root:

```powershell
.\tools\flash.ps1 firmware/board-tests/esp32-c3-blink-slow
```

Switch between the fast and slow projects to confirm that distinct firmware
images can be built and uploaded. No external wiring or libraries are required.
