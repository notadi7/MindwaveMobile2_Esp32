# Brain Control Motor System — file guide

This is a normal **Arduino sketch folder with multiple tabs**, not separate
programs. Open `BrainControlMotorSystem.ino` in the Arduino IDE (double-click
it, or use "Open" and pick the folder) and every other `.ino` file in this
folder will load automatically as its own tab along the top of the editor.
When you compile, the IDE stitches all the tabs back into one program — you
don't need to `#include` them or do anything special.

## What's in each file

| File | What it does |
|---|---|
| `BrainControlMotorSystem.ino` | **Start here.** Pins, WiFi/SMTP config, all global variables/objects, `setup()`, `loop()`. |
| `NeuroSky_Parser.ino` | Reads raw bytes off Bluetooth and decodes a MindWave packet into `attention`, `meditation`, brainwave values. Only used when `DEMO_MODE` is `0`. |
| `Demo_Mode.ino` | Fakes sensor data so you can test everything without a headset. Only used when `DEMO_MODE` is `1`. |
| `Telemetry.ino` | Runs once a second: relay/motor control, session countdown, focus-score tracking, builds the WebSocket JSON, refreshes the OLED. |
| `Session_Manager.ino` | The session state machine — starting a session, ending it when the timer hits 0, resetting for a new one. |
| `Email_Sender.ino` | Sends the result email over SMTP. |
| `Setup_Page.ino` | The `/` route (routes to setup, dashboard, or result page depending on session state) + the credential-capture (name/email/duration) page. |
| `Dashboard_Page.ino` | The gauges/trend/LED dashboard + countdown + live focus score. Redirects to `/` when the session ends (see `Result_Page.ino`). |
| `Result_Page.ino` | The standalone "session complete" result page — name/email/session-time cards, performance summary (total time, avg/peak attention, focus score), brainwave breakdown, and the email/new-session buttons. Served at `/` whenever the state machine is in `STATE_RESULT`. |
| `WebSocket_Handler.ino` | Logs WebSocket connect/disconnect events. |
| `OLED_Display.ino` | Small helpers for writing text to the screen. |
| `WiFi_Connect.ino` | Connects to WiFi once at boot. |

## Why config/pins/WiFi/SMTP live in the main file

Arduino always compiles the file with the same name as the folder **first**,
then the other tabs alphabetically, all glued into one file behind the
scenes. That means anything the main file's `setup()`/`loop()` needs (pin
numbers, the WiFi/SMTP `#define`s, global variables) has to be defined in
the main file itself, or it wouldn't exist yet at the point it's used. That's
just how multi-tab Arduino sketches work — it's not a limitation of this
particular project.

## Before you flash

Open `BrainControlMotorSystem.ino` and check near the top:

- `ssid` / `password` — your WiFi
- `SMTP_USER` / `SMTP_PASS` — your Gmail address + a 16-character **App
  Password** (not your normal Gmail password — generate one under Google
  Account → Security → 2-Step Verification → App passwords)
- `#define DEMO_MODE 1` — leave as `1` to test without a headset, set to
  `0` once your MindWave is ready to pair

## Libraries you need installed (Library Manager)

- ESP32 board package (gives you `WiFi.h`, `BluetoothSerial.h`)
- `WebServer` (bundled with the ESP32 core)
- `WebSockets` by Markus Sattler
- `Adafruit GFX Library` + `Adafruit SH110X`
- `ESP Mail Client` by Mobizt
