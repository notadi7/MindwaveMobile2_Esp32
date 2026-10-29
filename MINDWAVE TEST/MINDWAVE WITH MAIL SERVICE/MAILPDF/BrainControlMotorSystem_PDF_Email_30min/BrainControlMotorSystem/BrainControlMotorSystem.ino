/************************************************************
   🧠 BRAIN CONTROL MOTOR SYSTEM
   ESP32 + MindWave + L293N + OLED (SH1106) + WiFi
************************************************************/

#include "BluetoothSerial.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <WiFi.h>
#include <time.h>

/******************** WIFI *******************************/
const char* ssid = "Adis iPhone";
const char* password = "adiii6677";
bool wifiConnected = false;
String ipAddress = "NA";
#include <WebServer.h>
WebServer server(80);  

/******************** WEBSOCKET ************************/
#include <WebSocketsServer.h>
WebSocketsServer webSocket = WebSocketsServer(81);
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);
void handleRoot();
void handleStartSession();
void handleSendEmail();
void handleNewSession();
void sendSetupPage();
void sendDashboardPage();
void sendResultPage();
bool sendResultEmailNow();

/******************** EMAIL (SMTP) ************************
   Uses the ESP-Mail-Client library (Mobizt) for direct SMTP
   sending — no third-party server/webhook required.
   Install via Library Manager: "ESP Mail Client" by Mobizt.

   Fill these in before flashing. For Gmail, SMTP_PASS must be
   a 16-character App Password (Google Account > Security >
   2-Step Verification > App passwords) — your normal Gmail
   password will NOT work here.
*************************************************************/
#include <ESP_Mail_Client.h>

#define SMTP_HOST        "smtp.gmail.com"
#define SMTP_PORT        465
#define SMTP_USER        "mindwavemobile2@gmail.com"       // sender account
#define SMTP_PASS        "vibfizfjtgapaqnb"       // Gmail App Password
#define SMTP_SENDER_NAME "Brain Control Motor System"

SMTPSession smtp;
bool emailInProgress = false;
bool emailLastResult = false;   // true = last send succeeded
bool emailAttempted  = false;   // has an email send been tried this session


/******************** OBJECTS ********************/
BluetoothSerial SerialBT;
Adafruit_SH1106G display = Adafruit_SH1106G(128, 64, &Wire);

/******************** BLUETOOTH ************************/
uint8_t address[6] = {0xC4, 0x64, 0xE3, 0xE8, 0xC4, 0xAB};
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

/******************** DEMO MODE ****************************
   Set to 1 to test the whole web/session flow WITHOUT a real
   MindWave headset. Feeds synthetic attention/wave data into
   the same variables the real Bluetooth parser fills, so the
   relay/motor logic, countdown, motor-on tracking, result
   popup and email all run for real off fake input.
   Set back to 0 once your sensor is ready.
*************************************************************/
#define DEMO_MODE 0

/******************** TIMING *******************************/
unsigned long lastPrintTime = 0;

unsigned long signalLostTime = 0;
bool signalLost = false;

/******************** SESSION STATE MACHINE ******************
   STATE_SETUP     -> credential/setup page is served at "/"
   STATE_DASHBOARD -> countdown running, motor/relay live-controlled
   STATE_RESULT    -> countdown hit 0, motor forced off, result
                       popup shown on top of the dashboard
****************************************************************/
enum AppState { STATE_SETUP, STATE_DASHBOARD, STATE_RESULT };
AppState appState = STATE_SETUP;

// Credential-capture fields (in-memory only, cleared on new session)
String userName = "";
String userEmail = "";
int sessionMinutes = 0;

// Session timing (all millis()-based, non-blocking)
unsigned long sessionStartMillis = 0;
long sessionRemainingSeconds = 0;     // counts down to 0
unsigned long lastSessionTickMillis = 0;

// Motor-on-time accumulation ("focus score")
unsigned long motorOnSeconds = 0;
bool relayWasOn = false;
unsigned long relayOnSinceMillis = 0;

// Attention accumulation for the session (avg/peak), independent of
// the smoothed values used for the live gauges
unsigned long attentionSampleSum = 0;
unsigned long attentionSampleCount = 0;
byte attentionPeak = 0;
int avgAttentionResult = 0;   // frozen at session end for the result page
int peakAttentionResult = 0;

// Brainwave % + focus score, also frozen at session end for the result page
int resultDeltaPercent = 0;
int resultThetaPercent = 0;
int resultAlphaPercent = 0;
int resultBetaPercent  = 0;
int resultGammaPercent = 0;
int resultFocusScore   = 0;   // 0-100, blend of avg/peak attention + motor-on ratio


/************************************************************
   FORWARD DECLARATIONS for functions defined in the other
   tabs of this sketch. Arduino normally auto-generates these,
   they're spelled out here so it's obvious at a glance what
   lives where.
************************************************************/
void resetSessionAccumulators();
void endSession();
byte ReadOneByte();
void parseNeuroSkyPacket();
void telemetryTick();
void showText(String line1, String line2 = "", String line3 = "");
void drawCenteredText(String text, int y);
void connectWiFi();
#if DEMO_MODE
void simulateSensorData();
#endif

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
#if DEMO_MODE
  connected = true;   // pretend the headset is already connected
  showText("DEMO MODE", "No sensor needed", "Fake data active");
  delay(1500);
#else
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
#endif

  // 📶 CONNECT WIFI
  connectWiFi();

  // Flash filesystem used by ESP-Mail-Client for the temporary PDF attachment.
  if (!ESP_MAIL_DEFAULT_FLASH_FS.begin(true)) {
    Serial.println("Flash filesystem mount failed");
  } else {
    Serial.println("Flash filesystem ready for PDF reports");
  }

  // India Standard Time for report timestamps / mail time handling.
  configTime(19800, 0, "pool.ntp.org", "time.nist.gov");
  server.on("/", HTTP_GET, handleRoot);            // conditionally renders setup or dashboard
  server.on("/start-session", HTTP_POST, handleStartSession);
  server.on("/send-email", HTTP_POST, handleSendEmail);
  server.on("/new-session", HTTP_POST, handleNewSession);
server.begin();

webSocket.begin();
webSocket.onEvent(webSocketEvent);

Serial.println("Web server started");
Serial.print("Open in browser: ");
Serial.println(ipAddress);
}


/************************************************************
   🔁 LOOP
   Kept intentionally short: DEMO_MODE / real-sensor reads and
   the once-a-second telemetry+session tick are one-line calls
   into NeuroSky_Parser.ino / Demo_Mode.ino / Telemetry.ino, so
   you can read loop() top-to-bottom without wading into the
   packet-parsing or JSON-building details.
************************************************************/
void loop() {
  server.handleClient();
  webSocket.loop();

#if DEMO_MODE
  simulateSensorData();
#else
  /******** RECONNECT IF DISCONNECTED ********/
  if (!connected) {
    showText("Reconnecting...");
    connected = SerialBT.connect(address);
    delay(3000);
    return;
  }

  /******** PARSE NEUROSKY DATA ********/
  parseNeuroSkyPacket();
#endif

  /******** UPDATE EVERY 1 SECOND (telemetry + session tick) ********/
  if (millis() - lastPrintTime >= 1000) {
    lastPrintTime = millis();
    telemetryTick();
  }
}
