/************************************************************
   🧠 BRAIN CONTROL MOTOR SYSTEM
   ESP32 + MindWave + L293N + OLED (SH1106) + WiFi
************************************************************/

#include "BluetoothSerial.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <WiFi.h>

/******************** WIFI *******************************/
const char* ssid = "sybau";
const char* password = "12345678";
bool wifiConnected = false;
String ipAddress = "NA";
#include <WebServer.h>
WebServer server(80);  

/******************** WEBSOCKET ************************/
#include <WebSocketsServer.h>
WebSocketsServer webSocket = WebSocketsServer(81);
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);
/******************** OBJECTS ********************/
BluetoothSerial SerialBT;
Adafruit_SH1106G display = Adafruit_SH1106G(128, 64, &Wire);

/******************** BLUETOOTH ************************/
uint8_t address[6] = {0xC4, 0x64, 0xE3, 0xE6, 0xEA, 0x9A};
bool connected = false;

/******************** NEUROSKY DATA ********************/
byte payloadData[64];
int payloadLength = 0;
byte generatedChecksum = 0;
byte checksum = 0;

byte attention = 0;
byte meditation = 0;
byte poorQuality = 200;

/******************** BRAINWAVE VARIABLES ************************/
unsigned long delta = 0;
unsigned long theta = 0;
unsigned long alpha = 0;
unsigned long beta = 0;
unsigned long gammaWave = 0;
int alphaPercent = 0;
int betaPercent = 0;
int thetaPercent = 0;
int gammaPercent = 0;
int deltaPercent = 0;

float deltaSmooth = 0;
float thetaSmooth = 0;
float alphaSmooth = 0;
float betaSmooth = 0;
float gammaSmooth = 0;

int deltaPercentOld = 0;
int thetaPercentOld = 0;
int alphaPercentOld = 0;
int betaPercentOld = 0;
int gammaPercentOld = 0;
/******************** MOTOR PINS **************************/
#define IN1 26
#define IN2 27
#define ENA 14
#define RELAY_PIN 25

/******************** TIMING *******************************/
unsigned long lastPrintTime = 0;

/************************************************************
   📥 READ ONE BYTE FROM BLUETOOTH
************************************************************/
byte ReadOneByte() {
  while (!SerialBT.available());
  return SerialBT.read();
}

/************************************************************
   🖥️ SIMPLE TEXT DISPLAY
************************************************************/
void showText(String line1, String line2 = "", String line3 = "") {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);

  display.setCursor(0, 10);
  display.println(line1);

  display.setCursor(0, 30);
  display.println(line2);

  display.setCursor(0, 50);
  display.println(line3);

  display.display();
}

/************************************************************
   🖥️ CENTER TEXT FUNCTION
************************************************************/
void drawCenteredText(String text, int y) {
  int16_t x1, y1;
  uint16_t w, h;

  display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  int x = (128 - w) / 2;

  display.setCursor(x, y);
  display.print(text);
}

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

/************************************************************
   ⚙️ SETUP
************************************************************/
void setup() {
  Serial.begin(115200);

  // OLED INIT
  display.begin(0x3C, true);
  display.clearDisplay();

  // Project Intro
  showText("BRAIN CONTROL", "MOTOR SYSTEM");
  delay(2000);

  // MOTOR SETUP
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENA, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);   // Active LOW relay OFF initially
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);

  // BLUETOOTH SETUP
  SerialBT.begin("ESP32_Master", true);
  SerialBT.setPin("0000");

  showText("Connecting to", "MindWave...");
  delay(2000);

  connected = SerialBT.connect(address);

  if (connected) {
    showText("Connected!");
    delay(1500);
  } else {
    showText("Failed!");
  }

  // 📶 CONNECT WIFI
  connectWiFi();
  server.on("/", handleRoot);   // when user opens IP
server.begin();

webSocket.begin();
webSocket.onEvent(webSocketEvent);

Serial.println("Web server started");
Serial.print("Open in browser: ");
Serial.println(ipAddress);
}


/************************************************************
   🔁 LOOP
************************************************************/
void loop() {
  server.handleClient();
  webSocket.loop();
  /******** RECONNECT IF DISCONNECTED ********/
  if (!connected) {
    showText("Reconnecting...");
    connected = SerialBT.connect(address);
    delay(3000);
    return;
  }

  /******** PARSE NEUROSKY DATA ********/
  if (ReadOneByte() == 0xAA) {
    if (ReadOneByte() == 0xAA) {

      payloadLength = ReadOneByte();
      if (payloadLength > 64) return;

      generatedChecksum = 0;

      for (int i = 0; i < payloadLength; i++) {
        payloadData[i] = ReadOneByte();
        generatedChecksum += payloadData[i];
      }

      checksum = ReadOneByte();
      generatedChecksum = 255 - generatedChecksum;

      if (checksum == generatedChecksum) {

        for (int i = 0; i < payloadLength; i++) {

          switch (payloadData[i]) {

            case 2:
              poorQuality = payloadData[++i];
              break;

            case 4:
              attention = payloadData[++i];
              break;

            case 5:
              meditation = payloadData[++i];
              break;

            case 0x80:
              i += 3;
              break;

          case 0x83:
{
  i++;

  delta = ((unsigned long)payloadData[i] << 16) |
          ((unsigned long)payloadData[i+1] << 8) |
          payloadData[i+2];
  i += 3;

  theta = ((unsigned long)payloadData[i] << 16) |
          ((unsigned long)payloadData[i+1] << 8) |
          payloadData[i+2];
  i += 3;

  unsigned long lowAlpha =
    ((unsigned long)payloadData[i] << 16) |
    ((unsigned long)payloadData[i+1] << 8) |
    payloadData[i+2];
  i += 3;

  unsigned long highAlpha =
    ((unsigned long)payloadData[i] << 16) |
    ((unsigned long)payloadData[i+1] << 8) |
    payloadData[i+2];
  i += 3;

  unsigned long lowBeta =
    ((unsigned long)payloadData[i] << 16) |
    ((unsigned long)payloadData[i+1] << 8) |
    payloadData[i+2];
  i += 3;

  unsigned long highBeta =
    ((unsigned long)payloadData[i] << 16) |
    ((unsigned long)payloadData[i+1] << 8) |
    payloadData[i+2];
  i += 3;

  unsigned long lowGamma =
    ((unsigned long)payloadData[i] << 16) |
    ((unsigned long)payloadData[i+1] << 8) |
    payloadData[i+2];
  i += 3;

  unsigned long midGamma =
    ((unsigned long)payloadData[i] << 16) |
    ((unsigned long)payloadData[i+1] << 8) |
    payloadData[i+2];

  alpha = lowAlpha + highAlpha;
  beta  = lowBeta + highBeta;
  gammaWave = lowGamma + midGamma;

  break;
}
          }
        }
      }
    }
  }

  /******** UPDATE EVERY 1 SECOND ********/
  if (millis() - lastPrintTime >= 1000) {
    lastPrintTime = millis();

    if (poorQuality == 0) {

      int speed = 0;
      int speedPercent = 0;
       // Relay logic (active LOW)
       
        if (attention > 40) {
         digitalWrite(RELAY_PIN, LOW);   // Relay ON
          } else {
         digitalWrite(RELAY_PIN, HIGH);  // Relay OFF
  }


      if (attention > 60) {
        speed = map(attention, 60, 100, 50, 150);
        speedPercent = map(speed, 50, 150, 0, 100);
      }

      analogWrite(ENA, speed);
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
json += "\"signal\":" + String(100 - poorQuality);
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
      digitalWrite(RELAY_PIN, HIGH); // Relay OFF
      analogWrite(ENA, 0);

      Serial.println("❌ NO SIGNAL - WEAR PROPERLY");

      display.clearDisplay();
      display.setTextSize(1);

      drawCenteredText("NO SIGNAL", 20);
      drawCenteredText("Wear Properly", 40);
      drawCenteredText("IP: " + ipAddress, 55);

      display.display();
    }
  }
}
/******** HANDLE ROOTS ********/
void handleRoot() {

String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>MindaWave - Attention Monitor</title>
<link href="https://fonts.googleapis.com/css2?family=Orbitron:wght@400;700;900&family=Rajdhani:wght@400;500;600&display=swap" rel="stylesheet">
<style>
  :root {
    --bg: #06080f;
    --panel: #0c1220;
    --panel2: #0e1628;
    --border: rgba(0,200,255,0.12);
    --accent: #00c8ff;
    --green: #39ff6a;
    --red: #ff3b5c;
    --orange: #ff8c00;
    --yellow: #ffe600;
    --purple: #bf5fff;
    --text: #e0f0ff;
    --muted: #607090;
    --glow: 0 0 18px rgba(0,200,255,0.3);
  }

  * { margin:0; padding:0; box-sizing:border-box; }

  body {
    font-family: 'Rajdhani', sans-serif;
    background: var(--bg);
    color: var(--text);
    min-height: 100vh;
    overflow-x: hidden;
  }

  /* ── HEADER ── */
  .header {
    display: flex;
    align-items: center;
    justify-content: space-between;
    padding: 12px 28px;
    background: rgba(10,16,30,0.95);
    border-bottom: 1px solid var(--border);
    backdrop-filter: blur(10px);
  }

  .logo {
    display: flex;
    align-items: center;
    gap: 12px;
  }

  .logo svg { width:36px; height:36px; }

  .logo-text h1 {
    font-family: 'Orbitron', monospace;
    font-size: 18px;
    font-weight: 900;
    letter-spacing: 3px;
    color: var(--accent);
    text-shadow: 0 0 20px var(--accent);
  }

  .logo-text p {
    font-size: 10px;
    letter-spacing: 4px;
    color: var(--accent);
    opacity: 0.7;
    margin-top: 1px;
  }

  .header-center {
    font-family: 'Orbitron', monospace;
    font-size: 22px;
    font-weight: 700;
    color: white;
    letter-spacing: 2px;
  }

  .header-right {
    display: flex;
    align-items: center;
    gap: 20px;
    font-size: 13px;
  }

  .status-badge {
    display: flex;
    align-items: center;
    gap: 6px;
    padding: 4px 10px;
    border: 1px solid var(--border);
    border-radius: 20px;
    background: rgba(0,200,255,0.05);
    font-family: 'Orbitron', monospace;
    font-size: 11px;
    letter-spacing: 1px;
  }

  .battery-bar {
    display: flex;
    align-items: center;
    gap: 8px;
    padding: 4px 12px;
    border: 1px solid var(--border);
    border-radius: 20px;
    background: rgba(57,255,106,0.05);
    font-family: 'Orbitron', monospace;
    font-size: 11px;
    color: var(--green);
  }

  /* ── MAIN GRID ── */
  .main {
    display: grid;
    grid-template-columns: 300px 1fr 280px;
    gap: 16px;
    padding: 16px 20px;
    min-height: calc(100vh - 120px);
  }

  /* ── CARDS ── */
  .card {
    background: var(--panel);
    border: 1px solid var(--border);
    border-radius: 16px;
    padding: 18px;
    position: relative;
    overflow: hidden;
  }

  .card::before {
    content: '';
    position: absolute;
    top: 0; left: 0; right: 0;
    height: 1px;
    background: linear-gradient(90deg, transparent, var(--accent), transparent);
    opacity: 0.4;
  }

  .card-title {
    font-family: 'Orbitron', monospace;
    font-size: 11px;
    letter-spacing: 3px;
    color: var(--muted);
    margin-bottom: 14px;
    display: flex;
    align-items: center;
    justify-content: space-between;
  }

  .card-title svg { opacity: 0.6; }

  /* ── BRAINWAVE ROWS ── */
  .wave-row {
    margin: 10px 0;
  }

  .wave-header {
    display: flex;
    align-items: center;
    justify-content: space-between;
    margin-bottom: 6px;
  }

  .wave-label {
    display: flex;
    align-items: center;
    gap: 8px;
  }

  .wave-dot {
    width: 8px; height: 8px;
    border-radius: 50%;
    box-shadow: 0 0 8px currentColor;
  }

  .wave-name {
    font-weight: 600;
    font-size: 14px;
    letter-spacing: 1px;
  }

  .wave-hz {
    font-size: 10px;
    color: var(--muted);
    margin-top: 1px;
  }

  .wave-pct {
    font-family: 'Orbitron', monospace;
    font-size: 13px;
    font-weight: 700;
  }

  .wave-mini {
    height: 30px;
    width: 80px;
  }

  .wave-track {
    flex: 1;
    height: 4px;
    background: rgba(255,255,255,0.06);
    border-radius: 4px;
    overflow: hidden;
    margin: 0 8px;
  }

  .wave-fill {
    height: 100%;
    width: 0%;
    border-radius: 4px;
    transition: width 0.6s cubic-bezier(0.4,0,0.2,1);
  }

  /* ── SIGNAL QUALITY ── */
  .signal-card {
    margin-top: 14px;
  }

  .seg-bar {
    display: flex;
    gap: 3px;
    margin-top: 8px;
  }

  .seg {
    flex: 1;
    height: 10px;
    border-radius: 3px;
    background: rgba(255,255,255,0.06);
    transition: background 0.5s;
  }

  .seg.active { background: var(--green); box-shadow: 0 0 6px var(--green); }

  .sig-pct {
    font-family: 'Orbitron', monospace;
    font-size: 13px;
    color: var(--green);
  }

  /* ── CENTER ── */
  .center-col { display: flex; flex-direction: column; gap: 16px; }

  .attention-card {
    flex: 0 0 auto;
  }

  .gauge-wrap {
    display: flex;
    flex-direction: column;
    align-items: center;
    position: relative;
  }

  #attentionGauge {
    width: 100%;
    max-width: 340px;
  }

  .attention-value {
    font-family: 'Orbitron', monospace;
    font-size: 62px;
    font-weight: 900;
    color: var(--green);
    text-shadow: 0 0 40px var(--green);
    line-height: 1;
    margin-top: -30px;
  }

  .attention-sub {
    font-size: 13px;
    color: var(--muted);
    margin-top: 4px;
    letter-spacing: 1px;
  }

  .focus-badge {
    margin-top: 14px;
    display: inline-flex;
    align-items: center;
    gap: 8px;
    padding: 8px 22px;
    border: 2px solid var(--green);
    border-radius: 30px;
    background: rgba(57,255,106,0.08);
    font-family: 'Orbitron', monospace;
    font-size: 14px;
    font-weight: 700;
    color: var(--green);
    text-shadow: 0 0 10px var(--green);
    letter-spacing: 2px;
  }

  .trend-card { flex: 1; }

  #trendGraph {
    width: 100%;
    height: 120px;
  }

  .trend-labels {
    display: flex;
    justify-content: space-between;
    font-size: 10px;
    color: var(--muted);
    margin-top: 4px;
    letter-spacing: 1px;
  }

  /* ── RIGHT PANEL ── */
  .right-col { display: flex; flex-direction: column; gap: 16px; }

  .brain-card {
    text-align: center;
  }

  .brain-img {
    width: 120px;
    height: 100px;
    margin: 10px auto;
    position: relative;
  }

  /* Animated brain SVG placeholder */
  .brain-svg {
    width: 100%;
    height: 100%;
  }

  .state-row {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 8px 0;
    border-bottom: 1px solid rgba(255,255,255,0.04);
    font-size: 14px;
    font-weight: 500;
  }

  .state-row:last-child { border-bottom: none; }

  .state-label { color: var(--muted); }

  .state-val {
    font-family: 'Orbitron', monospace;
    font-size: 12px;
    font-weight: 700;
    letter-spacing: 1px;
  }

  .val-high { color: var(--green); text-shadow: 0 0 8px var(--green); }
  .val-low { color: var(--accent); text-shadow: 0 0 8px var(--accent); }
  .val-med { color: var(--yellow); text-shadow: 0 0 8px var(--yellow); }

  /* ── SYSTEM STATUS ── */
  .sys-item {
    display: flex;
    align-items: center;
    gap: 12px;
    padding: 10px 0;
    border-bottom: 1px solid rgba(255,255,255,0.04);
  }

  .sys-item:last-child { border-bottom: none; }

  .sys-icon {
    width: 38px; height: 38px;
    border-radius: 10px;
    display: flex;
    align-items: center;
    justify-content: center;
    background: rgba(0,200,255,0.08);
    border: 1px solid var(--border);
    font-size: 18px;
  }

  .sys-info { flex: 1; }
  .sys-name { font-weight: 600; font-size: 14px; }
  .sys-status { font-size: 11px; color: var(--green); letter-spacing: 1px; margin-top: 1px; }
  .sys-status.na { color: var(--muted); }

  .sys-check {
    width: 22px; height: 22px;
    border-radius: 50%;
    background: rgba(57,255,106,0.15);
    border: 2px solid var(--green);
    display: flex;
    align-items: center;
    justify-content: center;
    color: var(--green);
    font-size: 13px;
  }

  /* ── BOTTOM NAV ── */
  .bottom-nav {
    display: flex;
    gap: 10px;
    padding: 12px 20px;
    background: rgba(10,16,30,0.95);
    border-top: 1px solid var(--border);
  }

  .nav-btn {
    display: flex;
    align-items: center;
    gap: 8px;
    padding: 10px 24px;
    border-radius: 30px;
    border: 1px solid var(--border);
    background: transparent;
    color: var(--muted);
    font-family: 'Orbitron', monospace;
    font-size: 11px;
    letter-spacing: 2px;
    cursor: pointer;
    transition: all 0.3s;
  }

  .nav-btn.active {
    background: rgba(0,200,255,0.12);
    border-color: var(--accent);
    color: var(--accent);
    box-shadow: 0 0 20px rgba(0,200,255,0.15);
  }

  .nav-btn:hover:not(.active) {
    border-color: rgba(0,200,255,0.3);
    color: var(--text);
  }

  /* ── SCAN LINES ── */
  body::after {
    content: '';
    position: fixed;
    inset: 0;
    background: repeating-linear-gradient(
      0deg,
      transparent,
      transparent 2px,
      rgba(0,0,0,0.03) 2px,
      rgba(0,0,0,0.03) 4px
    );
    pointer-events: none;
    z-index: 9999;
  }

  /* corner decorations */
  .card-corner {
    position: absolute;
    width: 12px; height: 12px;
    border-color: var(--accent);
    border-style: solid;
    opacity: 0.5;
  }
  .card-corner.tl { top: 8px; left: 8px; border-width: 2px 0 0 2px; }
  .card-corner.tr { top: 8px; right: 8px; border-width: 2px 2px 0 0; }
  .card-corner.bl { bottom: 8px; left: 8px; border-width: 0 0 2px 2px; }
  .card-corner.br { bottom: 8px; right: 8px; border-width: 0 2px 2px 0; }

  /* pulse animation */
  @keyframes pulse { 0%,100%{opacity:1} 50%{opacity:0.4} }
  .pulsing { animation: pulse 2s infinite; }
</style>
</head>
<body>

<!-- HEADER -->
<div class="header">
  <div class="logo">
    <svg viewBox="0 0 36 36" fill="none" xmlns="http://www.w3.org/2000/svg">
      <path d="M4 18 Q8 8 12 18 Q16 28 20 18 Q24 8 28 18 Q32 28 36 18" stroke="#00c8ff" stroke-width="2.5" fill="none" stroke-linecap="round"/>
      <circle cx="4" cy="18" r="2" fill="#00c8ff" opacity="0.6"/>
      <circle cx="36" cy="18" r="2" fill="#00c8ff" opacity="0.6"/>
    </svg>
    <div class="logo-text">
      <h1>MINDAWAVE</h1>
      <p>ATTENTION MONITOR</p>
    </div>
  </div>
  <div class="header-center" id="clock">--:--:--</div>
  <div class="header-right">
    <div class="status-badge">
      <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="#00c8ff" stroke-width="2"><path d="M5 12.55a11 11 0 0 1 14.08 0"/><path d="M1.42 9a16 16 0 0 1 21.16 0"/><path d="M8.53 16.11a6 6 0 0 1 6.95 0"/><circle cx="12" cy="20" r="1" fill="#00c8ff"/></svg>
      WiFi
    </div>
    <div class="status-badge">
      <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="#00c8ff" stroke-width="2"><path d="M6.5 6.5A7 7 0 0 0 5 10a7 7 0 0 0 7 7 7 7 0 0 0 3.5-.9"/><path d="M6.5 6.5A7 7 0 0 1 17.5 17.5"/><path d="M12 2v4M12 18v4M2 12h4M18 12h4"/></svg>
      BT
    </div>
    <div class="status-badge">⬡ ESP32</div>
    <div class="battery-bar">
       <span id="battPct">85%</span>
    </div>
  </div>
</div>

<!-- MAIN -->
<div class="main">

  <!-- LEFT COLUMN -->
  <div style="display:flex;flex-direction:column;gap:14px;">

    <div class="card">
      <div class="card-corner tl"></div><div class="card-corner tr"></div>
      <div class="card-corner bl"></div><div class="card-corner br"></div>
      <div class="card-title">
        BRAINWAVE
        <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polyline points="22 12 18 12 15 21 9 3 6 12 2 12"/></svg>
      </div>

      <!-- Delta -->
      <div class="wave-row">
        <div class="wave-header">
          <div class="wave-label">
            <span class="wave-dot" style="color:#4dd9ff;background:#4dd9ff;"></span>
            <div>
              <div class="wave-name" style="color:#4dd9ff;">DELTA</div>
              <div class="wave-hz">0.5-4 Hz</div>
            </div>
          </div>
          <canvas class="wave-mini" id="wDelta"></canvas>
          <div class="wave-pct" id="pDelta" style="color:#4dd9ff;">0%</div>
        </div>
        <div class="wave-track"><div class="wave-fill" id="bDelta" style="background:#4dd9ff;box-shadow:0 0 8px #4dd9ff;"></div></div>
      </div>

      <!-- Theta -->
      <div class="wave-row">
        <div class="wave-header">
          <div class="wave-label">
            <span class="wave-dot" style="color:#00e5d4;background:#00e5d4;"></span>
            <div>
              <div class="wave-name" style="color:#00e5d4;">THETA</div>
              <div class="wave-hz">4-8 Hz</div>
            </div>
          </div>
          <canvas class="wave-mini" id="wTheta"></canvas>
          <div class="wave-pct" id="pTheta" style="color:#00e5d4;">0%</div>
        </div>
        <div class="wave-track"><div class="wave-fill" id="bTheta" style="background:#00e5d4;box-shadow:0 0 8px #00e5d4;"></div></div>
      </div>

      <!-- Alpha -->
      <div class="wave-row">
        <div class="wave-header">
          <div class="wave-label">
            <span class="wave-dot" style="color:#39ff6a;background:#39ff6a;"></span>
            <div>
              <div class="wave-name" style="color:#39ff6a;">ALPHA</div>
              <div class="wave-hz">8-12 Hz</div>
            </div>
          </div>
          <canvas class="wave-mini" id="wAlpha"></canvas>
          <div class="wave-pct" id="pAlpha" style="color:#39ff6a;">0%</div>
        </div>
        <div class="wave-track"><div class="wave-fill" id="bAlpha" style="background:#39ff6a;box-shadow:0 0 8px #39ff6a;"></div></div>
      </div>

      <!-- Beta -->
      <div class="wave-row">
        <div class="wave-header">
          <div class="wave-label">
            <span class="wave-dot" style="color:#ffe600;background:#ffe600;"></span>
            <div>
              <div class="wave-name" style="color:#ffe600;">BETA</div>
              <div class="wave-hz">12-30 Hz</div>
            </div>
          </div>
          <canvas class="wave-mini" id="wBeta"></canvas>
          <div class="wave-pct" id="pBeta" style="color:#ffe600;">0%</div>
        </div>
        <div class="wave-track"><div class="wave-fill" id="bBeta" style="background:#ffe600;box-shadow:0 0 8px #ffe600;"></div></div>
      </div>

      <!-- Gamma -->
      <div class="wave-row">
        <div class="wave-header">
          <div class="wave-label">
            <span class="wave-dot" style="color:#bf5fff;background:#bf5fff;"></span>
            <div>
              <div class="wave-name" style="color:#bf5fff;">GAMMA</div>
              <div class="wave-hz">30-45 Hz</div>
            </div>
          </div>
          <canvas class="wave-mini" id="wGamma"></canvas>
          <div class="wave-pct" id="pGamma" style="color:#bf5fff;">0%</div>
        </div>
        <div class="wave-track"><div class="wave-fill" id="bGamma" style="background:#bf5fff;box-shadow:0 0 8px #bf5fff;"></div></div>
      </div>
    </div>

    <!-- Signal Quality -->
    <div class="card signal-card">
      <div class="card-corner tl"></div><div class="card-corner tr"></div>
      <div class="card-corner bl"></div><div class="card-corner br"></div>
      <div class="card-title">
        SIGNAL QUALITY
        <span class="sig-pct" id="sigPct">0%</span>
      </div>
      <div class="seg-bar" id="segBar">
        <!-- filled by JS -->
      </div>
    </div>

  </div>

  <!-- CENTER COLUMN -->
  <div class="center-col">

    <div class="card attention-card">
      <div class="card-corner tl"></div><div class="card-corner tr"></div>
      <div class="card-corner bl"></div><div class="card-corner br"></div>
      <div class="card-title" style="justify-content:center;font-size:12px;letter-spacing:4px;">ATTENTION</div>
      <div class="gauge-wrap">
        <canvas id="attentionGauge" height="200"></canvas>
        <div class="attention-value" id="attVal">0</div>
        <div class="attention-sub">/100</div>
        <div class="focus-badge" id="focusBadge">
          🧠 <span id="focusLabel">WAITING...</span>
        </div>
      </div>
    </div>

    <div class="card trend-card" style="flex:1;">
      <div class="card-corner tl"></div><div class="card-corner tr"></div>
      <div class="card-corner bl"></div><div class="card-corner br"></div>
      <div class="card-title">
        ATTENTION TREND
        <span style="color:var(--green);font-size:10px;">LIVE</span>
      </div>
      <canvas id="trendGraph" style="width:100%;height:130px;display:block;"></canvas>
      <div class="trend-labels">
        <span>30s ago</span><span>15s</span><span>NOW</span>
      </div>
      <div style="display:flex;justify-content:space-between;margin-top:12px;font-size:11px;color:var(--muted);">
        <div style="text-align:center;">
          <div style="font-family:'Orbitron',monospace;color:var(--green);font-size:14px;" id="trendMax">—</div>
          <div>PEAK</div>
        </div>
        <div style="text-align:center;">
          <div style="font-family:'Orbitron',monospace;color:var(--accent);font-size:14px;" id="trendAvg">—</div>
          <div>AVG</div>
        </div>
        <div style="text-align:center;">
          <div style="font-family:'Orbitron',monospace;color:var(--yellow);font-size:14px;" id="trendMin">—</div>
          <div>MIN</div>
        </div>
      </div>
    </div>

  </div>

  <!-- RIGHT COLUMN -->
  <div class="right-col">

    <div class="card brain-card">
      <div class="card-corner tl"></div><div class="card-corner tr"></div>
      <div class="card-corner bl"></div><div class="card-corner br"></div>
      <div class="card-title" style="justify-content:center;">CURRENT STATE</div>
      <!-- Stylised brain SVG -->
      <div class="brain-img">
        <svg viewBox="0 0 120 100" xmlns="http://www.w3.org/2000/svg" class="brain-svg">
          <defs>
            <radialGradient id="brainGlow" cx="50%" cy="50%" r="50%">
              <stop offset="0%" stop-color="#bf5fff" stop-opacity="0.8"/>
              <stop offset="100%" stop-color="#00c8ff" stop-opacity="0.2"/>
            </radialGradient>
            <filter id="gf"><feGaussianBlur in="SourceGraphic" stdDeviation="3"/></filter>
          </defs>
          <ellipse cx="60" cy="50" rx="48" ry="40" fill="url(#brainGlow)" opacity="0.2" filter="url(#gf)"/>
          <!-- brain outline simplified -->
          <path d="M30 55 Q20 45 25 32 Q30 18 45 20 Q50 14 60 16 Q70 14 75 20 Q90 18 95 32 Q100 45 90 55 Q88 68 75 72 Q65 78 60 75 Q55 78 45 72 Q32 68 30 55Z" fill="none" stroke="#bf5fff" stroke-width="1.5" opacity="0.7"/>
          <path d="M60 16 Q60 35 60 75" stroke="#00c8ff" stroke-width="0.8" stroke-dasharray="3,3" opacity="0.5"/>
          <path d="M40 30 Q50 38 55 50 Q50 62 42 68" stroke="#4dd9ff" stroke-width="1" fill="none" opacity="0.6"/>
          <path d="M80 30 Q70 38 65 50 Q70 62 78 68" stroke="#4dd9ff" stroke-width="1" fill="none" opacity="0.6"/>
          <!-- neural dots -->
          <circle cx="45" cy="35" r="2.5" fill="#bf5fff" opacity="0.9" class="pulsing"/>
          <circle cx="75" cy="35" r="2.5" fill="#00c8ff" opacity="0.9" class="pulsing" style="animation-delay:0.5s"/>
          <circle cx="60" cy="50" r="3" fill="#39ff6a" opacity="0.9" class="pulsing" style="animation-delay:1s"/>
          <circle cx="48" cy="60" r="2" fill="#ffe600" opacity="0.8" class="pulsing" style="animation-delay:0.3s"/>
          <circle cx="72" cy="60" r="2" fill="#ff8c00" opacity="0.8" class="pulsing" style="animation-delay:0.8s"/>
          <!-- connecting lines -->
          <line x1="45" y1="35" x2="60" y2="50" stroke="#bf5fff" stroke-width="0.8" opacity="0.4"/>
          <line x1="75" y1="35" x2="60" y2="50" stroke="#00c8ff" stroke-width="0.8" opacity="0.4"/>
          <line x1="60" y1="50" x2="48" y2="60" stroke="#39ff6a" stroke-width="0.8" opacity="0.4"/>
          <line x1="60" y1="50" x2="72" y2="60" stroke="#39ff6a" stroke-width="0.8" opacity="0.4"/>
        </svg>
      </div>

      <div class="state-row">
        <span class="state-label">Focus Level</span>
        <span class="state-val val-high" id="stFocus">HIGH</span>
      </div>
      <div class="state-row">
        <span class="state-label">Meditation</span>
        <span class="state-val val-low" id="stMeditation">LOW</span>
      </div>
      <div class="state-row">
        <span class="state-label">Stress</span>
        <span class="state-val val-low" id="stStress">LOW</span>
      </div>
    </div>

    <div class="card">
      <div class="card-corner tl"></div><div class="card-corner tr"></div>
      <div class="card-corner bl"></div><div class="card-corner br"></div>
      <div class="card-title">SYSTEM</div>

      <div class="sys-item">
        <div class="sys-icon"></div>
        <div class="sys-info">
          <div class="sys-name">ESP32</div>
          <div class="sys-status">Connected</div>
        </div>
        <div class="sys-check"></div>
      </div>

      <div class="sys-item">
        <div class="sys-icon"></div>
        <div class="sys-info">
          <div class="sys-name">MindWave</div>
          <div class="sys-status" id="mwStatus">Connected</div>
        </div>
        <div class="sys-check" id="mwCheck"></div>
      </div>

      <div class="sys-item">
        <div class="sys-icon"></div>
        <div class="sys-info">
          <div class="sys-name">Battery</div>
          <div class="sys-status na" id="sysBatt">85%</div>
        </div>
      </div>

    </div>

  </div>

</div>



<script>
// ── CONFIG ──
const WS_PORT = 81;
const TOTAL_SEGS = 20;

// ── STATE ──
let trendData = [];
let waveData = { delta:[], theta:[], alpha:[], beta:[], gamma:[] };

// ── WEBSOCKET ──
let websocket;
function connectWS() {
  let gateway = `ws://${window.location.hostname}:${WS_PORT}/`;
  websocket = new WebSocket(gateway);
websocket.onmessage = function(e) {
  console.log("RAW:", e.data);

  try {
    let d = JSON.parse(e.data);
    console.log("PARSED:", d);
    updateAll(d);
  } catch(err) {
    console.log("JSON ERROR:", err);
  }
};
  websocket.onclose = () => setTimeout(connectWS, 2000);
}

function updateAll(d) {
  // Brainwaves
  setWave('Delta', d.delta || 0, '#4dd9ff');
  setWave('Theta', d.theta || 0, '#00e5d4');
  setWave('Alpha', d.alpha || 0, '#39ff6a');
  setWave('Beta',  d.beta  || 0, '#ffe600');
  setWave('Gamma', d.gamma || 0, '#bf5fff');

  // Signal
  let sigVal = d.signal !== undefined ? d.signal : 92;
  updateSignal(sigVal);

  // Attention gauge
  let att = d.attention || 0;
  drawAttentionGauge(att);
  document.getElementById('attVal').textContent = att;
  setFocusBadge(att);

  // Trend
  trendData.push(att);
  if (trendData.length > 30) trendData.shift();
  drawTrend();
  updateTrendStats();

  // States
  updateStates(att, d.meditation || 0, d.beta || 0);
}

function setWave(name, val, color) {
  let pct = Math.min(100, Math.max(0, val));
  let id = name.toLowerCase();
  document.getElementById('p' + name).textContent = pct + '%';
  document.getElementById('b' + name).style.width = pct + '%';

  // Push to mini wave data
  if (!waveData[id]) waveData[id] = [];
  waveData[id].push(pct);
  if (waveData[id].length > 20) waveData[id].shift();
  drawMiniWave('w' + name, waveData[id], color);
}

function drawMiniWave(canvasId, data, color) {
  let canvas = document.getElementById(canvasId);
  let ctx = canvas.getContext('2d');
  ctx.clearRect(0, 0, canvas.width, canvas.height);
  if (data.length < 2) return;
  ctx.beginPath();
  let step = canvas.width / (data.length - 1);
  ctx.moveTo(0, canvas.height / 2 - (data[0] / 100) * (canvas.height / 2));
  for (let i = 1; i < data.length; i++) {
    ctx.lineTo(i * step, canvas.height / 2 - (data[i] / 100) * (canvas.height / 2) + canvas.height * 0.2);
  }
  ctx.strokeStyle = color;
  ctx.lineWidth = 1.5;
  ctx.shadowBlur = 6;
  ctx.shadowColor = color;
  ctx.stroke();
}

function updateSignal(val) {
  let active = Math.round((val / 100) * TOTAL_SEGS);
  document.getElementById('sigPct').textContent = val + '%';
  let bar = document.getElementById('segBar');
  bar.innerHTML = '';
  for (let i = 0; i < TOTAL_SEGS; i++) {
    let seg = document.createElement('div');
    seg.className = 'seg' + (i < active ? ' active' : '');
    bar.appendChild(seg);
  }
}

// ── ATTENTION GAUGE ──
function drawAttentionGauge(value) {
  let canvas = document.getElementById('attentionGauge');
  let ctx = canvas.getContext('2d');
  let W = canvas.width, H = canvas.height;
  ctx.clearRect(0, 0, W, H);

  let cx = W / 2, cy = H * 0.72, r = Math.min(W, H) * 0.42;

  // Color stops for gradient arc
  let segments = [
    { start: 0,  end: 0.15, color: '#ff3b5c' },
    { start: 0.15, end: 0.35, color: '#ff8c00' },
    { start: 0.35, end: 0.6,  color: '#ffe600' },
    { start: 0.6,  end: 0.8,  color: '#39ff6a' },
    { start: 0.8,  end: 1.0,  color: '#00c8ff' }
  ];

  // Draw segment arcs (background glow)
  let gap = 0.03;
  segments.forEach(s => {
    let startA = Math.PI + s.start * Math.PI;
    let endA   = Math.PI + s.end   * Math.PI - gap;
    ctx.beginPath();
    ctx.arc(cx, cy, r, startA, endA);
    ctx.lineWidth = 18;
    ctx.strokeStyle = hexAlpha(s.color, 0.15);
    ctx.stroke();
  });

  // Filled portion
  let filledFraction = value / 100;
  segments.forEach(s => {
    if (filledFraction <= s.start) return;
    let endFrac = Math.min(filledFraction, s.end);
    let startA = Math.PI + s.start * Math.PI;
    let endA   = Math.PI + endFrac  * Math.PI - gap;
    ctx.beginPath();
    ctx.arc(cx, cy, r, startA, endA);
    ctx.lineWidth = 18;
    ctx.strokeStyle = s.color;
    ctx.shadowBlur = 15;
    ctx.shadowColor = s.color;
    ctx.stroke();
    ctx.shadowBlur = 0;
  });

  // Tick marks
  for (let i = 0; i <= 10; i++) {
    let frac = i / 10;
    let angle = Math.PI + frac * Math.PI;
    let x1 = cx + (r - 14) * Math.cos(angle);
    let y1 = cy + (r - 14) * Math.sin(angle);
    let x2 = cx + (r + 4)  * Math.cos(angle);
    let y2 = cy + (r + 4)  * Math.sin(angle);
    ctx.beginPath();
    ctx.moveTo(x1, y1);
    ctx.lineTo(x2, y2);
    ctx.lineWidth = i % 5 === 0 ? 2 : 1;
    ctx.strokeStyle = 'rgba(255,255,255,0.2)';
    ctx.shadowBlur = 0;
    ctx.stroke();
  }

  // Labels
  ctx.font = '500 11px Rajdhani, sans-serif';
  ctx.fillStyle = 'rgba(255,255,255,0.4)';
  ctx.textAlign = 'center';
  ctx.fillText('0',  cx + (r + 20) * Math.cos(Math.PI),     cy + (r + 20) * Math.sin(Math.PI));
  ctx.fillText('50', cx + (r + 20) * Math.cos(Math.PI*1.5), cy + (r + 20) * Math.sin(Math.PI*1.5) + 4);
  ctx.fillText('100',cx + (r + 20) * Math.cos(Math.PI*2),   cy + (r + 20) * Math.sin(Math.PI*2));

  // Needle
  let needleAngle = Math.PI + filledFraction * Math.PI;
  ctx.beginPath();
  ctx.moveTo(cx, cy);
  ctx.lineTo(cx + (r - 20) * Math.cos(needleAngle), cy + (r - 20) * Math.sin(needleAngle));
  ctx.lineWidth = 3;
  ctx.strokeStyle = 'white';
  ctx.shadowBlur = 10;
  ctx.shadowColor = 'white';
  ctx.stroke();
  ctx.shadowBlur = 0;

  // Center dot
  ctx.beginPath();
  ctx.arc(cx, cy, 6, 0, Math.PI * 2);
  ctx.fillStyle = 'white';
  ctx.fill();
}

function hexAlpha(hex, alpha) {
  let r = parseInt(hex.slice(1,3),16);
  let g = parseInt(hex.slice(3,5),16);
  let b = parseInt(hex.slice(5,7),16);
  return `rgba(${r},${g},${b},${alpha})`;
}

function setFocusBadge(val) {
  let label, color;
  if (val >= 80)      { label ='EXCELLENT!'; color = '#00c8ff'; }
  else if (val >= 60) { label ='HIGH Great Focus!'; color = '#39ff6a'; }
  else if (val >= 40) { label ='MODERATE'; color = '#ffe600'; }
  else if (val >= 20) { label ='LOW'; color = '#ff8c00'; }
  else                { label ='VERY LOW'; color = '#ff3b5c'; }
  let badge = document.getElementById('focusBadge');
  let span = document.getElementById('focusLabel');
  span.textContent = label;
  badge.style.borderColor = color;
  badge.style.color = color;
  badge.style.background = hexAlpha(color, 0.08);
  badge.style.textShadow = `0 0 10px ${color}`;
}

// ── TREND ──
function drawTrend() {
  let canvas = document.getElementById('trendGraph');
  let ctx = canvas.getContext('2d');
  let W = canvas.width, H = canvas.height;
  ctx.clearRect(0, 0, W, H);
  if (trendData.length < 2) return;

  // Grid lines
  ctx.strokeStyle = 'rgba(255,255,255,0.05)';
  ctx.lineWidth = 1;
  [25,50,75,100].forEach(v => {
    let y = H - (v/100)*H;
    ctx.beginPath(); ctx.moveTo(0,y); ctx.lineTo(W,y); ctx.stroke();
  });

  // Fill gradient
  let grad = ctx.createLinearGradient(0, 0, 0, H);
  grad.addColorStop(0, 'rgba(57,255,106,0.3)');
  grad.addColorStop(1, 'rgba(57,255,106,0)');

  let step = W / (trendData.length - 1);
  ctx.beginPath();
  ctx.moveTo(0, H - (trendData[0]/100)*H);
  for (let i = 1; i < trendData.length; i++) {
    let x = i * step;
    let y = H - (trendData[i]/100)*H;
    ctx.lineTo(x, y);
  }
  ctx.lineTo(W, H); ctx.lineTo(0, H); ctx.closePath();
  ctx.fillStyle = grad;
  ctx.fill();

  // Line
  ctx.beginPath();
  ctx.moveTo(0, H - (trendData[0]/100)*H);
  for (let i = 1; i < trendData.length; i++) {
    ctx.lineTo(i * step, H - (trendData[i]/100)*H);
  }
  ctx.strokeStyle = '#39ff6a';
  ctx.lineWidth = 2.5;
  ctx.shadowBlur = 12;
  ctx.shadowColor = '#39ff6a';
  ctx.stroke();
  ctx.shadowBlur = 0;

  // Latest dot
  let last = trendData[trendData.length-1];
  let lx = (trendData.length-1)*step, ly = H-(last/100)*H;
  ctx.beginPath();
  ctx.arc(lx, ly, 5, 0, Math.PI*2);
  ctx.fillStyle = '#39ff6a';
  ctx.shadowBlur = 15;
  ctx.shadowColor = '#39ff6a';
  ctx.fill();
  ctx.shadowBlur = 0;
}

function updateTrendStats() {
  if (!trendData.length) return;
  document.getElementById('trendMax').textContent = Math.max(...trendData) + '%';
  document.getElementById('trendMin').textContent = Math.min(...trendData) + '%';
  let avg = Math.round(trendData.reduce((a,b)=>a+b,0)/trendData.length);
  document.getElementById('trendAvg').textContent = avg + '%';
}

// ── STATES ──
function updateStates(att, med, beta) {
  let focusEl = document.getElementById('stFocus');
  if (att >= 70) { focusEl.textContent='HIGH'; focusEl.className='state-val val-high'; }
  else if (att >= 40) { focusEl.textContent='MED'; focusEl.className='state-val val-med'; }
  else { focusEl.textContent='LOW'; focusEl.className='state-val val-low'; }

  let medEl = document.getElementById('stMeditation');
  if (med >= 70) { medEl.textContent='HIGH'; medEl.className='state-val val-high'; }
  else if (med >= 40) { medEl.textContent='MED'; medEl.className='state-val val-med'; }
  else { medEl.textContent='LOW'; medEl.className='state-val val-low'; }

  let stressEl = document.getElementById('stStress');
  if (beta >= 70) { stressEl.textContent='HIGH'; stressEl.className='state-val val-high'; }
  else if (beta >= 40) { stressEl.textContent='MED'; stressEl.className='state-val val-med'; }
  else { stressEl.textContent='LOW'; stressEl.className='state-val val-low'; }
}

// ── CLOCK ──
function updateClock() {
  let now = new Date();
  document.getElementById('clock').textContent =
    now.toLocaleTimeString('en-US', {hour:'2-digit',minute:'2-digit',second:'2-digit'});
}
setInterval(updateClock, 1000);
updateClock();

// ── INIT SIGNAL BAR ──
updateSignal(0);
drawAttentionGauge(0);

// ── CONNECT ──
connectWS();

</script>
</body>
</html>
)rawliteral";

server.send(200,"text/html",html);
}
 /******** WebSocket event handler ********/
 
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {

  switch(type) {

    case WStype_CONNECTED:
      Serial.println("WebSocket Client Connected");
      break;

    case WStype_DISCONNECTED:
      Serial.println("WebSocket Client Disconnected");
      break;

    default:
      break;
  }
}
