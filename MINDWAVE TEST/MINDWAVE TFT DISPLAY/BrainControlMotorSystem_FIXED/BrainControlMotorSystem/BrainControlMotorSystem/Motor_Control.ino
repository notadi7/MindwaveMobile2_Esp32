/************************************************************
   MOTOR_CONTROL.INO
   The ONLY place in the whole project that writes RELAY_PIN or
   ENA (relay/motor GPIO). Runs every loop() so a lost EEG
   signal, a GPIO13 press, an emergency stop, or a session end
   all cut power within one loop iteration, not on the next
   1-second telemetry tick (req. #14).

   Motor-on-time / attention accumulation (req. #32) happens
   here too, gated by the exact same "is this really a live,
   valid, active session" condition used for the relay/motor
   thresholds — so the numbers used for the result page can
   never disagree with what the motor actually did.
************************************************************/

void updateMotorControl(unsigned long now) {

  bool sessionLive = (systemState == STATE_ACTIVE_SESSION) && eegSignalValid;

  if (!sessionLive) {
    // Covers READY / NO_SIGNAL / SESSION_ENDED / EMERGENCY_STOP /
    // WIFI_COUNTDOWN / WIFI_PORTAL / SESSION_STARTED / signal-lost-
    // mid-session — motor and relay are forced off immediately in
    // every one of those cases (req. #5, #14, #19).
    digitalWrite(RELAY_PIN, HIGH);   // active-LOW relay -> OFF
    analogWrite(ENA, 0);
    motorRunning = false;
    motorSpeedPercent = 0;
    ledOn = false;
    return;
  }

  // ── Relay + LED logic (active LOW relay): ATT > 40 -> relay ON,
  //    dashboard LED ON. This is a separate, lower gate than the
  //    motor-speed threshold below — the LED/relay engage well
  //    before the motor is actually spinning. ──
  if (attention > 40) {
    digitalWrite(RELAY_PIN, LOW);
    ledOn = true;
  } else {
    digitalWrite(RELAY_PIN, HIGH);
    ledOn = false;
  }

  // ── Motor logic: ATT > 60 -> motor spins, mapped 50-70 PWM / 0-100% ──
  int speed = 0;
  motorSpeedPercent = 0;
  if (attention > 60) {
    speed = map(attention, 60, 100, 60, 80);
    motorSpeedPercent = map(speed, 60, 80, 0, 100);
  }
  analogWrite(ENA, speed);
  motorRunning = (speed > 0);

  // ── Session accumulation, once per second (matches the original
  //    cadence) — ONLY while sessionLive is true, i.e. never during
  //    READY / NO_SIGNAL / SESSION_ENDED / EMERGENCY_STOP (req. #32).
  if (now - lastAccumMillis >= 1000) {
    lastAccumMillis = now;

    if (attention > 60) motorOnSeconds++;   // motor-on time uses the motor threshold, not the relay one

    attentionSampleSum += attention;
    attentionSampleCount++;
    if (attention > attentionPeak) attentionPeak = attention;
  }
}
