/************************************************************
   TELEMETRY.INO
   Builds and broadcasts the WebSocket JSON telemetry frame on
   its OWN timer (TELEMETRY_INTERVAL_MS). This file no longer
   touches the relay/motor (Motor_Control.ino owns that), the
   session countdown (Session_Manager.ino owns that), or the
   TFT (TFT_Display.ino owns that) — it only reads state and
   reports it (req. #13, #24, #28).
************************************************************/

unsigned long lastTelemetryMillis = 0;

const char* systemStateName(SystemState s) {
  switch (s) {
    case STATE_BOOT:              return "BOOT";
    case STATE_WIFI_CONNECTING:   return "WIFI_CONNECTING";
    case STATE_SENSOR_CONNECTING: return "SENSOR_CONNECTING";
    case STATE_NO_SIGNAL:         return "NO_SIGNAL";
    case STATE_READY:             return "READY";
    case STATE_WIFI_COUNTDOWN:    return "WIFI_COUNTDOWN";
    case STATE_WIFI_PORTAL:       return "WIFI_PORTAL";
    case STATE_SESSION_STARTED:   return "SESSION_STARTED";
    case STATE_ACTIVE_SESSION:    return "ACTIVE_SESSION";
    case STATE_SESSION_ENDED:     return "SESSION_ENDED";
    case STATE_EMERGENCY_STOP:    return "EMERGENCY_STOP";
  }
  return "UNKNOWN";
}

void updateTelemetry(unsigned long now) {
  if (now - lastTelemetryMillis < TELEMETRY_INTERVAL_MS) return;
  lastTelemetryMillis = now;
  broadcastTelemetry();
}

/************************************************************
   Recomputes the smoothed brainwave percentages (display-only
   — never used for motor control) and broadcasts one JSON
   telemetry frame. Callable directly (e.g. from handleEmergencyStop())
   when a state change needs to reach the browser immediately
   rather than waiting for the next timer tick.
************************************************************/
void broadcastTelemetry() {
  deltaSmooth = (deltaSmooth * 0.8) + (delta * 0.2);
  thetaSmooth = (thetaSmooth * 0.8) + (theta * 0.2);
  alphaSmooth = (alphaSmooth * 0.8) + (alpha * 0.2);
  betaSmooth  = (betaSmooth * 0.8) + (beta * 0.2);
  gammaSmooth = (gammaSmooth * 0.8) + (gammaWave * 0.2);

  unsigned long maxWave = delta;
  if (theta > maxWave) maxWave = theta;
  if (alpha > maxWave) maxWave = alpha;
  if (beta > maxWave) maxWave = beta;
  if (gammaWave > maxWave) maxWave = gammaWave;

  if (maxWave == 0 || !eegSignalValid) {
    // Guards the original code's implicit divide-by-zero when no
    // brainwave packets have arrived yet, and enforces req. #8 (no
    // stale/invalid brainwave data displayed once signal is lost).
    deltaPercent = thetaPercent = alphaPercent = betaPercent = gammaPercent = 0;
  } else {
    int dP = (delta * 100) / maxWave;
    int tP = (theta * 100) / maxWave;
    int aP = (alpha * 100) / maxWave;
    int bP = (beta * 100) / maxWave;
    int gP = (gammaWave * 100) / maxWave;

    deltaPercent = (deltaPercentOld * 0.7) + (dP * 0.3);
    thetaPercent = (thetaPercentOld * 0.7) + (tP * 0.3);
    alphaPercent = (alphaPercentOld * 0.7) + (aP * 0.3);
    betaPercent  = (betaPercentOld  * 0.7) + (bP * 0.3);
    gammaPercent = (gammaPercentOld * 0.7) + (gP * 0.3);
  }

  deltaPercentOld = deltaPercent;
  thetaPercentOld = thetaPercent;
  alphaPercentOld = alphaPercent;
  betaPercentOld  = betaPercent;
  gammaPercentOld = gammaPercent;

  int avgAttNow = attentionSampleCount ? (int)(attentionSampleSum / attentionSampleCount) : 0;
  int signalQuality = 100 - (int)poorQuality;
  if (signalQuality < 0) signalQuality = 0;
  if (signalQuality > 100) signalQuality = 100;

  String json = "{";
  json += "\"state\":\"" + String(systemStateName(systemState)) + "\",";
  json += "\"sensorConnected\":" + String(mindwaveConnected ? 1 : 0) + ",";
  json += "\"eegValid\":" + String(eegSignalValid ? 1 : 0) + ",";
  json += "\"signalQuality\":" + String(signalQuality) + ",";
  json += "\"attention\":" + String(eegSignalValid ? attention : 0) + ",";
  json += "\"meditation\":" + String(eegSignalValid ? meditation : 0) + ",";
  json += "\"delta\":" + String(deltaPercent) + ",";
  json += "\"theta\":" + String(thetaPercent) + ",";
  json += "\"alpha\":" + String(alphaPercent) + ",";
  json += "\"beta\":" + String(betaPercent) + ",";
  json += "\"gamma\":" + String(gammaPercent) + ",";
  json += "\"motor\":" + String(motorRunning ? 1 : 0) + ",";
  json += "\"led\":" + String(ledOn ? 1 : 0) + ",";
  json += "\"motorSpeed\":" + String(motorSpeedPercent) + ",";
  json += "\"remaining\":" + String(sessionRemainingSeconds) + ",";
  json += "\"motorOnSeconds\":" + String(motorOnSeconds) + ",";
  json += "\"avgAttention\":" + String(avgAttNow) + ",";
  json += "\"peakAttention\":" + String(attentionPeak) + ",";
  json += "\"sessionEnded\":" + String(systemState == STATE_SESSION_ENDED ? 1 : 0);
  json += "}";

  webSocket.broadcastTXT(json);
}
