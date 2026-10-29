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
const char* ssid = "realme 14 Pro+ 5G t8nu";
const char* password = "";
bool wifiConnected = false;
String ipAddress = "NA";
#include <WebServer.h>
WebServer server(80);  

/******************** WEBSOCKET ************************/
#include <WebSocketsServer.h>
WebSocketsServer webSocket = WebSocketsServer(81);
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

/******************** MOTOR PINS **************************/
#define IN1 26
#define IN2 27
#define ENA 14

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
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENA, OUTPUT);

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
              i += 25;
              break;
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

      if (attention > 30) {
        speed = map(attention, 30, 100, 50, 150);
        speedPercent = map(speed, 50, 150, 0, 100);
      }

      analogWrite(ENA, speed);

      // Send live data to dashboard
String json = "{";
json += "\"attention\":" + String(attention) + ",";
json += "\"speed\":" + String(speedPercent);
json += "}";

webSocket.broadcastTXT(json);

      /******** SERIAL ********/
      Serial.println("====== DATA ======");
      Serial.print("Attention: ");
      Serial.print(attention);
      Serial.println(" %");

      Serial.print("Motor Speed: ");
      Serial.print(speedPercent);
      Serial.println(" %");

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

void handleRoot() {

  int speedPercent = 0;

  if (attention > 30) {
    int speed = map(attention, 30, 100, 50, 150);
    speedPercent = map(speed, 50, 150, 0, 100);
  }

  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Brain Control Dashboard</title>

<style>
body {
  background: #f5f5dc; /* cream */
  font-family: Arial;
  text-align: center;
  margin: 0;
}

h1 {
  padding: 15px;
  background: #e6e6c3;
  margin: 0;
}

.container {
  display: flex;
  justify-content: center;
  gap: 40px;
  flex-wrap: wrap;
  margin-top: 20px;
}

canvas {
  background: white;
  border-radius: 50%;
  box-shadow: 0px 0px 10px rgba(0,0,0,0.2);
}

/* Focus Dot */
.dot {
  width: 40px;
  height: 40px;
  background: green;
  border-radius: 50%;
  margin: 30px auto;
  box-shadow: 0 0 20px green;
}

.message {
  font-weight: bold;
  font-size: 16px;
  color: #333;
}
</style>

</head>

<body>

<h1>Brain Control System</h1>

<div class="container">
  <canvas id="attentionGauge" width="250" height="250"></canvas>
  <canvas id="speedGauge" width="250" height="250"></canvas>
</div>

<div class="dot"></div>
<div class="message">ATTENTION ON THE GREEN DOT</div>

<script>

let attention = )rawliteral" + String(attention) + R"rawliteral(;
let speed = )rawliteral" + String(speedPercent) + R"rawliteral(;

// Professional Gauge
function drawGauge(canvasId, value, label, color) {
  let canvas = document.getElementById(canvasId);
  let ctx = canvas.getContext("2d");

  ctx.clearRect(0, 0, canvas.width, canvas.height);

  let cx = canvas.width / 2;
  let cy = canvas.height / 2;
  let r = 90;

  // Background arc
  ctx.beginPath();
  ctx.arc(cx, cy, r, Math.PI, 0);
  ctx.lineWidth = 12;
  ctx.strokeStyle = "#ddd";
  ctx.stroke();

  // Value arc
  let end = Math.PI + (value / 100) * Math.PI;
  ctx.beginPath();
  ctx.arc(cx, cy, r, Math.PI, end);
  ctx.strokeStyle = color;
  ctx.lineWidth = 12;
  ctx.stroke();

  // Needle
  let angle = Math.PI + (value / 100) * Math.PI;
  let nx = cx + 70 * Math.cos(angle);
  let ny = cy + 70 * Math.sin(angle);

  ctx.beginPath();
  ctx.moveTo(cx, cy);
  ctx.lineTo(nx, ny);
  ctx.strokeStyle = "black";
  ctx.lineWidth = 3;
  ctx.stroke();

  // Center dot
  ctx.beginPath();
  ctx.arc(cx, cy, 6, 0, 2 * Math.PI);
  ctx.fillStyle = "black";
  ctx.fill();

  // Label
  ctx.font = "16px Arial";
  ctx.fillStyle = "#333";
  ctx.textAlign = "center";
  ctx.fillText(label, cx, cy + 50);

  // Value
  ctx.font = "20px Arial";
  ctx.fillText(value + "%", cx, cy + 75);
}

// Draw both gauges
drawGauge("attentionGauge", attention, "Attention", "#4CAF50");
drawGauge("speedGauge", speed, "Motor Speed", "#FF9800");

// Auto refresh
let gateway = `ws://${window.location.hostname}:81/`;
let websocket;

function initWebSocket() {
  websocket = new WebSocket(gateway);

  websocket.onopen = function() {
    console.log("WebSocket Connected");
  };

  websocket.onmessage = function(event) {
    let data = JSON.parse(event.data);

    attention = data.attention;
    speed = data.speed;

    drawGauge("attentionGauge", attention, "Attention", "#4CAF50");
    drawGauge("speedGauge", speed, "Motor Speed", "#FF9800");
  };

  websocket.onclose = function() {
    setTimeout(initWebSocket, 2000);
  };
}

window.onload = initWebSocket;

</script>
</body>
</html>
)rawliteral";

  server.send(200, "text/html", html);
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
