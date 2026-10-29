/************************************************************
   SESSION_MANAGER.INO
   The session state machine's logic: starting a session
   (from the setup page POST), ending one automatically when
   the countdown hits zero, resetting for a new one, and the
   shared accumulator-reset helper all three of those use.
************************************************************/

void resetSessionAccumulators() {
  sessionRemainingSeconds = 0;
  motorOnSeconds = 0;
  relayWasOn = false;
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
   🛑 END SESSION (called from loop() when countdown hits 0)
************************************************************/
void endSession() {
  // Force motor/relay off immediately regardless of attention.
  digitalWrite(RELAY_PIN, HIGH);
  analogWrite(ENA, 0);

  // Freeze the result numbers for the result page.
  avgAttentionResult = attentionSampleCount ? (attentionSampleSum / attentionSampleCount) : 0;
  peakAttentionResult = attentionPeak;
  sessionRemainingSeconds = 0;

  // Freeze the brainwave mix at the moment the session ended.
  resultDeltaPercent = deltaPercent;
  resultThetaPercent = thetaPercent;
  resultAlphaPercent = alphaPercent;
  resultBetaPercent  = betaPercent;
  resultGammaPercent = gammaPercent;

  // Focus score /100: 50% avg attention + 20% peak attention +
  // 30% how much of the session the motor spent running.
  // (All-integer math, +5 before the final /10 rounds to nearest.)
  long totalSessionSeconds = (long)sessionMinutes * 60;
  int motorOnRatio = 0;
  if (totalSessionSeconds > 0) {
    motorOnRatio = (int)((motorOnSeconds * 100) / totalSessionSeconds);
  }
  if (motorOnRatio > 100) motorOnRatio = 100;
  resultFocusScore = (avgAttentionResult * 5 + peakAttentionResult * 2 + motorOnRatio * 3 + 5) / 10;
  if (resultFocusScore > 100) resultFocusScore = 100;
  if (resultFocusScore < 0) resultFocusScore = 0;

  appState = STATE_RESULT;

  showText("Session Ended", "Avg Att: " + String(avgAttentionResult) + "%", "Motor: " + String(motorOnSeconds) + "s");

  Serial.println("SESSION ENDED - entering STATE_RESULT");
}

/************************************************************
   📩 POST /start-session
   Body: application/x-www-form-urlencoded
     name=<string>&email=<string>&minutes=<1..30>
************************************************************/
void handleStartSession() {
  if (!server.hasArg("name") || !server.hasArg("email") || !server.hasArg("minutes")) {
    server.send(400, "text/plain", "Missing name, email, or minutes");
    return;
  }

  String name = server.arg("name");
  String email = server.arg("email");
  int minutes = server.arg("minutes").toInt();

  // Server-side validation mirrors the client-side checks — 0 minutes
  // is explicitly disallowed, range clamped to 1-30.
  if (name.length() == 0 || email.length() == 0 || email.indexOf('@') < 0) {
    server.send(400, "text/plain", "Invalid name or email");
    return;
  }
  if (minutes < 1) minutes = 1;
  if (minutes > 30) minutes = 30;

  userName = name;
  userEmail = email;
  sessionMinutes = minutes;

  resetSessionAccumulators();
  sessionRemainingSeconds = (long)sessionMinutes * 60;
  sessionStartMillis = millis();
  appState = STATE_DASHBOARD;

  showText("Session Started", userName, String(sessionMinutes) + " min");
  Serial.println("Session started for " + userName + " (" + String(sessionMinutes) + " min)");

  // Redirect the browser to "/", which will now render the dashboard
  // because appState has changed.
  server.sendHeader("Location", "/");
  server.send(303);
}

/************************************************************
   🔄 POST /new-session — resets everything, back to setup
************************************************************/
void handleNewSession() {
  resetSessionAccumulators();
  userName = "";
  userEmail = "";
  sessionMinutes = 0;
  appState = STATE_SETUP;

  showText("BRAIN CONTROL", "MOTOR SYSTEM");

  server.sendHeader("Location", "/");
  server.send(303);
}
