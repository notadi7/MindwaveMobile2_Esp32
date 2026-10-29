/************************************************************
   SENSOR_STATE.INO
   Owns the distinction between "Bluetooth transport connected"
   and "valid EEG signal currently arriving" (req. #34), and is
   the ONLY place allowed to move systemState between
   STATE_SENSOR_CONNECTING / STATE_NO_SIGNAL / STATE_READY.

   It deliberately does NOT touch systemState while a session,
   WiFi setup, or emergency-stop sequence is in progress — those
   states own their own transitions (Session_Manager.ino /
   WiFi_Setup_Portal.ino). During STATE_ACTIVE_SESSION, losing
   EEG signal does NOT change systemState at all (req. #11); it
   only flips eegSignalValid, which TFT_Display.ino and
   Motor_Control.ino read directly.

   DEBOUNCE: a single noisy/high poorQuality reading (e.g. from
   a blink or brief head movement) must NOT instantly flip
   eegSignalValid to false — NeuroSky's poorQuality byte jitters
   on totally normal, momentary contact noise. eegSignalValid
   only goes false once poorQuality has stayed at/above
   POOR_QUALITY_THRESHOLD continuously for EEG_TIMEOUT_MS
   (2.5s). Recovering to good quality is still immediate — no
   reason to delay good news, only bad.
************************************************************/

unsigned long lastGoodPoorQualityMillis = 0;

void updateSensorState(unsigned long now) {
  bool wasValid = eegSignalValid;

  // EEG signal validity — debounced, see header comment above.
  if (poorQuality < POOR_QUALITY_THRESHOLD) {
    lastGoodPoorQualityMillis = now;
    eegSignalValid = true;
  } else {
    eegSignalValid = (now - lastGoodPoorQualityMillis) < EEG_TIMEOUT_MS;
  }

  // Req. #8 — never let a stale attention value linger once the
  // signal is considered invalid.
  if (!eegSignalValid) {
    attention = 0;
  }
  (void)wasValid; // reserved for future edge-triggered logic/logging

  switch (systemState) {

    case STATE_SENSOR_CONNECTING:
      if (!mindwaveConnected) return;   // still trying, nothing to do yet
      systemState = eegSignalValid ? STATE_READY : STATE_NO_SIGNAL;
      break;

    case STATE_NO_SIGNAL:
      if (!mindwaveConnected) {
        systemState = STATE_SENSOR_CONNECTING;
        break;
      }
      if (eegSignalValid) {
        systemState = STATE_READY;   // req. TEST3 — auto NO_SIGNAL -> READY
      }
      break;

    case STATE_READY:
      if (!mindwaveConnected) {
        systemState = STATE_SENSOR_CONNECTING;
        break;
      }
      if (!eegSignalValid) {
        systemState = STATE_NO_SIGNAL;
      }
      break;

    // BOOT / WIFI_CONNECTING: owned by setup(), nothing to do in loop().
    // WIFI_COUNTDOWN / WIFI_PORTAL / SESSION_STARTED / ACTIVE_SESSION /
    // SESSION_ENDED / EMERGENCY_STOP: owned by their own modules — this
    // function must never override them (req. #24's "ONE authoritative
    // rendering/state path" rule applies here too).
    default:
      break;
  }
}
