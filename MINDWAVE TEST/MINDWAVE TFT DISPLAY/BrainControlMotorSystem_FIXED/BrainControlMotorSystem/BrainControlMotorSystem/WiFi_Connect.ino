/************************************************************
   WIFI_CONNECT.INO
   One-shot WiFi connect attempt, called once from setup(),
   BEFORE loop() (and therefore updateTFT()) ever runs. The
   showText() calls in here are the one deliberate exception to
   "TFT_Display.ino::updateTFT() is the only render path"
   (req. #24): that rule exists to stop concurrent modules from
   fighting over the screen once the state machine is running,
   which cannot happen here since nothing else executes until
   this function returns and setup() continues.
************************************************************/

/************************************************************
   📶 WIFI CONNECTION FUNCTION
************************************************************/
void connectWiFi() {

  // True first boot, nothing ever saved to NVS and no fallback
  // set in the sketch either — skip straight to setup mode
  // instead of trying to connect with an empty SSID.
  if (wifiSsid.length() == 0) {
    Serial.println("No WiFi credentials found, opening setup portal...");
    startWifiConfigPortal();
    return;
  }

  showText("Connecting", "WiFi...");
  Serial.println("Connecting WiFi...");

  WiFi.begin(wifiSsid.c_str(), wifiPassword.c_str());

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
