/************************************************************
   🧠 BRAIN CONTROL MOTOR SYSTEM
   ESP32 + MindWave + L293N + ST7735 TFT + WiFi

   ARCHITECTURE (v2 — state-machine rewrite)
   ------------------------------------------------------------
   The whole application is driven by ONE authoritative
   SystemState (see enum below). No module is allowed to decide
   on its own what the TFT or web dashboard should show — every
   module only ever *reads* systemState/eegSignalValid or moves
   systemState forward through an explicit transition. Rendering
   (TFT_Display.ino::updateTFT) and the web dashboard both react
   to that single source of truth.

   See README.md for the full state-transition tables.
************************************************************/

#include "BluetoothSerial.h"
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <WiFi.h>
#include <time.h>

/******************** WIFI *******************************
   ssid/password are now loaded at runtime from NVS flash
   (see WiFi_Setup_Portal.ino) instead of being hardcoded.
   These two act as a one-time fallback ONLY — used if NVS
   has never been configured yet (first boot out of the box).
   After the first successful setup via the GPIO13 button, NVS
   always wins and these defaults are ignored.
*************************************************************/
String wifiSsid     = "Adis iPhone";
String wifiPassword = "adiii6677";
bool wifiConnected = false;
String ipAddress = "NA";
#include <WebServer.h>
WebServer server(80);

/******************** WIFI SETUP PORTAL ********************
   Shared defines/objects for WiFi_Setup_Portal.ino. Declared
   here (main file) since it always compiles first — the tab
   itself sorts alphabetically AFTER Setup_Page.ino and
   WiFi_Connect.ino, which both need these symbols too.
*************************************************************/
#include <Preferences.h>
#include <DNSServer.h>

#define WIFI_SETUP_BTN         13             // physical GPIO13 WiFi-setup button
#define WIFI_HOLD_MS           3000           // how long to hold to trigger setup mode
#define WIFI_PORTAL_TIMEOUT_MS 300000         // auto-give-up after 5 min stuck in setup mode

Preferences wifiPrefs;
DNSServer dnsServer;
bool wifiSetupPortalActive = false;           // mirrors (systemState == STATE_WIFI_PORTAL)
unsigned long wifiPortalStartedAt = 0;

/******************** WEBSOCKET ************************/
#include <WebSocketsServer.h>
WebSocketsServer webSocket = WebSocketsServer(81);
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);
void handleRoot();
void handleStartSession();
void handleSendEmail();
void handleNewSession();
void handleDownloadPdf();
void handleEmergencyStop();
void handleStatus();
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
#define SMTP_SENDER_NAME "Brain Control Motor System"
String smtpUser = "";
String smtpPass = "";

SMTPSession smtp;
bool emailInProgress = false;
bool emailLastResult = false;   // true = last send succeeded
bool emailAttempted  = false;   // has an email send been tried this session


/******************** OBJECTS ********************/
BluetoothSerial SerialBT;
// TFT `tft` object, pin map, and colors are declared in TFT_Display.ino

/******************** BLUETOOTH / SENSOR TRANSPORT ********************
   mindwaveConnected = the SPP/Bluetooth *transport* is connected.
   eegSignalValid    = recent, checksum-valid EEG packets are arriving
                        AND signal quality is acceptable.
   These are deliberately two different booleans (req. #34) — a
   connected headset sitting loosely on the user's head is
   "CONNECTED + NO EEG" = NO SIGNAL, not READY.
*************************************************************/
uint8_t address[6] = {0xC4, 0x64, 0xE3, 0xE8, 0xC4, 0xAB};
bool mindwaveConnected = false;

#define EEG_TIMEOUT_MS          2500   // no valid packet for this long -> signal lost
#define POOR_QUALITY_THRESHOLD  50     // NeuroSky poorSignal below this = acceptable
unsigned long lastValidEEGPacketMillis = 0;
bool eegSignalValid = false;

#define BT_RECONNECT_INTERVAL_MS 8000  // real-mode only; see NOTE in loop() below
unsigned long lastBtReconnectAttempt = 0;

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

// Motor/relay live status, computed each loop by Motor_Control.ino::updateMotorControl()
// and read by Telemetry.ino + TFT_Display.ino. Never written from anywhere else.
int motorSpeedPercent = 0;
bool motorRunning = false;

// Dashboard LED indicator — deliberately a SEPARATE signal from motorRunning:
// the LED lights at ATT > 40 (same gate as the relay) while the motor itself
// only actually starts spinning at ATT > 60 (see Motor_Control.ino). Written
// only by updateMotorControl(), read by Telemetry.ino for the "led" field.
bool ledOn = false;

/******************** DEMO MODE ****************************
   Set to 1 to test the whole web/session/TFT flow WITHOUT a
   real MindWave headset. Feeds synthetic EEG data into the
   exact same variables the real Bluetooth parser fills, so
   every downstream module (state machine, motor logic, TFT,
   dashboard, session/result/email) runs identically in demo
   or real mode — only the *source* of attention/brainwave data
   differs (req. #1, #35).
   Set to 0 for real hardware.
*************************************************************/
#define DEMO_MODE 0

#if DEMO_MODE
bool demoSimulateSignalLoss = false;   // optional test hook, see Demo_Mode.ino
#endif

/******************** TIMING *******************************/
#define TFT_LIVE_REFRESH_MS       300    // ACTIVE_SESSION / WIFI_COUNTDOWN live redraw cadence
#define TELEMETRY_INTERVAL_MS     250    // WebSocket broadcast cadence
#define SESSION_STARTED_SCREEN_MS 1800   // how long "07 - SESSION STARTED" holds before ACTIVE_SESSION
#define EMERGENCY_STOP_HOLD_MS    1500   // how long "EMERGENCY STOP" holds before auto-recovery

/******************** GLOBAL SYSTEM STATE MACHINE ******************
   ONE authoritative state. Every module (TFT, web dashboard,
   motor control, session manager, WiFi portal) reacts to this;
   none of them may set it except through the explicit
   transitions documented in README.md.
****************************************************************/
enum SystemState {
  STATE_BOOT,
  STATE_WIFI_CONNECTING,
  STATE_SENSOR_CONNECTING,
  STATE_NO_SIGNAL,
  STATE_READY,
  STATE_WIFI_COUNTDOWN,
  STATE_WIFI_PORTAL,
  STATE_SESSION_STARTED,
  STATE_ACTIVE_SESSION,
  STATE_SESSION_ENDED,
  STATE_EMERGENCY_STOP
};
SystemState systemState = STATE_BOOT;

// Remembers what to restore to if a GPIO13 hold is cancelled before
// reaching WIFI_HOLD_MS (req. #5 — WiFi button must never corrupt or
// lose whatever the system was doing).
SystemState wifiCountdownPreviousState = STATE_READY;
unsigned long wifiCountdownStartedAt = 0;
unsigned long emergencyStopSince = 0;

const char* systemStateName(SystemState s);   // Telemetry.ino / status endpoint

// Credential-capture fields (in-memory only, cleared on new session)
String userName = "";
String userEmail = "";
int sessionMinutes = 0;

// Session timing — ALL millis()-based and absolute (req. #12/#13):
// remaining = duration - (now - sessionStartMillis). Never decremented
// by a tick function, so it can't drift regardless of WiFi/BLE/TFT load.
unsigned long sessionStartMillis = 0;
long sessionDurationSeconds = 0;
long sessionRemainingSeconds = 0;       // recomputed every loop from the two values above
unsigned long sessionStartedScreenSince = 0;

// Motor-on-time accumulation ("focus score") — accumulated once/second
// from Motor_Control.ino, ONLY while STATE_ACTIVE_SESSION && eegSignalValid.
unsigned long motorOnSeconds = 0;
unsigned long lastAccumMillis = 0;

// Attention accumulation for the session (avg/peak), independent of
// the smoothed values used for the live gauges
unsigned long attentionSampleSum = 0;
unsigned long attentionSampleCount = 0;
byte attentionPeak = 0;
int avgAttentionResult = 0;   // frozen at session end for the result page
int peakAttentionResult = 0;

// Brainwave % + focus score, also frozen at session end for the result
// page (SessionResult, req. #31 — a dedicated set of "write-once"
// globals that Email_Sender/Result_Page read; nothing touches them
// again until the NEXT session starts).
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
void updateSessionState(unsigned long now);

void processMindWaveBluetooth();
void processMindWaveByte(uint8_t b);
void parseNeuroSkyPayload();

void updateSensorState(unsigned long now);

void updateMotorControl(unsigned long now);

void updateTelemetry(unsigned long now);
void broadcastTelemetry();

void initTFT();
void updateTFT(unsigned long now);
void showText(String line1, String line2 = "", String line3 = "");
void drawCenteredText(String text, int y);
void showBootScreen();
void showDemoMode();
void showSensorConnecting();
void showWifiSetup(int secondsLeft);
void showWifiPortal();
void showSessionStarted(String name, int minutes);
void drawActiveSessionStatic();
void refreshActiveSessionLive(int speedPercent);
void showBrainwaves();
void showNoSignal();
void showReady();
void showSessionEnded(int avgAttention, unsigned long motorSeconds);

void connectWiFi();
void loadWifiCredentials();
void saveWifiCredentials(const String &newSsid, const String &newPass);
void checkWifiSetupButton();
void startWifiConfigPortal();
void wifiSetupPortalTick();
void handleWifiSetupRoot();
void handleWifiSetupSave();
void handleWifiSetupNotFound();
void loadSystemCredentials();
void saveSystemCredentials(const String &gmail, const String &gmailPass);
void handleSettingsData();
void handleSettingsSave();
void handleWifiSetupFromDashboard();
void handleWifiScan();
#if DEMO_MODE
void simulateSensorData();
#endif

/************************************************************
   ⚙️ SETUP
   Everything here runs ONCE, before loop() starts — the
   blocking calls in this function (short boot-screen holds,
   the one-shot WiFi connect attempt, the one-shot Bluetooth
   pairing attempt) are the explicitly-allowed exception in
   req. #37 ("extremely short hardware initialization delays").
   None of them execute again once loop() is running.
************************************************************/
void setup() {
  Serial.begin(115200);

  // TFT INIT
  initTFT();
  systemState = STATE_BOOT;
  showBootScreen();
  delay(1200);

  // WIFI SETUP BUTTON (GPIO13, hold 3s to reconfigure WiFi)
  pinMode(WIFI_SETUP_BTN, INPUT_PULLUP);

  // Load any previously-saved WiFi credentials from NVS flash
  loadWifiCredentials();
  loadSystemCredentials();

  // MOTOR SETUP
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENA, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);   // Active LOW relay OFF initially
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);

  // 📶 CONNECT WIFI (blocking, one-shot, pre-loop only)
  systemState = STATE_WIFI_CONNECTING;
  connectWiFi();   // starts the config portal itself if there are no saved credentials at all

  // BLUETOOTH / SENSOR SETUP
  systemState = STATE_SENSOR_CONNECTING;
#if DEMO_MODE
  mindwaveConnected = true;   // pretend the headset is already connected
  showDemoMode();
  delay(1200);
#else
  SerialBT.begin("ESP32_Master", true);
  SerialBT.setPin("0000");

  showSensorConnecting();
  mindwaveConnected = SerialBT.connect(address);
  lastBtReconnectAttempt = millis();
  // If this first attempt failed, loop()'s non-blocking reconnect timer
  // (see loop() below) will keep retrying periodically — no blocking
  // wait-loop here, and the TFT/state machine already correctly show
  // STATE_SENSOR_CONNECTING regardless of the outcome.
#endif

  // Flash filesystem used by ESP-Mail-Client for the temporary PDF attachment.
  if (!ESP_MAIL_DEFAULT_FLASH_FS.begin(true)) {
    Serial.println("Flash filesystem mount failed");
  } else {
    Serial.println("Flash filesystem ready for PDF reports");
  }

  // India Standard Time for report timestamps / mail time handling.
  configTime(19800, 0, "pool.ntp.org", "time.nist.gov");

  server.on("/", HTTP_GET, handleRoot);                       // renders according to systemState
  server.on("/setup", HTTP_GET, sendSetupPage);                // explicit, deterministic routes
  server.on("/dashboard", HTTP_GET, sendDashboardPage);
  server.on("/result", HTTP_GET, sendResultPage);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/start-session", HTTP_POST, handleStartSession);
  server.on("/send-email", HTTP_POST, handleSendEmail);
  server.on("/new-session", HTTP_POST, handleNewSession);
  server.on("/download-pdf", HTTP_GET, handleDownloadPdf);
  server.on("/estop", HTTP_POST, handleEmergencyStop);
  server.on("/wifi-save", HTTP_POST, handleWifiSetupSave);
  server.on("/settings-data", HTTP_GET, handleSettingsData);
  server.on("/settings-save", HTTP_POST, handleSettingsSave);
  server.on("/wifi-setup", HTTP_POST, handleWifiSetupFromDashboard);
  server.on("/wifi-scan", HTTP_GET, handleWifiScan);
  server.onNotFound(handleWifiSetupNotFound);                 // captive portal redirect when in setup mode
  server.begin();

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  Serial.println("Web server started");
  Serial.print("Open in browser: ");
  Serial.println(ipAddress);
}


/************************************************************
   🔁 LOOP
   Cooperative / non-blocking. Every subsystem gets serviced
   every single iteration — no individual function may
   monopolize the CPU (req. #22). GPIO13 is checked FIRST and
   unconditionally, so the WiFi-setup button always works
   regardless of what else is happening (req. #25).
************************************************************/
void loop() {
  unsigned long now = millis();

  // Highest priority: safety input, works in every state.
  checkWifiSetupButton();

  // WiFi/captive portal DNS (non-blocking, only does anything
  // while systemState == STATE_WIFI_PORTAL).
  wifiSetupPortalTick();

  // HTTP + WebSocket — always serviced, even mid-session, mid-portal.
  server.handleClient();
  webSocket.loop();

  // MindWave — byte-by-byte, non-blocking, returns immediately
  // when there is nothing to read.
#if DEMO_MODE
  simulateSensorData();
#else
  processMindWaveBluetooth();

  // Real-mode reconnect: BluetoothSerial's classic SPP connect() call
  // is itself a blocking library call (no true async connect exists in
  // the ESP32 Arduino BluetoothSerial API) — the fix in req. #6 is for
  // the *byte parser* (ReadOneByte()/while(!available())), which is
  // fully non-blocking above. To keep that one unavoidable blocking
  // call from starving the rest of the loop, it is only attempted on a
  // long, explicit timer (every BT_RECONNECT_INTERVAL_MS) rather than
  // every iteration, and only while actually disconnected.
  if (!mindwaveConnected && (now - lastBtReconnectAttempt >= BT_RECONNECT_INTERVAL_MS)) {
    lastBtReconnectAttempt = now;
    mindwaveConnected = SerialBT.connect(address);
  }
#endif

  // Sensor connection / EEG-signal-validity state
  updateSensorState(now);

  // Session state/timer (server-authoritative, millis()-based)
  updateSessionState(now);

  // Motor safety/control — the ONLY place that writes RELAY_PIN/ENA
  updateMotorControl(now);

  // TFT — the ONLY place that calls a show*()/draw*() render function
  updateTFT(now);

  // Telemetry — WebSocket JSON broadcast
  updateTelemetry(now);
}
