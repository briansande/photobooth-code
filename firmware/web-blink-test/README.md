# Browser-controlled blink test

This firmware turns the ESP32-C3 SuperMini into a small Wi-Fi access point and
web server. The page controls the onboard blue GPIO8 LED's on and off durations
without recompiling or uploading again.

## Upload

From the repository root:

```powershell
.\tools\flash.ps1 firmware/web-blink-test
```

## Open the controls

1. Connect a phone or computer to Wi-Fi network **ESP32-Blink-Test**.
2. Enter password **blinktest**.
3. If the device warns that the network has no internet, choose to stay
   connected.
4. Open **http://192.168.4.1/** in a browser.
5. Enter LED-on and LED-off durations in milliseconds and select **Apply**.

Accepted values are 10 through 60,000 ms. Changes take effect immediately but
reset to the 100 ms defaults whenever the ESP32 restarts. The ESP32 does not
join a home or office network, and the firmware stores no network credentials.

USB serial at 115200 baud reports the access-point address and every accepted
timing change. It also prints a `WEB BLINK TEST running` status every five
seconds so the active firmware can be identified without resetting the board.
