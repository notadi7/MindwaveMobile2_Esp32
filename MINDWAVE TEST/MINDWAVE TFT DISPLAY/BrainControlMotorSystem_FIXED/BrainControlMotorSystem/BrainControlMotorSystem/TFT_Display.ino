/************************************************************
   TFT_DISPLAY.INO
   1.8" ST7735 TFT UI.

   THIS IS A DISPLAY-LAYER MODULE ONLY. It reads existing
   application globals (attention, systemState, eegSignalValid,
   sessionRemainingSeconds, ipAddress, etc.) and draws them. It
   never sets or changes any of them, and never touches
   motor/relay/WiFi/session logic.

   AUTHORITATIVE RENDER PATH (req. #3, #23, #24):
   updateTFT(now) is the ONLY function called from loop(), and
   is the ONLY place any show*()/draw*() screen function is
   invoked. No other module (Session_Manager, Email_Sender,
   WiFi_Setup_Portal, Telemetry, ...) may call a screen function
   directly — they only change systemState/data, and updateTFT()
   notices and renders accordingly. A full screen is redrawn
   only when the state changes; ACTIVE_SESSION and WIFI_COUNTDOWN
   get a bounded periodic "live data" redraw on top of that
   (TFT_LIVE_REFRESH_MS); every other screen (READY, NO_SIGNAL,
   SESSION_ENDED, SENSOR_CONNECTING, WIFI_PORTAL, SESSION_STARTED,
   EMERGENCY_STOP) is drawn once on entry and then HELD, exactly
   as required (e.g. SESSION_ENDED must never be silently
   overwritten by a stray EEG-driven redraw).

   DRIVER NOTE: no ST7735 driver file was supplied with this
   project, so this uses the standard Adafruit_ST7735 library
   (pairs with the Adafruit_GFX core already used by the old
   OLED code) initialized with INITR_BLACKTAB, the most common
   tab variant for generic 1.8" 128x160 ST7735 boards. If your
   panel is a different tab/offset variant, change the single
   tft.initR(...) line in initTFT() below — everything else is
   unaffected.
************************************************************/

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

/******************** TFT PIN MAP (per spec) ********************
   MOSI (23) and SCLK (18) are ESP32's default hardware VSPI
   pins, so we use the fast hardware-SPI constructor rather than
   bit-banged software SPI - keeps redraws quick and non-blocking
   relative to WiFi/WebSocket/telemetry work in loop().
*******************************************************************/
#define TFT_CS   5
#define TFT_RST  4
#define TFT_DC   2
#define TFT_MOSI 23   // ESP32 default VSPI MOSI - not passed to constructor, documented for reference
#define TFT_SCLK 18   // ESP32 default VSPI SCLK - not passed to constructor, documented for reference

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

/******************** COLOR PALETTE (RGB565) ********************/
#define COLOR_BG      0x0000   // black
#define COLOR_PANEL   0x18E3   // dark navy panel fill
#define COLOR_WHITE   0xFFFF
#define COLOR_GRAY    0x8410
#define COLOR_CYAN    0x07FF
#define COLOR_GREEN   0x07E6
#define COLOR_RED     0xF800
#define COLOR_ORANGE  0xFD20
#define COLOR_BLUE    0x051D
#define COLOR_YELLOW  0xFFE0
// New accent colors used by the redesigned boot screen (and other
// restyled screens) — add alongside your existing COLOR_* defines
#define COLOR_PURPLE  0x801F   // brand / boot-title accent
#define COLOR_TEAL    0x05B6   // sliding bar / secondary success accent
#define COLOR_AMBER   0xFDA0   // mid-attention accent (used elsewhere in the theme)
#define COLOR_BORDER  0x39C7   // card border / muted divider

/************************************************************
   ⚙️ INIT
************************************************************/
void initTFT() {
  tft.initR(INITR_BLACKTAB);   // <-- see driver note above if colors/offsets look wrong
  tft.setRotation(1);          // landscape, 160x128 - matches the horizontal layout below
  tft.fillScreen(COLOR_BG);
  tft.setTextWrap(false);
}

/************************************************************
   🧱 SHARED LOW-LEVEL HELPERS
************************************************************/
void drawCenteredText(String text, int y) {
  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  int x = (tft.width() - w) / 2;
  tft.setCursor(x, y);
  tft.print(text);
}

void tftHeader(String title, uint16_t color) {
  tft.fillRect(0, 0, tft.width(), 18, color);
  tft.setTextColor(COLOR_BG);
  tft.setTextSize(1);
  int16_t x1, y1; uint16_t w, h;
  tft.getTextBounds(title, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((tft.width() - w) / 2, 5);
  tft.print(title);
}

/************************************************************
   📟 GENERIC 3-LINE TEXT SCREEN
   Kept with the exact same signature as the old OLED version
   so existing call sites (WiFi_Connect.ino boot-time messages,
   EMERGENCY_STOP) work unmodified.
************************************************************/
void showText(String line1, String line2, String line3) {
  tft.fillScreen(COLOR_BG);
  tftHeader("WIFI CONNECTION", COLOR_CYAN);

  tft.setTextColor(COLOR_WHITE);
  tft.setTextSize(1);
  drawCenteredText(line1, 40);
  if (line2.length()) drawCenteredText(line2, 60);
  if (line3.length()) drawCenteredText(line3, 80);
}

/************************************************************
   00 · BOOT SCREEN
   Org branding on top ("TBM FINISHING SCHOOL" + presenter
   line), project name below it in two accent colors, then a
   sliding indeterminate loading bar under "INITIALIZING".

   This is the one screen allowed to animate with delay() calls:
   it runs once inside setup(), entirely BEFORE loop() starts,
   so there is no WiFi/WebSocket/motor servicing happening yet
   for a short animation to block. Once loop() begins, no other
   screen in this file ever uses delay() — that rule is
   unchanged everywhere else.

   BOOT_ANIM_FRAMES / BOOT_ANIM_STEP_MS control total animation
   length (≈1.4s here) and smoothness — tune freely, this does
   not affect anything else in the firmware.
************************************************************/
#define BOOT_ANIM_FRAMES   28
#define BOOT_ANIM_STEP_MS  50

void showBootScreen() {

  // ============================================================
  // PROFESSIONAL BLACK BOOT SCREEN
  // ============================================================

  // Pure black background
  tft.fillScreen(ST77XX_BLACK);

  // ------------------------------------------------------------
  // TOP ACCENT LINE
  // ------------------------------------------------------------
  tft.drawFastHLine(10, 4, 140, COLOR_BORDER);
  tft.fillCircle(10, 4, 2, COLOR_CYAN);
  tft.fillCircle(150, 4, 2, COLOR_PURPLE);

  // ------------------------------------------------------------
  // ORGANIZATION
  // ------------------------------------------------------------
  tft.setTextSize(1);

  tft.setTextColor(COLOR_GRAY);
  drawCenteredText("TBM FINISHING SCHOOL", 10);

  // Thin separator
  tft.drawFastHLine(35, 17, 90, COLOR_BORDER);

  // ------------------------------------------------------------
  // PSYCHOLOGIST NAME
  // ------------------------------------------------------------
  tft.setTextColor(COLOR_WHITE);
  drawCenteredText("NILESH GUNJAL", 27);

  tft.setTextColor(COLOR_CYAN);
  drawCenteredText("PSYCHOLOGIST", 37);

  // ------------------------------------------------------------
  // PROJECT SECTION
  // ------------------------------------------------------------
  tft.drawFastHLine(20, 47, 120, COLOR_BORDER);

  // Small left/right accents
  tft.fillRect(20, 46, 18, 2, COLOR_PURPLE);
  tft.fillRect(122, 46, 18, 2, COLOR_CYAN);

  // Project name
  tft.setTextColor(COLOR_PURPLE);
  drawCenteredText("BRAIN CONTROL", 57);

  tft.setTextColor(COLOR_CYAN);
  drawCenteredText("MOTOR SYSTEM", 69);

  // ------------------------------------------------------------
  // INITIALIZATION STATUS
  // ------------------------------------------------------------
  tft.setTextColor(COLOR_GRAY);
  drawCenteredText("INITIALIZING", 84);

  // Status dots
  tft.fillCircle(40, 84, 1, COLOR_PURPLE);
  tft.fillCircle(120, 84, 1, COLOR_CYAN);

  // ------------------------------------------------------------
  // LOADING BAR
  // ------------------------------------------------------------
  int trackX = 18;
  int trackY = 96;
  int trackW = 124;
  int trackH = 8;

  // Outer border
  tft.drawRoundRect(
    trackX,
    trackY,
    trackW,
    trackH,
    2,
    COLOR_BORDER
  );

  int segW = 30;
  int travel = trackW - 4 + segW;

  // ------------------------------------------------------------
  // ANIMATION
  // ------------------------------------------------------------
  for (int frame = 0; frame < BOOT_ANIM_FRAMES; frame++) {

    int segX =
      trackX + 2 - segW +
      (travel * frame / (BOOT_ANIM_FRAMES - 1));

    // Clear inside bar only
    tft.fillRect(
      trackX + 2,
      trackY + 2,
      trackW - 4,
      trackH - 4,
      ST77XX_BLACK
    );

    int drawX = segX;
    int drawW = segW;

    // Left clipping
    if (drawX < trackX + 2) {
      drawW += drawX - (trackX + 2);
      drawX = trackX + 2;
    }

    // Right clipping
    int rightEdge = trackX + trackW - 2;

    if (drawX + drawW > rightEdge) {
      drawW = rightEdge - drawX;
    }

    if (drawW > 0) {

      // Main cyan segment
      tft.fillRoundRect(
        drawX,
        trackY + 2,
        drawW,
        trackH - 4,
        2,
        COLOR_CYAN
      );

      // Purple leading accent
      if (drawW > 6) {
        tft.fillRect(
          drawX,
          trackY + 2,
          5,
          trackH - 4,
          COLOR_PURPLE
        );
      }
    }

    delay(BOOT_ANIM_STEP_MS);
  }

  // ------------------------------------------------------------
  // FINAL LOADING STATE
  // ------------------------------------------------------------
  tft.fillRoundRect(
    trackX + 2,
    trackY + 2,
    trackW - 4,
    trackH - 4,
    2,
    COLOR_CYAN
  );

  delay(150);
}
/************************************************************
   · DEMO MODE (boot-time only, shown once before loop() starts)
************************************************************/
void showDemoMode() {
  tft.fillScreen(COLOR_BG);
  tftHeader("DEMO MODE", COLOR_ORANGE);
  tft.setTextColor(COLOR_ORANGE);
  tft.setTextSize(1);
  drawCenteredText("SENSOR SIMULATION", 45);
  drawCenteredText("DATA ACTIVE", 65);
  tft.setTextColor(COLOR_GRAY);
  drawCenteredText("No hardware needed", 90);
}

/************************************************************
   · MINDWAVE NOT CONNECTED / TRYING TO CONNECT
   STATE_SENSOR_CONNECTING. Exact text per req. #4 — must NEVER
   show attention, old attention, ACTIVE SESSION, or READY.
************************************************************/
void showSensorConnecting() {
  tft.fillScreen(COLOR_BG);
  tftHeader("MINDWAVE MOBILE 2", COLOR_CYAN);
  tft.setTextColor(COLOR_RED);
  tft.setTextSize(1);
  drawCenteredText("NOT CONNECTED", 50);
  tft.setTextColor(COLOR_GRAY);
  drawCenteredText("TRYING TO CONNECT", 72);
  tft.drawCircle(80, 100, 10, COLOR_CYAN);
}

/************************************************************
   05 · WIFI SETUP (GPIO13 hold countdown)
   secondsLeft comes straight from WIFI_HOLD_MS / btnPressStart
   in WiFi_Setup_Portal.ino - not hardcoded here, so it can never
   drift out of sync with the actual hold duration.
************************************************************/
void showWifiSetup(int secondsLeft) {
  if (secondsLeft < 0) secondsLeft = 0;
  tft.fillScreen(COLOR_BG);
  tftHeader("05 . WIFI SETUP", COLOR_BLUE);
  tft.setTextColor(COLOR_WHITE);
  tft.setTextSize(1);
  drawCenteredText("HOLD BUTTON", 45);
  tft.setTextColor(COLOR_CYAN);
  tft.setTextSize(2);
  drawCenteredText(String(secondsLeft), 70);
}

/************************************************************
   06 · WIFI PORTAL ACTIVE
   AP IP is ALWAYS read live from WiFi.softAPIP() - never a
   stale/cached string (req. #27).
************************************************************/
void showWifiPortal() {
  tft.fillScreen(COLOR_BG);
  tftHeader("05 . WIFI SETUP", COLOR_BLUE);
  tft.setTextColor(COLOR_CYAN);
  tft.setTextSize(1);
  drawCenteredText("MindWave-Setup", 42);
  tft.setTextColor(COLOR_GRAY);
  drawCenteredText("IP:", 62);
  tft.setTextColor(COLOR_WHITE);
  tft.setTextSize(1);
  drawCenteredText(WiFi.softAPIP().toString(), 78);
  tft.setTextColor(COLOR_GREEN);
  drawCenteredText("OPEN WIFI SETUP PORTAL", 100);
}

/************************************************************
   07 · SESSION STARTED (transient hold, SESSION_STARTED_SCREEN_MS)
************************************************************/
void showSessionStarted(String name, int minutes) {
  tft.fillScreen(COLOR_BG);
  tftHeader("07 . SESSION STARTED", COLOR_GREEN);
  tft.setTextColor(COLOR_CYAN);
  tft.setTextSize(1);
  drawCenteredText(name, 55);
  tft.setTextColor(COLOR_GRAY);
  drawCenteredText(String(minutes) + " MIN SESSION", 78);
}

/************************************************************
   08 · ACTIVE SESSION - PRIMARY LIVE SCREEN
   FLICKER FIX: previously this was one function that called
   tft.fillScreen() on every single refresh (every
   TFT_LIVE_REFRESH_MS), wiping and redrawing the ENTIRE screen
   — including the header, "ATTENTION" label, and bar outline —
   even though only the numbers actually change. That full
   black-out-then-redraw is what caused the visible blink.

   Split into two functions:
     drawActiveSessionStatic()  — header/labels/bar outline/IP,
                                   drawn ONCE on state entry.
     refreshActiveSessionLive() — ONLY the values that actually
                                   change (attention number, bar
                                   fill, motor/relay status,
                                   countdown), called on entry
                                   AND on every live-refresh tick.
                                   Never calls fillScreen(); each
                                   value clears just its own
                                   fixed-size region first, so
                                   nothing outside that region is
                                   ever touched -> no flicker.
************************************************************/
void drawActiveSessionStatic() {
  tft.fillScreen(COLOR_BG);
  tftHeader("08 . ACTIVE SESSION", COLOR_CYAN);

  tft.setTextColor(COLOR_GRAY);
  tft.setTextSize(1);
  drawCenteredText("ATTENTION", 24);

  // Progress bar OUTLINE only — the fill is live, drawn in
  // refreshActiveSessionLive().
  int barW = 130, barX = (tft.width() - barW) / 2, barY = 62;
  tft.drawRect(barX, barY, barW, 8, COLOR_GRAY);

  // Row labels — these text strings themselves never change
  // during a session, only the values after them do.
  tft.setTextSize(1);
  tft.setTextColor(COLOR_WHITE);
  tft.setCursor(4, 78);
  tft.print("MOTOR ");

  tft.setTextColor(COLOR_WHITE);
  tft.setCursor(4, 90);
  tft.print("RELAY ");

  tft.setTextColor(COLOR_WHITE);
  tft.setCursor(4, 102);
  tft.print("SIGNAL ");
  // This screen is only ever shown while eegSignalValid is true
  // (mapStateToTFTScreen() routes to TFT_NO_SIGNAL otherwise), so
  // "GOOD" here is static for the whole time this screen is up.
  tft.setTextColor(COLOR_GREEN);
  tft.print("GOOD");

  tft.setTextColor(COLOR_CYAN);
  tft.setCursor(4, 116);
  tft.print("SESSION ");

  // IP address doesn't change mid-session — draw it once here.
  tft.setTextColor(COLOR_GRAY);
  tft.setCursor(90, 116);
  tft.print(ipAddress);
}

void refreshActiveSessionLive(int speedPercent) {
  // Visual-only attention category. Never used for control -
  // Motor_Control.ino's digitalWrite()/analogWrite() logic runs
  // on the real 40/60 thresholds, completely independent of this.
  uint16_t attColor = COLOR_RED;
  if (attention > 60) attColor = COLOR_GREEN;
  else if (attention > 40) attColor = COLOR_BLUE;

  // ── Big attention number: clear only its own bounding box
  //    (wide enough for "0%".."100%"), then draw the new value.
  //    This is the only fillRect big enough to matter, and it's
  //    a small fraction of the screen, so no visible blink.
  int attBoxX = 14, attBoxY = 33, attBoxW = tft.width() - 28, attBoxH = 26;
  tft.fillRect(attBoxX, attBoxY, attBoxW, attBoxH, COLOR_BG);

  tft.setTextColor(attColor);
  tft.setTextSize(3);
  String attStr = String(attention) + "%";
  int16_t x1, y1; uint16_t w, h;
  tft.getTextBounds(attStr, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((tft.width() - w) / 2, 36);
  tft.print(attStr);

  // ── Progress bar fill: clear just the bar interior, then
  //    redraw the new fill width. Never touches the outline.
  int barW = 130, barX = (tft.width() - barW) / 2, barY = 62;
  tft.fillRect(barX + 1, barY + 1, barW - 2, 6, COLOR_BG);
  int fillW = (barW - 2) * (int)attention / 100;
  if (fillW > 0) tft.fillRect(barX + 1, barY + 1, fillW, 6, attColor);

  // ── MOTOR value (after the static "MOTOR " label, 6 chars =
  //    36px at size1, so the value starts at x=4+36=40). Fixed-
  //    width formatting ("%-3s" status + "%3d%%" percent) means
  //    the string is always the same length, so drawing straight
  //    over the old text with an opaque (fg,bg) color always
  //    fully erases it — no leftover digits/ghosting even when
  //    going from e.g. "100%" down to "0%".
  bool relayOn = (digitalRead(RELAY_PIN) == LOW);
  char motorBuf[12];
  snprintf(motorBuf, sizeof(motorBuf), "%-3s%3d%%", relayOn ? "ON" : "OFF", speedPercent);
  tft.setTextSize(1);
  tft.setCursor(40, 78);
  tft.setTextColor(relayOn ? COLOR_GREEN : COLOR_GRAY, COLOR_BG);
  tft.print(motorBuf);

  // ── RELAY value (after "RELAY ", same 40px start). "ON "/"OFF"
  //    are already the same fixed length, so no ghosting risk.
  tft.setCursor(40, 90);
  tft.setTextColor(relayOn ? COLOR_GREEN : COLOR_GRAY, COLOR_BG);
  tft.print(relayOn ? "ON " : "OFF");

  // ── Countdown timer (after "SESSION ", 8 chars = 48px at
  //    size1, so it starts at x=4+48=52). "%02d:%02d" is always
  //    exactly 5 characters, so opaque redraw is always clean.
  long remaining = sessionRemainingSeconds;
  int mm = remaining / 60;
  int ss = remaining % 60;
  char timeBuf[8];
  snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", mm, ss);
  tft.setCursor(52, 116);
  tft.setTextColor(COLOR_CYAN, COLOR_BG);
  tft.print(timeBuf);
}

/************************************************************
   · BRAINWAVES (compact panel, not currently wired to any
   state - kept available for future use, satisfies the
   module's screen set)
************************************************************/
void showBrainwaves() {
  tft.fillScreen(COLOR_BG);
  tftHeader("BRAINWAVES", COLOR_CYAN);

  struct Wave { const char* label; int pct; uint16_t color; };
  Wave waves[5] = {
    {"D", deltaPercent, COLOR_BLUE},
    {"T", thetaPercent, COLOR_CYAN},
    {"A", alphaPercent, COLOR_GREEN},
    {"B", betaPercent,  COLOR_YELLOW},
    {"G", gammaPercent, COLOR_ORANGE},
  };

  int y = 30;
  for (int i = 0; i < 5; i++) {
    tft.setTextColor(COLOR_WHITE);
    tft.setCursor(4, y);
    tft.print(waves[i].label);
    tft.print(" ");
    tft.print(waves[i].pct);
    tft.print("%");

    int barX = 40, barW = 110;
    tft.drawRect(barX, y, barW, 10, COLOR_GRAY);
    int fillW = (barW - 2) * waves[i].pct / 100;
    if (fillW > barW - 2) fillW = barW - 2;
    if (fillW > 0) tft.fillRect(barX + 1, y + 1, fillW, 8, waves[i].color);

    y += 18;
  }
}

/************************************************************
   10 · NO SIGNAL
   Exact req. #4 text. Shown both for STATE_NO_SIGNAL (idle,
   before a session) and for STATE_ACTIVE_SESSION when
   eegSignalValid is false (req. #11) - the caller (updateTFT)
   decides which case it is; the screen itself is identical.
************************************************************/
void showNoSignal() {
  tft.fillScreen(COLOR_BG);
  tftHeader("10 . NO SIGNAL", COLOR_RED);
  tft.setTextColor(COLOR_ORANGE);
  tft.setTextSize(1);
  drawCenteredText("WAITING FOR EEG SIGNAL", 55);
  tft.setTextColor(COLOR_GRAY);
  drawCenteredText("IP: " + ipAddress, 100);
}

/************************************************************
   · READY (idle, default screen - req. #9: NO live attention
   gauge here, ever)
************************************************************/
void showReady() {
  tft.fillScreen(COLOR_BG);
  tftHeader("READY", COLOR_GREEN);
  tft.setTextColor(COLOR_GREEN);
  tft.setTextSize(1);
  drawCenteredText("SENSOR READY", 40);
  tft.setTextColor(COLOR_GRAY);
  drawCenteredText("IP:", 62);
  tft.setTextColor(COLOR_WHITE);
  drawCenteredText(ipAddress, 76);
  tft.setTextColor(COLOR_CYAN);
  drawCenteredText("START SESSION FROM", 98);
  drawCenteredText("WEB DASHBOARD", 110);
}

/************************************************************
   11 · SESSION ENDED
   Drawn once on entry, then HELD (updateTFT() never calls this
   again while systemState == STATE_SESSION_ENDED, req. #15/#23).
************************************************************/
void showSessionEnded(int avgAttention, unsigned long motorSeconds) {
  tft.fillScreen(COLOR_BG);
  tftHeader("11 . SESSION ENDED", COLOR_GREEN);
  tft.setTextColor(COLOR_WHITE);
  tft.setTextSize(1);
  drawCenteredText("SESSION COMPLETE", 24);

  tft.setTextColor(COLOR_GRAY);
  drawCenteredText("AVG ATTENTION", 38);
  tft.setTextColor(COLOR_CYAN);
  tft.setTextSize(2);
  drawCenteredText(String(avgAttention) + "%", 50);

  int mm = motorSeconds / 60;
  int ss = motorSeconds % 60;
  char buf[8];
  snprintf(buf, sizeof(buf), "%02d:%02d", mm, ss);

  tft.setTextSize(1);
  tft.setTextColor(COLOR_GRAY);
  drawCenteredText("MOTOR TIME", 76);
  tft.setTextColor(COLOR_WHITE);
  tft.setTextSize(2);
  drawCenteredText(String(buf), 88);

  tft.setTextSize(1);
  tft.setTextColor(COLOR_GREEN);
  drawCenteredText("VIEW RESULT ON DASHBOARD", 114);
}

/************************************************************
   ═══════════════════════════════════════════════════════════
   THE SINGLE AUTHORITATIVE RENDER PATH
   ═══════════════════════════════════════════════════════════
   Maps systemState (+ eegSignalValid, for the ACTIVE_SESSION
   special case) to a screen, redraws fully whenever that mapped
   screen changes, and otherwise only issues the bounded live
   refreshes described at the top of this file. This is the only
   function called from loop(); it is the only place any of the
   show*()/update*Display() functions above are invoked.
************************************************************/
enum TFTScreen {
  TFT_BOOT,
  TFT_WIFI_CONNECTING,
  TFT_SENSOR_CONNECTING,
  TFT_NO_SIGNAL,
  TFT_READY,
  TFT_WIFI_COUNTDOWN,
  TFT_WIFI_PORTAL,
  TFT_SESSION_STARTED,
  TFT_ACTIVE_SESSION,
  TFT_SESSION_ENDED,
  TFT_EMERGENCY_STOP
};

TFTScreen mapStateToTFTScreen(SystemState s) {
  switch (s) {
    case STATE_BOOT:              return TFT_BOOT;
    case STATE_WIFI_CONNECTING:   return TFT_WIFI_CONNECTING;
    case STATE_SENSOR_CONNECTING: return TFT_SENSOR_CONNECTING;
    case STATE_NO_SIGNAL:         return TFT_NO_SIGNAL;
    case STATE_READY:             return TFT_READY;
    case STATE_WIFI_COUNTDOWN:    return TFT_WIFI_COUNTDOWN;
    case STATE_WIFI_PORTAL:       return TFT_WIFI_PORTAL;
    case STATE_SESSION_STARTED:   return TFT_SESSION_STARTED;
    case STATE_ACTIVE_SESSION:    return eegSignalValid ? TFT_ACTIVE_SESSION : TFT_NO_SIGNAL;
    case STATE_SESSION_ENDED:     return TFT_SESSION_ENDED;
    case STATE_EMERGENCY_STOP:    return TFT_EMERGENCY_STOP;
  }
  return TFT_BOOT;
}

void renderTFTScreen(TFTScreen s) {
  switch (s) {
    case TFT_BOOT:              showBootScreen(); break;
    case TFT_WIFI_CONNECTING:   showText("CONNECTING", "WIFI..."); break;
    case TFT_SENSOR_CONNECTING: showSensorConnecting(); break;
    case TFT_NO_SIGNAL:         showNoSignal(); break;
    case TFT_READY:             showReady(); break;
    case TFT_WIFI_COUNTDOWN:    showWifiSetup((int)(WIFI_HOLD_MS / 1000)); break;
    case TFT_WIFI_PORTAL:       showWifiPortal(); break;
    case TFT_SESSION_STARTED:   showSessionStarted(userName, sessionMinutes); break;
    case TFT_ACTIVE_SESSION:
      drawActiveSessionStatic();
      refreshActiveSessionLive(motorSpeedPercent);   // paint initial values immediately, not after the first 300ms tick
      break;
    case TFT_SESSION_ENDED:     showSessionEnded(avgAttentionResult, motorOnSeconds); break;
    case TFT_EMERGENCY_STOP:    showText("EMERGENCY STOP", "SYSTEM HALTED"); break;
  }
}

void updateTFT(unsigned long now) {
  static TFTScreen lastScreen = (TFTScreen)-1;
  static unsigned long lastLiveRefresh = 0;
  static int lastCountdownShown = -1;

  TFTScreen desired = mapStateToTFTScreen(systemState);
  bool stateChanged = (desired != lastScreen);

  if (stateChanged) {
    renderTFTScreen(desired);
    lastScreen = desired;
    lastLiveRefresh = now;
    lastCountdownShown = -1;
    return;
  }

  switch (desired) {

    case TFT_ACTIVE_SESSION:
      // Live gauge/timer refresh, bounded to TFT_LIVE_REFRESH_MS -
      // never redrawn every loop (req. #3).
      if (now - lastLiveRefresh >= TFT_LIVE_REFRESH_MS) {
        lastLiveRefresh = now;
        refreshActiveSessionLive(motorSpeedPercent);
      }
      break;

    case TFT_WIFI_COUNTDOWN: {
      // Countdown number only changes once a second, but we poll on
      // TFT_LIVE_REFRESH_MS and only actually redraw when the
      // displayed digit changes, to avoid flicker.
      long heldMs = (long)(now - wifiCountdownStartedAt);
      int secsLeft = ((long)WIFI_HOLD_MS - heldMs) / 1000 + 1;
      if (secsLeft < 0) secsLeft = 0;
      if (secsLeft != lastCountdownShown) {
        lastCountdownShown = secsLeft;
        showWifiSetup(secsLeft);
      }
      break;
    }

    default:
      // READY / NO_SIGNAL / SESSION_ENDED / SENSOR_CONNECTING /
      // WIFI_PORTAL / SESSION_STARTED / EMERGENCY_STOP / BOOT /
      // WIFI_CONNECTING: drawn once on entry above, then held.
      // Nothing to do here - this is what stops SESSION_ENDED (etc.)
      // from ever being silently overwritten.
      break;
  }
}
