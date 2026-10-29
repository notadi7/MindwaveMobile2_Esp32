# BrainControlMotorSystem — State-Machine Rewrite

This document covers the items requested in the modification brief (§43 A–L).
It describes the **architecture as it now exists** after the rewrite — read it
alongside the `.ino` files, not as a separate design that still needs applying.

---

## A. State machine explanation

Everything is driven by one enum, `SystemState`, declared in
`BrainControlMotorSystem.ino`:

```
BOOT → WIFI_CONNECTING → SENSOR_CONNECTING ⇄ NO_SIGNAL ⇄ READY
                                                              │
                                          (start-session, validated)
                                                              ▼
                                                     SESSION_STARTED
                                                     (holds ~1.8s)
                                                              ▼
                                                     ACTIVE_SESSION ──(timer=0)──► SESSION_ENDED
                                                                                        │
                                                                              (new-session)
                                                                                        ▼
                                                                            READY or NO_SIGNAL

  GPIO13 held 3s, from ANY state:  <state> → WIFI_COUNTDOWN → WIFI_PORTAL → (reboot) → BOOT
  GPIO13 released early:           WIFI_COUNTDOWN → <state it came from>

  /estop, from ANY state:          <state> → EMERGENCY_STOP → (after 1.5s) → READY or NO_SIGNAL
```

No module sets `systemState` except the one that owns that transition:

| Transition | Owner |
|---|---|
| `SENSOR_CONNECTING ⇄ NO_SIGNAL ⇄ READY` | `Sensor_State.ino::updateSensorState()` |
| `READY → SESSION_STARTED` | `Session_Manager.ino::handleStartSession()` |
| `SESSION_STARTED → ACTIVE_SESSION` | `Session_Manager.ino::updateSessionState()` |
| `ACTIVE_SESSION → SESSION_ENDED` | `Session_Manager.ino::endSession()` |
| `SESSION_ENDED → READY/NO_SIGNAL` | `Session_Manager.ino::handleNewSession()` |
| `<any> → WIFI_COUNTDOWN → WIFI_PORTAL` | `WiFi_Setup_Portal.ino::checkWifiSetupButton()` |
| `<any> → EMERGENCY_STOP → READY/NO_SIGNAL` | `Session_Manager.ino::handleEmergencyStop()` / `updateSessionState()` |

A second, independent boolean — `eegSignalValid` — tracks whether *recent,
checksum-valid* EEG packets are arriving (req. #34: "Bluetooth connected" and
"EEG valid" are different things). Critically, **losing EEG signal during
`ACTIVE_SESSION` does not change `systemState`** (req. #11) — the session
stays `ACTIVE_SESSION` internally; only the TFT/dashboard/motor react to
`eegSignalValid` going false, and the moment it returns everything resumes
without touching the session accumulators.

`loop()` runs, every single iteration, in this order: GPIO13 → WiFi portal
DNS → HTTP → WebSocket → MindWave parser → `updateSensorState()` →
`updateSessionState()` → `updateMotorControl()` → `updateTFT()` →
`updateTelemetry()`. Nothing in that list blocks for more than a few
milliseconds (see §I).

---

## B. TFT state-transition table

`TFT_Display.ino::updateTFT()` is the **only** function that calls a
`show*()`/`update*Display()` render function anywhere in the project. It maps
`systemState` (+ `eegSignalValid` for one special case) to a `TFTScreen`, and:

- redraws the **entire** screen the moment the mapped screen changes,
- gives `ACTIVE_SESSION` and `WIFI_COUNTDOWN` a bounded periodic redraw
  (`TFT_LIVE_REFRESH_MS` = 300ms) for their live numbers,
- **holds** every other screen indefinitely (READY, NO_SIGNAL,
  SESSION_ENDED, SENSOR_CONNECTING, WIFI_PORTAL, SESSION_STARTED,
  EMERGENCY_STOP) — it is never redrawn again until the state changes.

| systemState | eegSignalValid | TFT screen | Redraw behavior |
|---|---|---|---|
| BOOT | — | Boot / init | once |
| WIFI_CONNECTING | — | "CONNECTING WIFI..." | once (pre-loop only) |
| SENSOR_CONNECTING | — | "MINDWAVE / NOT CONNECTED / TRYING TO CONNECT" | once |
| NO_SIGNAL | — | "10 · NO SIGNAL / WAITING FOR EEG SIGNAL" | once, held |
| READY | — | "READY / SENSOR READY / IP: … / START SESSION FROM WEB DASHBOARD" (no attention gauge — req. #9) | once, held |
| WIFI_COUNTDOWN | — | "05 · WIFI SETUP / HOLD BUTTON / 3 2 1" | once + every 1s digit change |
| WIFI_PORTAL | — | "MindWave-Setup / IP: 192.168.4.1 / OPEN WIFI SETUP PORTAL" | once, held |
| SESSION_STARTED | — | "07 · SESSION STARTED / name / N MIN SESSION" | once, held (~1.8s) |
| ACTIVE_SESSION | **true** | "08 · ACTIVE SESSION" — attention, motor, relay, signal, timer | once + every 300ms |
| ACTIVE_SESSION | **false** | "10 · NO SIGNAL" (same screen as idle NO_SIGNAL) | once, held until valid again |
| SESSION_ENDED | — | "11 · SESSION ENDED / AVG ATTENTION / MOTOR TIME / VIEW RESULT ON DASHBOARD" | once, held — **never** overwritten |
| EMERGENCY_STOP | — | "EMERGENCY STOP / SYSTEM HALTED" | once, held (~1.5s) |

---

## C. Web dashboard state-transition table

Three page templates, served both at `GET /` (state-driven, in
`Setup_Page.ino::handleRoot()`) and at explicit, deterministic routes:

| Route | Serves |
|---|---|
| `GET /setup` | Credential-capture / start-session form |
| `GET /dashboard` | Live session gauges/trend/countdown |
| `GET /result` | Frozen session result |
| `GET /status` | Lightweight JSON (`state`, `sensorConnected`, `eegValid`, `ip`) |

`GET /` dispatches directly (no redirect) based on `systemState`:

| systemState | `/` serves |
|---|---|
| WIFI_PORTAL | WiFi credential-entry portal page |
| SESSION_STARTED, ACTIVE_SESSION | Dashboard |
| SESSION_ENDED | Result |
| everything else (BOOT, WIFI_CONNECTING, SENSOR_CONNECTING, NO_SIGNAL, READY, WIFI_COUNTDOWN, EMERGENCY_STOP) | Setup page (polls `/status`, shows a readiness banner, disables Start until `READY`) |

The **dashboard's** WebSocket handler (`updateAll()` in `Dashboard_Page.ino`)
reacts to the `state`/`eegValid` fields of each frame:

| Frame says | Dashboard does |
|---|---|
| `state: SESSION_ENDED` | navigate to `/result` (once) |
| `state` is anything other than `SESSION_STARTED`/`ACTIVE_SESSION` | navigate to `/setup` (once) — covers e-stop or a GPIO13 hold interrupting the session while this tab was open |
| `eegValid: false` (mid-session) | show "NO SIGNAL" waiting view; countdown/motor/relay keep updating from server truth so nothing freezes |
| `eegValid: true` | full live gauges/brainwave/trend update |

The **setup** page polls `GET /status` every 2s and paints a banner + gates
the Start button:

| `/status` state | Banner | Start button |
|---|---|---|
| READY | hidden | enabled |
| NO_SIGNAL | "NO SIGNAL — put the headset on…" | disabled |
| SENSOR_CONNECTING / WIFI_CONNECTING / BOOT | "MINDWAVE NOT CONNECTED — trying to connect…" | disabled |
| WIFI_COUNTDOWN / WIFI_PORTAL | "WiFi setup in progress…" | disabled |
| EMERGENCY_STOP | "EMERGENCY STOP — recovering…" | disabled |
| SESSION_STARTED / ACTIVE_SESSION | (navigates away to `/dashboard`) | — |
| SESSION_ENDED | (navigates away to `/result`) | — |

`POST /start-session`, `/new-session`, `/estop` all return **JSON**
(`{"ok":true,"state":"..."}` or `{"ok":false,"error":"..."}"}`) instead of
`303`/`302` redirects; the page's own JS does an explicit
`window.location.href = ...` only after reading `ok:true` — this is the fix
for the blank-dashboard bug (see §F).

---

## D. GPIO13 WiFi setup behavior

`WiFi_Setup_Portal.ino::checkWifiSetupButton()` runs **first**, every single
`loop()` iteration, regardless of `systemState` (it only skips work once
already inside `STATE_WIFI_PORTAL`, since that only exits via reboot):

1. Button goes LOW → `wifiCountdownPreviousState = systemState`,
   `systemState = WIFI_COUNTDOWN`, relay/motor cut immediately
   (`digitalWrite`/`analogWrite`, zero-latency, doesn't wait for
   `updateMotorControl()`'s next pass).
2. Held ≥ `WIFI_HOLD_MS` (3000ms) → `startWifiConfigPortal()`:
   `systemState = WIFI_PORTAL`, soft-AP `MindWave-Setup` starts, DNS capture
   starts. TFT shows the live `WiFi.softAPIP()` (never hardcoded).
3. Released early → `systemState = wifiCountdownPreviousState` — whatever was
   happening (including an `ACTIVE_SESSION`) resumes exactly where it left
   off; the session accumulators were never touched, so nothing is lost.
4. Session data is **preserved** through a GPIO13 excursion (unlike
   `/estop`, which clears it — see §19 of the brief). If the portal is
   actually entered and WiFi credentials are saved, the device reboots
   (`ESP.restart()`), which does end the in-memory session — this is a
   deliberate, documented limitation (no session state is persisted across
   reboot; nothing in the brief asks for that).
5. `wifiSetupPortalTick()` (DNS servicing + 5-minute timeout) runs every
   loop while `systemState == WIFI_PORTAL`, never blocking.

---

## E. Non-blocking architecture explanation

`loop()` now looks like this every iteration (`BrainControlMotorSystem.ino`):

```
checkWifiSetupButton();     // GPIO13, always
wifiSetupPortalTick();      // DNS, only while in the portal
server.handleClient();
webSocket.loop();
processMindWaveBluetooth(); // drains available bytes, never waits
updateSensorState(now);
updateSessionState(now);
updateMotorControl(now);
updateTFT(now);
updateTelemetry(now);
```

No function in that list contains a `delay()` or a `while(...)` that waits
on external state. The only `delay()` calls left anywhere in the project are:

- **Inside `setup()`**, before `loop()` ever starts (boot-screen hold, the
  one-shot `WiFi.begin()`/`SerialBT.connect()` attempts). These match the
  brief's own exception for "extremely short hardware initialization
  delays" and cannot race with anything, since nothing else is running yet.
- **`WiFi_Setup_Portal.ino`, `delay(100)`** — a one-time ~100ms
  AP-teardown/restart settle time required by the ESP32 WiFi driver,
  incurred once per portal entry (not per loop). Documented in-code.
- **`WiFi_Setup_Portal.ino`, `delay(1500)`** — immediately before
  `ESP.restart()` in `handleWifiSetupSave()`. The device is about to reboot
  anyway, so nothing is lost by this brief pause.

One honest caveat, documented in `loop()` itself: the classic ESP32
`BluetoothSerial::connect()` call used for the initial SPP pairing has **no
true async variant** in the Arduino core — it is a blocking library call by
design. The fix applied is to (a) make the **byte parser** fully
non-blocking (this was the actual, severe bug — see §I) and (b) only attempt
`connect()` again on a long timer (`BT_RECONNECT_INTERVAL_MS` = 8s) while
disconnected, rather than every loop, so the rest of the system is starved
for at most a couple of seconds once every 8 seconds in the worst case
(headset gone completely) — not on every iteration.

---

## F. Exact changes made to fix the blank dashboard

**Root cause:** `handleStartSession()` and `handleNewSession()` returned HTTP
`303` redirects to `/`, and the client did `.then(() => window.location.href
= '/')` regardless of whether the POST actually succeeded. A slow AP-mode
association or a transient `server.send()` timing issue could leave the
browser mid-navigation with nothing rendered.

**Fix:**
- `POST /start-session`, `/new-session`, `/estop` now all return
  `200 application/json` (`{"ok":true,"state":"..."}` or
  `{"ok":false,"error":"..."}"}`) — never a redirect.
- The page's JS **checks `ok`** before navigating, and navigates to an
  **explicit route** (`/dashboard`, `/setup`, `/result`) rather than `/`.
- `GET /`, `/setup`, `/dashboard`, `/result` are all registered as separate,
  deterministic `server.on(...)` routes (`setup()` in the main file) — none
  of them depend on a prior redirect having set state correctly.
- The dashboard and setup pages both actively self-correct if they're ever
  looking at a "wrong" state (§C) instead of just going blank.

---

## G. Exact changes made to fix TFT attention always showing

**Root cause:** `updateActiveSessionDisplay()` was called directly from
`telemetryTick()` any time `poorQuality < 50`, with no check on whether a
session was actually running — so the live attention gauge appeared even
during `STATE_SETUP`/idle.

**Fix:**
- `showReady()` (the new idle screen) contains **no attention gauge at
  all** (req. #9) — just "READY / SENSOR READY / IP / START SESSION FROM WEB
  DASHBOARD".
- `updateActiveSessionDisplay()` is only ever invoked by
  `TFT_Display.ino::renderTFTScreen(TFT_ACTIVE_SESSION)`, which is only
  reached when `systemState == STATE_ACTIVE_SESSION && eegSignalValid`.
- `Sensor_State.ino` zeroes `attention` the instant `eegSignalValid` goes
  false, so even a stray read of the variable elsewhere can't show a stale
  number (req. #8).

---

## H. Exact changes made to make SESSION ENDED hold

**Root cause:** every module was free to call a `show*()` function directly
(`telemetryTick()` → `showNoSignal()`, `endSession()` → `showSessionEnded()`,
etc.), so a subsequent EEG-driven telemetry tick could silently redraw over
the result screen.

**Fix (req. #24, the core architectural fix):**
- `TFT_Display.ino::updateTFT()` is now the **only** function in the entire
  project that calls a render function. Every other module (`Session_Manager`,
  `Sensor_State`, `Motor_Control`, `Telemetry`, `Email_Sender`,
  `WiFi_Setup_Portal`) only ever changes `systemState`/data.
- `updateTFT()`'s `default:` branch (covering `SESSION_ENDED` and every
  other "hold" screen) does **nothing** once the screen has already been
  drawn for the current state — it only redraws on an actual state change.
  Since nothing else can change `systemState` away from `SESSION_ENDED`
  except `handleNewSession()`/`/estop`, the screen is provably stable until
  the user acts.
- `endSession()` no longer calls `showSessionEnded()` at all — it just sets
  `systemState = STATE_SESSION_ENDED` and freezes the result globals.

---

## I. Exact changes made to prevent stale EEG data

**Root cause 1 (the blocking parser):** `ReadOneByte()` was
`while (!SerialBT.available());` — a genuine infinite-ish stall that could
block `loop()` for however long the next Bluetooth byte took, starving
WebServer/WebSocket/GPIO13/TFT/session timing. Replaced with the
`processMindWaveBluetooth()` / `processMindWaveByte()` byte-machine in
`NeuroSky_Parser.ino` (states: `NS_WAIT_SYNC_1 → NS_WAIT_SYNC_2 →
NS_READ_LENGTH → NS_READ_PAYLOAD → NS_READ_CHECKSUM`), which only consumes
bytes that are already available and returns immediately otherwise.

**Root cause 2 (staleness):** `attention`/brainwave globals were simply left
at their last value once packets stopped arriving; nothing zeroed them or
tracked "how long ago was the last good packet".

**Fix:**
- `lastValidEEGPacketMillis` is stamped, unconditionally, every time a
  checksum-valid packet is decoded (`parseNeuroSkyPayload()`).
- `Sensor_State.ino::updateSensorState()` computes
  `eegSignalValid = mindwaveConnected && (now - lastValidEEGPacketMillis <=
  EEG_TIMEOUT_MS) && (poorQuality < POOR_QUALITY_THRESHOLD)` every loop
  (`EEG_TIMEOUT_MS` = 2500ms, configurable), and **zeroes `attention`** the
  moment it goes false.
- `Telemetry.ino::broadcastTelemetry()` sends `attention: 0` /
  `meditation: 0` and zeroed brainwave percentages whenever `!eegSignalValid`
  — the dashboard never receives a stale number either. (This also fixes a
  latent divide-by-zero in the original brainwave-percentage math when no
  wave packets had arrived yet.)
- `Motor_Control.ino` forces the relay/motor off the instant
  `eegSignalValid` is false, regardless of what `systemState` says.

---

## J. Exact changes made to prevent WebServer/WebSocket blocking

- The parser fix in §I is the big one — it was the only genuinely
  loop()-blocking call in the runtime path.
- `Session_Manager.ino`'s countdown no longer decrements inside
  `telemetryTick()`; it's `remaining = duration - (now - startMillis)`,
  recomputed fresh every call — so a slow WebSocket client or a burst of
  HTTP requests can never make the timer itself lag or drift (req. #12/#13).
- `Motor_Control.ino` and `Telemetry.ino` are now separate, independently
  timed functions (`updateMotorControl()` every loop, `updateTelemetry()`
  gated to `TELEMETRY_INTERVAL_MS` = 250ms) instead of one 1-second,
  do-everything `telemetryTick()` — so a heavy telemetry broadcast can never
  delay motor safety logic, and vice versa.
- `checkWifiSetupButton()` and `wifiSetupPortalTick()` run before
  `server.handleClient()`/`webSocket.loop()` in `loop()`'s ordering, so
  GPIO13 is always read even if HTTP/WS work is momentarily heavier that
  iteration.

---

## K. Real hardware test procedure

Set `#define DEMO_MODE 0` in `BrainControlMotorSystem.ino`, then run the 15
acceptance tests from the brief in order:

1. Power on with the MindWave **off** → TFT shows "MINDWAVE / NOT CONNECTED
   / TRYING TO CONNECT" (no attention, no READY).
2. Power the headset on but don't wear it (or wear it loosely) → TFT shows
   "10 · NO SIGNAL / WAITING FOR EEG SIGNAL" once `mindwaveConnected` is
   true but no valid packets arrive within `EEG_TIMEOUT_MS`.
3. Put it on properly → TFT auto-transitions to "READY", showing the real
   station IP.
4. Open `http://<ip>/` from a phone/laptop on the same network → setup page
   loads, Start button enabled once the banner shows nothing (i.e. state is
   READY).
5. Start a 1-minute session → TFT shows "07 · SESSION STARTED" for ~1.8s,
   then "08 · ACTIVE SESSION"; browser goes straight to `/dashboard`, never
   blank.
6. Mid-session, remove the headset → TFT shows "10 · NO SIGNAL", motor goes
   off immediately, dashboard shows the NO SIGNAL view, **countdown keeps
   running**, attention shown is not stale.
7. Put the headset back on → TFT and dashboard both resume "ACTIVE SESSION"
   automatically; `motorOnSeconds`/`attentionSampleSum` were not reset.
8. Let the timer reach 0 → TFT shows "11 · SESSION ENDED" and **holds**;
   browser navigates to `/result` with frozen numbers that don't change
   afterward even if you keep wearing the headset.
9. Press "START NEW SESSION" → returns to READY (headset still on/valid) or
   NO SIGNAL (headset off), automatically.
10. Mid-session, hold GPIO13 for 3s → TFT shows the WIFI SETUP countdown,
    then the portal screen with the live AP IP; motor stops safely; connect
    to `MindWave-Setup` and confirm `/` there shows the credential form, not
    a blank/garbled page.
11. Press/hold GPIO13 during READY (no session running) → same WiFi setup
    flow works identically.
12. Start a session, then press Emergency Stop on the dashboard (two-press
    confirm) → motor/relay off immediately, TFT shows "EMERGENCY STOP /
    SYSTEM HALTED", then auto-recovers to READY or NO_SIGNAL after ~1.5s.
13. Close the browser tab mid-session, wait, then reopen `/dashboard` → the
    ESP32 never stopped running the session; the WebSocket reconnects and
    live data resumes.
14. While a real headset is streaming continuously, hammer the HTTP
    endpoints (`/status`, refresh `/dashboard` repeatedly) and confirm
    GPIO13 still responds instantly and the TFT never freezes.
15. Run a full multi-minute session and confirm the countdown shown on the
    TFT and the dashboard never disagree or drift, regardless of WiFi/BLE
    load, and that no screen ever gets silently overwritten.

## L. Demo mode test procedure

Set `#define DEMO_MODE 1`. The exact same 15 tests above apply, with two
differences:

- There is no physical headset — `Demo_Mode.ino::simulateSensorData()`
  drives `attention` through a slow sine wave so the 40%/60% relay/motor
  thresholds trigger naturally, and lets a full session run end-to-end.
- To exercise the NO_SIGNAL path (tests 1, 2, 3, 6, 7) without unplugging
  real hardware, set `demoSimulateSignalLoss = true` (declared in
  `BrainControlMotorSystem.ino`, e.g. toggle it from the serial monitor or
  wire a debug route to it) — this stops `lastValidEEGPacketMillis` from
  advancing and forces `poorQuality = 200`, so `eegSignalValid` times out
  exactly the way a real dropped headset would, proving the state machine
  behaves identically in both modes (req. #1/#35). Set it back to `false`
  to resume normal demo data.

---

## Known, deliberate limitations

- **BluetoothSerial's classic `connect()` is a blocking library call** with
  no async alternative in the ESP32 Arduino core (see §E). The byte-level
  *parser* — the actual bug named in the brief — is fully non-blocking;
  the reconnect *attempt* itself is rate-limited to minimize its impact
  rather than eliminated, which isn't possible with this library.
- **Session state is not persisted across a reboot.** A GPIO13 WiFi-setup
  save always reboots the device (unchanged from the original project); any
  session in progress at that moment is lost. Nothing in the brief asked
  for NVS-backed session persistence, so this wasn't added.
- **`SessionResult`** (req. #31) is implemented as the original project's
  existing set of dedicated "frozen once, in `endSession()`" globals
  (`avgAttentionResult`, `resultDeltaPercent`, etc.) rather than being
  wrapped in a literal C `struct`, to avoid an unnecessary and risky
  rewrite of the ~1500-line PDF/email generator that reads them — they are
  functionally identical to the struct described in the brief (written
  once, read-only afterward until the next session).
