/************************************************************
   TELEMETRY.INO
   Runs once per second from loop(). Applies the relay/motor
   control logic, accumulates the session's focus-score /
   avg / peak attention, advances the countdown, builds and
   broadcasts the WebSocket JSON frame, and refreshes the OLED.
   This is the single biggest function in the project — it's
   kept as ONE function (matching the original inline block)
   rather than split further, since every line in it runs
   under the same "poor signal?" branch and splitting it apart
   would make that branching harder to follow, not easier.
************************************************************/
void telemetryTick() {
    if (poorQuality < 50) {
      signalLost = false;

      int speed = 0;
      int speedPercent = 0;

      // Motor/relay control logic only runs during an active session.
      // In STATE_SETUP / STATE_RESULT the motor is forced off below.
      if (appState == STATE_DASHBOARD) {

        // Relay logic (active LOW)
        if (attention > 40) {
          digitalWrite(RELAY_PIN, LOW);   // Relay ON
        } else {
          digitalWrite(RELAY_PIN, HIGH);  // Relay OFF
        }

        if (attention > 60) {
          speed = map(attention, 60, 100, 50, 70);
          speedPercent = map(speed, 50, 70, 0, 100);
        }

        analogWrite(ENA, speed);

        /******** SESSION ACCUMULATORS (once per second, matches this block's cadence) ********/
        if (attention > 60) {
       motorOnSeconds++;
       }
        attentionSampleSum += attention;
        attentionSampleCount++;
        if (attention > attentionPeak) attentionPeak = attention;

        /******** COUNTDOWN ********/
        if (sessionRemainingSeconds > 0) {
          sessionRemainingSeconds--;
        }
        if (sessionRemainingSeconds <= 0) {
          endSession();
        }

      } else {
        // Not in an active session: motor/relay stay off.
        digitalWrite(RELAY_PIN, HIGH);
        analogWrite(ENA, 0);
      }

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

deltaPercent = (delta * 100) / maxWave;
thetaPercent = (theta * 100) / maxWave;
alphaPercent = (alpha * 100) / maxWave;
betaPercent = (beta * 100) / maxWave;
gammaPercent = (gammaWave * 100) / maxWave;

deltaPercent = (deltaPercentOld * 0.7) + (deltaPercent * 0.3);
thetaPercent = (thetaPercentOld * 0.7) + (thetaPercent * 0.3);
alphaPercent = (alphaPercentOld * 0.7) + (alphaPercent * 0.3);
betaPercent = (betaPercentOld * 0.7) + (betaPercent * 0.3);
gammaPercent = (gammaPercentOld * 0.7) + (gammaPercent * 0.3);
      // Send live data to dashboard
String json = "{";
json += "\"attention\":" + String(attention) + ",";
json += "\"meditation\":" + String(meditation) + ",";
json += "\"delta\":" + String(deltaPercent) + ",";
json += "\"theta\":" + String(thetaPercent) + ",";
json += "\"alpha\":" + String(alphaPercent) + ",";
json += "\"beta\":" + String(betaPercent) + ",";
json += "\"gamma\":" + String(gammaPercent) + ",";
json += "\"signal\":" + String(100 - poorQuality) + ",";
json += "\"connected\":1,";
json += "\"speed\":" + String(speedPercent) + ",";
json += "\"relay\":" + String(digitalRead(RELAY_PIN) == LOW ? 1 : 0) + ",";
json += "\"state\":" + String((int)appState) + ",";
json += "\"remaining\":" + String(sessionRemainingSeconds) + ",";
json += "\"motorOnSeconds\":" + String(motorOnSeconds) + ",";
json += "\"avgAttention\":" + String(attentionSampleCount ? (attentionSampleSum / attentionSampleCount) : 0) + ",";
json += "\"peakAttention\":" + String(attentionPeak) + ",";
json += "\"sessionEnded\":" + String(appState == STATE_RESULT ? 1 : 0);
json += "}";
Serial.println(json);
webSocket.broadcastTXT(json);

// Save current values for next smoothing cycle
deltaPercentOld = deltaPercent;
thetaPercentOld = thetaPercent;
alphaPercentOld = alphaPercent;
betaPercentOld = betaPercent;
gammaPercentOld = gammaPercent;

      /******** SERIAL ********/
      Serial.println("====== DATA ======");
      Serial.print("Attention: ");
      Serial.print(attention);
      Serial.println(" %");

      Serial.print("Motor Speed: ");
      Serial.print(speedPercent);
      Serial.println(" %");
      Serial.print("LED Status: ");
if (digitalRead(RELAY_PIN) == LOW) {
  Serial.println("ON");
} else {
  Serial.println("OFF");
}

      Serial.print("IP: ");
      Serial.println(ipAddress);
      Serial.println("==================");

      /******** OLED ********/
      display.clearDisplay();
      display.setTextSize(1);

      drawCenteredText("Brain Control System", 0);
      drawCenteredText("Attention: " + String(attention) + " %", 18);
      drawCenteredText("Motor: " + String(speedPercent) + " %", 34);
      drawCenteredText("IP: " + ipAddress, 52);

      display.display();

    } else {

  if (!signalLost) {
    signalLost = true;
    signalLostTime = millis();
  }

  // only show no signal if lost for 3 sec
  if (millis() - signalLostTime > 3000) {

    digitalWrite(RELAY_PIN, HIGH);
    analogWrite(ENA, 0);
    String json = "{";
json += "\"attention\":0,";
json += "\"meditation\":0,";
json += "\"speed\":0,";
json += "\"relay\":0,";
json += "\"delta\":0,";
json += "\"theta\":0,";
json += "\"alpha\":0,";
json += "\"beta\":0,";
json += "\"gamma\":0,";
json += "\"signal\":0,";
json += "\"connected\":0,";
json += "\"state\":" + String((int)appState) + ",";
json += "\"remaining\":" + String(sessionRemainingSeconds) + ",";
json += "\"motorOnSeconds\":" + String(motorOnSeconds) + ",";
json += "\"avgAttention\":" + String(attentionSampleCount ? (attentionSampleSum / attentionSampleCount) : 0) + ",";
json += "\"peakAttention\":" + String(attentionPeak) + ",";
json += "\"sessionEnded\":" + String(appState == STATE_RESULT ? 1 : 0);
json += "}";

webSocket.broadcastTXT(json);
Serial.println(json);
    Serial.println("NO SIGNAL - WEAR PROPERLY");

    display.clearDisplay();
    display.setTextSize(1);

    drawCenteredText("NO SIGNAL", 20);
    drawCenteredText("Wear Properly", 40);
    drawCenteredText("IP: " + ipAddress, 55);

    display.display();
  }
}
}
