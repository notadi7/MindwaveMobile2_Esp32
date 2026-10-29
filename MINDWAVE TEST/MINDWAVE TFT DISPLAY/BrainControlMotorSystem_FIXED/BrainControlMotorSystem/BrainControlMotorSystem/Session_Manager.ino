/************************************************************
   SESSION_MANAGER.INO
   The session state machine's logic: starting a session (from
   the setup page POST), holding the transient "SESSION STARTED"
   screen, advancing to ACTIVE_SESSION, ending one automatically
   when the server-authoritative timer reaches zero, resetting
   for a new one, and emergency stop.

   TIMER (req. #12/#13): sessionRemainingSeconds is recomputed
   every call from two absolute values (sessionStartMillis,
   sessionDurationSeconds) — it is never decremented by a tick
   function, so WebSocket load, TFT refresh, or a slow browser
   can never make it drift.
************************************************************/

void resetSessionAccumulators() {
  sessionRemainingSeconds = 0;
  sessionDurationSeconds = 0;
  motorOnSeconds = 0;
  lastAccumMillis = millis();
  attentionSampleSum = 0;
  attentionSampleCount = 0;
  attentionPeak = 0;
  avgAttentionResult = 0;
  peakAttentionResult = 0;
  resultDeltaPercent = 0;
  resultThetaPercent = 0;
  resultAlphaPercent = 0;
  resultBetaPercent = 0;
  resultGammaPercent = 0;
  resultFocusScore = 0;
  emailInProgress = false;
  emailLastResult = false;
  emailAttempted = false;
}

/************************************************************
   🔁 CALLED EVERY loop() — advances SESSION_STARTED ->
   ACTIVE_SESSION, recomputes the authoritative countdown, ends
   the session at zero, and recovers from EMERGENCY_STOP after
   its hold period. This is the ONLY function allowed to move
   systemState into/out of these states.
************************************************************/
void updateSessionState(unsigned long now) {
  switch (systemState) {

    case STATE_SESSION_STARTED:
      if (now - sessionStartedScreenSince >= SESSION_STARTED_SCREEN_MS) {
        systemState = STATE_ACTIVE_SESSION;
      }
      break;

    case STATE_ACTIVE_SESSION: {
      long elapsed = (long)((now - sessionStartMillis) / 1000UL);
      long remaining = sessionDurationSeconds - elapsed;
      if (remaining < 0) remaining = 0;
      sessionRemainingSeconds = remaining;

      if (remaining <= 0) {
        endSession();
      }
      break;
    }

    case STATE_EMERGENCY_STOP:
      if (now - emergencyStopSince >= EMERGENCY_STOP_HOLD_MS) {
        systemState = eegSignalValid ? STATE_READY : STATE_NO_SIGNAL;
      }
      break;

    default:
      break;   // every other state is owned elsewhere
  }
}

/************************************************************
   🛑 END SESSION (called from updateSessionState() when the
   countdown hits 0). Freezes the result numbers ONCE — nothing
   after this point may modify them until the next session
   starts (req. #16/#17/#31).
************************************************************/
void endSession() {
  // Force motor/relay off immediately. updateMotorControl() will also
  // force them off on the very next loop() (systemState is about to
  // stop being STATE_ACTIVE_SESSION), but do it here too so there is
  // zero delay between "timer hits 0" and "motor stops".
  digitalWrite(RELAY_PIN, HIGH);
  analogWrite(ENA, 0);

  avgAttentionResult = attentionSampleCount ? (attentionSampleSum / attentionSampleCount) : 0;
  peakAttentionResult = attentionPeak;
  sessionRemainingSeconds = 0;

  resultDeltaPercent = deltaPercent;
  resultThetaPercent = thetaPercent;
  resultAlphaPercent = alphaPercent;
  resultBetaPercent  = betaPercent;
  resultGammaPercent = gammaPercent;

  // Focus score /100: 50% avg attention + 20% peak attention +
  // 30% how much of the session the motor spent running.
  long totalSessionSeconds = sessionDurationSeconds;
  int motorOnRatio = 0;
  if (totalSessionSeconds > 0) {
    motorOnRatio = (int)((motorOnSeconds * 100) / totalSessionSeconds);
  }
  if (motorOnRatio > 100) motorOnRatio = 100;
  resultFocusScore = (avgAttentionResult * 5 + peakAttentionResult * 2 + motorOnRatio * 3 + 5) / 10;
  if (resultFocusScore > 100) resultFocusScore = 100;
  if (resultFocusScore < 0) resultFocusScore = 0;

  systemState = STATE_SESSION_ENDED;

  Serial.println("SESSION ENDED - entering STATE_SESSION_ENDED");
  // NOTE: no showSessionEnded() call here — TFT_Display.ino::updateTFT()
  // is the ONLY function allowed to render, and it renders this screen
  // (and holds it) the moment it observes systemState == STATE_SESSION_ENDED.
}

/************************************************************
   📩 POST /start-session
   Body: application/x-www-form-urlencoded
     name=<string>&email=<string>&minutes=<1..30>

   Responds with JSON (req. #20 — never a 30x redirect that can
   leave the browser on a blank page). The page itself navigates
   explicitly to /dashboard once it sees ok:true.
************************************************************/
void handleStartSession() {
  if (systemState != STATE_READY) {
    server.send(409, "application/json",
      "{\"ok\":false,\"error\":\"Sensor not ready or a session is already running\"}");
    return;
  }
  if (!mindwaveConnected || !eegSignalValid) {
    server.send(409, "application/json",
      "{\"ok\":false,\"error\":\"No valid EEG signal\"}");
    return;
  }
  if (!server.hasArg("name") || !server.hasArg("email") || !server.hasArg("minutes")) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"Missing name, email, or minutes\"}");
    return;
  }

  String name = server.arg("name");
  String email = server.arg("email");
  int minutes = server.arg("minutes").toInt();

  // Server-side validation mirrors the client-side checks — 0 minutes
  // is explicitly disallowed, range clamped to 1-30.
  if (name.length() == 0 || email.length() == 0 || email.indexOf('@') < 0) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"Invalid name or email\"}");
    return;
  }
  if (minutes < 1) minutes = 1;
  if (minutes > 30) minutes = 30;

  userName = name;
  userEmail = email;
  sessionMinutes = minutes;

  resetSessionAccumulators();
  sessionDurationSeconds = (long)sessionMinutes * 60;
  sessionRemainingSeconds = sessionDurationSeconds;
  sessionStartMillis = millis();
  sessionStartedScreenSince = sessionStartMillis;
  systemState = STATE_SESSION_STARTED;

  Serial.println("Session started for " + userName + " (" + String(sessionMinutes) + " min)");

  server.send(200, "application/json", "{\"ok\":true,\"state\":\"SESSION_STARTED\"}");
}

/************************************************************
   🔄 POST /new-session — clears everything and returns to
   READY or NO_SIGNAL depending on current EEG status (req. #18).
************************************************************/
void handleNewSession() {
  resetSessionAccumulators();
  userName = "";
  userEmail = "";
  sessionMinutes = 0;

  systemState = eegSignalValid ? STATE_READY : STATE_NO_SIGNAL;

  server.send(200, "application/json",
    String("{\"ok\":true,\"state\":\"") + systemStateName(systemState) + "\"}");
}

/************************************************************
   🛑 POST /estop — EMERGENCY STOP
   Cuts the relay and PWM immediately (doesn't wait for the
   next loop's updateMotorControl), fully clears the session
   (req. #19 — unlike the GPIO13 WiFi path, which preserves
   session data), and broadcasts the new state immediately so
   the dashboard doesn't have to wait for the next telemetry
   tick to notice.
************************************************************/
void handleEmergencyStop() {
  // Hardware off FIRST — before any bookkeeping.
  digitalWrite(RELAY_PIN, HIGH);   // relay is active-LOW, so HIGH = OFF
  analogWrite(ENA, 0);
  motorRunning = false;
  motorSpeedPercent = 0;
  ledOn = false;

  resetSessionAccumulators();
  userName = "";
  userEmail = "";
  sessionMinutes = 0;

  systemState = STATE_EMERGENCY_STOP;
  emergencyStopSince = millis();

  broadcastTelemetry();   // don't wait for the next TELEMETRY_INTERVAL_MS tick

  server.send(200, "application/json", "{\"ok\":true,\"state\":\"EMERGENCY_STOP\"}");
}
