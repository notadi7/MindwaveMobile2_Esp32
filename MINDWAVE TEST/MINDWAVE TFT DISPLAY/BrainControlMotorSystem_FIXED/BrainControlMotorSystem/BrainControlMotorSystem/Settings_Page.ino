/************************************************************
   SETTINGS PAGE / API
   Local device configuration: WiFi + Gmail + hardware pin map.
************************************************************/

String jsonEscape(String v) {
  v.replace("\\", "\\\\");
  v.replace("\"", "\\\"");
  v.replace("\r", "\\r");
  v.replace("\n", "\\n");
  return v;
}

void handleSettingsData() {
  String json = "{";
  json += "\"wifiSsid\":\"" + jsonEscape(wifiSsid) + "\",";
  json += "\"wifiPass\":\"" + jsonEscape(wifiPassword) + "\",";
  json += "\"gmail\":\"" + jsonEscape(smtpUser) + "\",";
  json += "\"gmailPass\":\"" + jsonEscape(smtpPass) + "\",";
  json += "\"pins\":{";
  json += "\"GPIO13_WIFI_CREDENTIAL\":13,";
  json += "\"MOTOR_IN1\":26,";
  json += "\"MOTOR_IN2\":27,";
  json += "\"MOTOR_ENA\":14,";
  json += "\"RELAY\":25";
  json += "}}";
  server.send(200, "application/json", json);
}

void handleSettingsSave() {
  String newGmail = server.arg("gmail");
  String newGmailPass = server.arg("gmailPass");

  if (newGmail.length() == 0 || newGmailPass.length() == 0) {
    server.send(400, "text/plain", "Gmail and Gmail App Password are required.");
    return;
  }

  saveSystemCredentials(newGmail, newGmailPass);
  server.send(200, "text/plain", "Settings saved.");
}

void handleWifiSetupFromDashboard() {
  // Zero-latency safety cutoff, same as the GPIO13 path (req. #5) —
  // this button can be pressed mid-session too.
  digitalWrite(RELAY_PIN, HIGH);
  analogWrite(ENA, 0);
  startWifiConfigPortal();
  server.send(200, "text/plain", "WiFi setup mode started. Connect to MindWave-Setup.");
}
