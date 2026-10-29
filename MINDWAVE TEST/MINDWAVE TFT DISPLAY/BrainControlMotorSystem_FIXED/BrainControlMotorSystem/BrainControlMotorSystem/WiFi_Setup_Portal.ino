/************************************************************
   WIFI_SETUP_PORTAL.INO
   Runtime WiFi credential changer.

   HOW IT WORKS (from the user's side):
   1. Hold the GPIO13 button for 3 seconds, any time the device
      is powered on — works during READY, mid-session, after a
      session ends, with no EEG signal, with the dashboard open,
      literally any state (req. #5/#25).
   2. TFT shows a "05 . WIFI SETUP" countdown, then the portal
      screen. ESP32 opens its own WiFi network: "MindWave-Setup"
      (no password).
   3. Phone connects to that network, and because of the DNS
      redirect below, the phone auto-pops a "Sign in to
      network" style page — no need to type an IP manually.
      (If it doesn't auto-pop, open any http:// address and
      it will redirect there, or go to 192.168.4.1 directly.)
   4. Pick/enter the WiFi name + password, hit Save.
   5. ESP32 stores it in NVS flash (Preferences library) and
      reboots. From then on it just connects on its own —
      nothing hardcoded, nothing to reflash.

   NOTE ON THE BUTTON PIN:
   WIFI_SETUP_BTN (defined in BrainControlMotorSystem.ino) is
   GPIO13. Wire the button between this pin and GND;
   INPUT_PULLUP is used, so the pin reads HIGH when idle and
   LOW when pressed.
************************************************************/

// NOTE: #defines, the Preferences/DNSServer objects, and the
// wifiSetupPortalActive flag all live in BrainControlMotorSystem.ino
// now (not here), because Arduino concatenates .ino tabs in
// alphabetical order with the main file first — anything defined
// only in THIS file wouldn't exist yet by the time Setup_Page.ino
// or WiFi_Connect.ino (which sort earlier) needed it.

unsigned long btnPressStart = 0;
bool btnHeld = false;

/************************************************************
   🔐 LOAD / SAVE CREDENTIALS (NVS via Preferences)
************************************************************/
void loadWifiCredentials() {
  wifiPrefs.begin("wifi", true);           // read-only
  wifiSsid     = wifiPrefs.getString("ssid", "");
  wifiPassword = wifiPrefs.getString("pass", "");
  wifiPrefs.end();
}

void saveWifiCredentials(const String &newSsid, const String &newPass) {
  wifiPrefs.begin("wifi", false);          // read-write
  wifiPrefs.putString("ssid", newSsid);
  wifiPrefs.putString("pass", newPass);
  wifiPrefs.end();
}


/************************************************************
   🔐 SYSTEM CREDENTIALS (NVS)
************************************************************/
void loadSystemCredentials() {
  wifiPrefs.begin("system", true);
  smtpUser = wifiPrefs.getString("gmail", "");
  smtpPass = wifiPrefs.getString("gmailPass", "");
  wifiPrefs.end();
  if (smtpUser.length() == 0) smtpUser = "mindwavemobile2@gmail.com";
  if (smtpPass.length() == 0) smtpPass = "vibfizfjtgapaqnb";
}

void saveSystemCredentials(const String &gmail, const String &gmailPass) {
  wifiPrefs.begin("system", false);
  wifiPrefs.putString("gmail", gmail);
  wifiPrefs.putString("gmailPass", gmailPass);
  wifiPrefs.end();
  smtpUser = gmail;
  smtpPass = gmailPass;
}

/************************************************************
   🔘 BUTTON WATCHER — call every loop(), non-blocking.
   Works at ANY time (req. #5/#25): READY, before a session,
   mid ACTIVE_SESSION, after SESSION_ENDED, with no EEG signal,
   with the dashboard open — checkWifiSetupButton() never
   returns early because of any of those. It only skips work
   once systemState is already STATE_WIFI_PORTAL (nothing left
   to do; the portal is running and only exits via reboot).

   TFT feedback is NOT drawn here — entering STATE_WIFI_COUNTDOWN
   is enough; TFT_Display.ino::updateTFT() notices the state
   change and renders/holds the countdown itself (req. #24).
************************************************************/
void checkWifiSetupButton() {
  if (systemState == STATE_WIFI_PORTAL) return;   // already there; portal only exits via reboot

  if (digitalRead(WIFI_SETUP_BTN) == LOW) {
    if (!btnHeld) {
      btnHeld = true;
      btnPressStart = millis();
      wifiCountdownStartedAt = btnPressStart;
      wifiCountdownPreviousState = systemState;   // remember what to restore on cancel
      systemState = STATE_WIFI_COUNTDOWN;

      // Zero-latency safety cutoff (req. #5): don't wait for the next
      // updateMotorControl() pass if a session was active.
      digitalWrite(RELAY_PIN, HIGH);
      analogWrite(ENA, 0);
    }

    unsigned long held = millis() - btnPressStart;
    if (held >= WIFI_HOLD_MS) {
      btnHeld = false;
      startWifiConfigPortal();   // sets systemState = STATE_WIFI_PORTAL
    }
  } else if (btnHeld) {
    btnHeld = false;
    if (systemState == STATE_WIFI_COUNTDOWN) {
      // Released early -> cancel, restore whatever was happening before
      // (session data/accumulators were never touched, so an active
      // session simply keeps running exactly where it left off).
      systemState = wifiCountdownPreviousState;
    }
  }
}

/************************************************************
   📡 START CONFIG PORTAL (soft AP + captive portal)
************************************************************/
void startWifiConfigPortal() {
  Serial.println("Entering WiFi setup portal...");
  systemState = STATE_WIFI_PORTAL;
  wifiSetupPortalActive = true;
  wifiPortalStartedAt = millis();

  WiFi.disconnect(false, false);
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPdisconnect(true);
  // One-time, ~100ms hardware settle time the ESP32 WiFi driver needs
  // between tearing down and restarting the AP — this is the
  // "extremely short hardware initialization delay" exception in
  // req. #37, runs exactly once per portal entry (not on every loop),
  // and every other subsystem (GPIO13, HTTP, WebSocket) simply resumes
  // being serviced on the very next loop() iteration afterward.
  delay(100);
  WiFi.softAP("MindWave-Setup");

  IPAddress apIP = WiFi.softAPIP();
  dnsServer.start(53, "*", apIP);  // all DNS queries -> this device, triggers captive portal popup

  Serial.print("Setup AP IP: ");
  Serial.println(apIP);
}

/************************************************************
   🌀 CALL FROM loop() WHILE PORTAL IS ACTIVE
   Services the DNS redirect and gives up after a timeout so
   the device doesn't get stuck in setup mode forever if the
   user walks away.
************************************************************/
void wifiSetupPortalTick() {
  if (systemState != STATE_WIFI_PORTAL) return;

  dnsServer.processNextRequest();

  if (millis() - wifiPortalStartedAt >= WIFI_PORTAL_TIMEOUT_MS) {
    Serial.println("WiFi setup portal timed out, rebooting...");
    ESP.restart();
  }
}

/************************************************************
   📡 AJAX WIFI SCAN HANDLER
   Called by the setup page at /wifi-scan. Returns nearby
   networks as JSON so the SSID dropdown is populated
   automatically instead of requiring manual entry.
************************************************************/
String wifiJsonEscape(String value) {
  value.replace("\\", "\\\\");
  value.replace("\"", "\\\"");
  value.replace("\r", "\\r");
  value.replace("\n", "\\n");
  return value;
}

void handleWifiScan() {
  Serial.println("Scanning for nearby WiFi networks...");

  // Keep the AP alive while scanning. WIFI_AP_STA allows the ESP32
  // to host MindWave-Setup and scan/connect to nearby networks.
  if (WiFi.getMode() != WIFI_AP_STA) {
    WiFi.mode(WIFI_AP_STA);
  }

  int n = WiFi.scanNetworks(false, true);
  if (n < 0) n = 0;

  String json = "{\"current\":\"" + wifiJsonEscape(wifiSsid) + "\",\"networks\":[";

  bool first = true;
  for (int i = 0; i < n; i++) {
    String ssid = WiFi.SSID(i);
    if (ssid.length() == 0) continue;

    if (!first) json += ",";
    first = false;

    json += "{\"ssid\":\"" + wifiJsonEscape(ssid) + "\",\"rssi\":" + String(WiFi.RSSI(i)) + "}";
  }

  json += "]}";

  WiFi.scanDelete();
  server.send(200, "application/json", json);
}

/************************************************************
   📝 SETUP FORM PAGE  (served at "/" while portal is active,
   and at every unmatched captive-portal probe path)
************************************************************/
void handleWifiSetupRoot() {
  int n = WiFi.scanNetworks(false, true);

  String options = "";
  for (int i = 0; i < n; i++) {
    String ssid = WiFi.SSID(i);
    if (!ssid.length()) continue;
    String safe = ssid; safe.replace("&", "&amp;"); safe.replace("\"", "&quot;"); safe.replace("<", "&lt;"); safe.replace(">", "&gt;");
    options += "<option value=\"" + safe + "\"" + (ssid == wifiSsid ? " selected" : "") + ">" + safe + "  (" + String(WiFi.RSSI(i)) + " dBm)</option>";
  }

  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>MindWave - WiFi Setup</title>
<link href="https://fonts.googleapis.com/css2?family=Orbitron:wght@700;900&family=Rajdhani:wght@500;600;700&display=swap" rel="stylesheet">
<style>
  :root{--bg:#05070d;--panel:rgba(12,18,32,0.7);--border:rgba(0,200,255,0.25);--accent:#00c8ff;--green:#39ff6a;--text:#e0f0ff;--muted:#6d80a0;}
  *{margin:0;padding:0;box-sizing:border-box;}
  body{font-family:'Rajdhani',sans-serif;background:var(--bg);color:var(--text);min-height:100vh;
       display:flex;align-items:center;justify-content:center;padding:24px;}
  .card{width:100%;max-width:380px;background:var(--panel);border:1px solid var(--border);border-radius:16px;padding:28px;}
  h1{font-family:'Orbitron',sans-serif;font-size:18px;color:var(--accent);margin-bottom:4px;letter-spacing:1px;}
  p.sub{color:var(--muted);font-size:14px;margin-bottom:20px;}
  label{display:block;font-size:14px;color:var(--muted);margin:14px 0 6px;}
  select,input{width:100%;padding:12px;border-radius:10px;border:1px solid var(--border);
       background:#0b0f1c;color:var(--text);font-family:'Rajdhani',sans-serif;font-size:15px;}
  button{margin-top:22px;width:100%;padding:14px;border:none;border-radius:10px;
       background:var(--accent);color:#001018;font-weight:700;font-size:15px;letter-spacing:0.5px;cursor:pointer;}
  button:active{background:var(--green);}
  .password-wrap{position:relative;display:flex;align-items:center;}
  .password-wrap input{padding-right:78px;}
  .password-toggle{position:absolute;right:8px;top:50%;transform:translateY(-50%);
       margin:0!important;width:auto!important;padding:7px 10px!important;
       background:transparent!important;color:var(--accent)!important;
       border:1px solid var(--border)!important;border-radius:7px!important;
       font-size:11px!important;letter-spacing:.3px!important;}
  .password-setting{display:flex;align-items:center;gap:10px;justify-content:space-between;}
  .mini-toggle{margin:0!important;width:auto!important;padding:6px 10px!important;
       background:transparent!important;color:var(--accent)!important;
       border:1px solid var(--border)!important;border-radius:7px!important;
       font-size:11px!important;letter-spacing:.3px!important;}
  .settings-btn{margin-top:12px;background:transparent;color:var(--accent);border:1px solid var(--border);}
  .settings-overlay{display:none;position:fixed;inset:0;background:rgba(0,0,0,.78);
       backdrop-filter:blur(6px);z-index:1000;align-items:center;justify-content:center;padding:18px;}
  .settings-modal{width:100%;max-width:430px;max-height:90vh;overflow:auto;background:#0a101d;
       border:1px solid var(--border);border-radius:16px;padding:20px;box-shadow:0 20px 60px rgba(0,0,0,.6);}
  .settings-head{display:flex;justify-content:space-between;align-items:center;margin-bottom:16px;}
  .settings-title{font-family:'Orbitron',sans-serif;color:var(--accent);font-size:15px;letter-spacing:1.5px;}
  .settings-close{margin:0;width:auto;padding:0 6px;background:none;border:0;color:var(--muted);font-size:25px;}
  .setting-row{padding:10px 0;border-bottom:1px solid rgba(0,200,255,.10);}
  .setting-label{display:block;color:var(--muted);font-size:12px;margin-bottom:4px;}
  .setting-value{color:var(--text);font-size:14px;word-break:break-word;}
  .pin-title{margin-top:16px;color:var(--accent);font-family:'Orbitron',sans-serif;font-size:12px;letter-spacing:1px;}
  .pin-table{width:100%;border-collapse:collapse;margin-top:8px;font-size:13px;}
  .pin-table td{padding:7px 5px;border-bottom:1px solid rgba(0,200,255,.08);}
  .pin-table td:last-child{text-align:right;color:var(--accent);font-weight:700;}
  .settings-note{margin-top:12px;color:var(--muted);font-size:11px;line-height:1.45;}
</style>
</head>
<body>
  <div class="card">
    <h1>TBM FINISHING SCHOOL</h1>
    <p class="sub">Connect the device to your WiFi network</p>
    <form action="/wifi-save" method="POST">
      <label>Network</label>
      <select name="ssid_select" onchange="document.getElementById('ssid').value=this.value">
        <option value="">-- choose network --</option>
        )rawliteral" + options + R"rawliteral(
      </select>
      <label>Or type SSID manually</label>
      <input type="text" id="ssid" name="ssid" placeholder="WiFi name" value="__CURRENT_SSID__" autocomplete="off">
      <label>Password</label>
      <div class="password-wrap"><input type="password" name="pass" placeholder="WiFi password" autocomplete="off" id="wifiPassword"><button type="button" class="password-toggle" id="wifiPasswordToggle">SHOW</button></div>
      <button type="submit">Save &amp; Reboot</button>
    </form>
    <div id="scanStatus" style="margin-top:12px;text-align:center;color:#6d80a0;font-size:12px;">Scanning nearby networks…</div>
    <button type="button" class="settings-btn" id="settingsBtn">SETTINGS</button>
  </div>

  <div class="settings-overlay" id="settingsOverlay">
    <div class="settings-modal">
      <div class="settings-head">
        <div class="settings-title">DEVICE SETTINGS</div>
        <button type="button" class="settings-close" id="settingsClose">&times;</button>
      </div>

      <div class="setting-row">
        <span class="setting-label">CURRENT NETWORK</span>
        <div class="setting-value" id="setWifi">Loading...</div>
      </div>
      <div class="setting-row">
        <span class="setting-label">NETWORK PASSWORD</span>
        <div class="password-setting">
          <span class="setting-value" id="setWifiPass">••••••••••••</span>
          <button type="button" class="mini-toggle" id="wifiPassView">SHOW</button>
        </div>
      </div>
      <div class="setting-row">
        <span class="setting-label">GMAIL</span>
        <div class="setting-value" id="setGmail">Loading...</div>
      </div>
      <div class="setting-row">
        <span class="setting-label">GMAIL APP PASSWORD</span>
        <div class="password-setting">
          <span class="setting-value" id="setGmailPass">••••••••••••</span>
          <button type="button" class="mini-toggle" id="gmailPassView">SHOW</button>
        </div>
      </div>

      <div class="pin-title">HARDWARE PIN MAP</div>
      <table class="pin-table">
        <tr><td>GPIO13-WiFi Credential / Setup</td><td>GPIO 13</td></tr>
        <tr><td>Motor IN1</td><td>GPIO 26</td></tr>
        <tr><td>Motor IN2</td><td>GPIO 27</td></tr>
        <tr><td>Motor ENA / PWM</td><td>GPIO 14</td></tr>
        <tr><td>Relay</td><td>GPIO 25</td></tr>
      </table>

      <div class="settings-note">
        Read-only device information. Configuration changes are made through the WiFi setup form above.
      </div>
    </div>
  </div>

<script>
async function refreshNetworks(){
  const status=document.getElementById('scanStatus');
  try{const r=await fetch('/wifi-scan'); const d=await r.json(); const sel=document.querySelector('select[name="ssid_select"]');
    const current=d.current||''; sel.innerHTML='<option value="">-- choose network --</option>';
    d.networks.forEach(x=>{const o=document.createElement('option');o.value=x.ssid;o.textContent=x.ssid+'  ('+x.rssi+' dBm)';if(x.ssid===current)o.selected=true;sel.appendChild(o);});
    if(!document.getElementById('ssid').value) document.getElementById('ssid').value=current;
    status.textContent=d.networks.length+' WiFi network(s) found';
  }catch(e){status.textContent='Scan unavailable — enter SSID manually';}
}
refreshNetworks();

const settingsOverlay=document.getElementById('settingsOverlay');

async function loadSettings(){
  try{
    const r=await fetch('/settings-data');
    const d=await r.json();

    document.getElementById('setWifi').textContent=d.wifiSsid || 'Not configured';

    const wifiPass=d.wifiPass || '';
    const gmailPass=d.gmailPass || '';
    const wifiEl=document.getElementById('setWifiPass');
    const gmailEl=document.getElementById('setGmailPass');

    wifiEl.dataset.value=wifiPass;
    gmailEl.dataset.value=gmailPass;

    wifiEl.textContent=wifiPass ? '••••••••••••' : 'Not configured';
    gmailEl.textContent=gmailPass ? '••••••••••••' : 'Not configured';

    document.getElementById('setGmail').textContent=d.gmail || 'Not configured';
    document.getElementById('wifiPassView').textContent='SHOW';
    document.getElementById('gmailPassView').textContent='SHOW';
  }catch(e){
    document.getElementById('setWifi').textContent='Unable to load';
    document.getElementById('setWifiPass').textContent='Unable to load';
    document.getElementById('setGmail').textContent='Unable to load';
    document.getElementById('setGmailPass').textContent='Unable to load';
  }
}

document.getElementById('settingsBtn').onclick=()=>{
  settingsOverlay.style.display='flex';
  loadSettings();
};
document.getElementById('settingsClose').onclick=()=>{
  settingsOverlay.style.display='none';
};
settingsOverlay.addEventListener('click',e=>{
  if(e.target===settingsOverlay) settingsOverlay.style.display='none';
});
function setupSecretToggle(buttonId,valueId){
  const btn=document.getElementById(buttonId);
  const el=document.getElementById(valueId);
  if(!btn || !el) return;
  btn.onclick=()=>{
    const value=el.dataset.value || '';
    if(!value) return;
    const showing=btn.textContent==='HIDE';
    el.textContent=showing?'••••••••••••':value;
    btn.textContent=showing?'SHOW':'HIDE';
  };
}
setupSecretToggle('wifiPassView','setWifiPass');
setupSecretToggle('gmailPassView','setGmailPass');

const wifiPassword=document.getElementById('wifiPassword');
const wifiPasswordToggle=document.getElementById('wifiPasswordToggle');
if(wifiPassword && wifiPasswordToggle){
  wifiPasswordToggle.onclick=()=>{
    const hidden=wifiPassword.type==='password';
    wifiPassword.type=hidden?'text':'password';
    wifiPasswordToggle.textContent=hidden?'HIDE':'SHOW';
  };
}
</script>
</body>
</html>
)rawliteral";

  html.replace("__CURRENT_SSID__", wifiSsid);
  server.send(200, "text/html", html);
}

/************************************************************
   💾 HANDLE FORM SUBMIT
************************************************************/
void handleWifiSetupSave() {
  String newSsid = server.arg("ssid");
  String newPass = server.arg("pass");

  if (newSsid.length() == 0) {
    server.send(400, "text/plain", "SSID cannot be empty. Go back and try again.");
    return;
  }

  saveWifiCredentials(newSsid, newPass);

  String html = R"rawliteral(
<!DOCTYPE html><html><head><meta name="viewport" content="width=device-width, initial-scale=1">
<style>body{font-family:sans-serif;background:#05070d;color:#e0f0ff;display:flex;align-items:center;
justify-content:center;height:100vh;text-align:center;padding:24px;}</style></head>
<body><div><h2>Saved!</h2><p>Rebooting and connecting to your WiFi now.</p>
<p>You can close this page and reconnect to your normal WiFi.</p></div></body></html>
)rawliteral";

  server.send(200, "text/html", html);
  delay(1500);
  ESP.restart();
}

/************************************************************
   🚧 CAPTURE ALL UNMATCHED ROUTES WHILE PORTAL IS ACTIVE
   Makes phones/laptops auto-detect a captive portal and pop
   the setup page instead of requiring the user to find the IP.
************************************************************/
void handleWifiSetupNotFound() {
  if (wifiSetupPortalActive) {
    server.sendHeader("Location", "/", true);
    server.send(302, "text/plain", "");
  } else {
    server.send(404, "text/plain", "Not found");
  }
}
