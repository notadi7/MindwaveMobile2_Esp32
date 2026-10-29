/************************************************************
   WIFI_CONNECT.INO
   One-shot WiFi connect attempt, called once from setup().
************************************************************/

/************************************************************
   📶 WIFI CONNECTION FUNCTION
************************************************************/
void connectWiFi() {

  showText("Connecting", "WiFi...");
  Serial.println("Connecting WiFi...");

  WiFi.begin(ssid, password);

  int timeout = 0;

  while (WiFi.status() != WL_CONNECTED && timeout < 10) {
    delay(500);
    Serial.print(".");
    timeout++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    ipAddress = WiFi.localIP().toString();

    Serial.println("\nWiFi Connected!");
    Serial.println(ipAddress);

    showText("WiFi Connected", "IP:", ipAddress);
    delay(2000);

  } else {
    wifiConnected = false;
    ipAddress = "NA";

    Serial.println("\nWiFi Failed!");

    showText("WiFi Failed", "Offline Mode");
    delay(2000);
  }
}
