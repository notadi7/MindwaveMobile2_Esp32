/************************************************************
   DASHBOARD_PAGE.INO
   The main dashboard ONLY: gauges, brainwave trend chart, LED,
   countdown timer, and live focus score. When the WebSocket
   frame carries sessionEnded:1, this page just redirects to
   "/" — the ESP32 is by then in STATE_RESULT, so "/" serves
   the separate, standalone result page (see Result_Page.ino).
************************************************************/

/************************************************************
   📊 MAIN DASHBOARD PAGE (gauges/trend/LED UI, countdown timer,
   live focus score). No result UI lives in this file anymore.
************************************************************/
void sendDashboardPage() {
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
    padding: 4px 20px 16px;
    min-height: calc(100vh - 228px);
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
    flex: 1;
    display: flex;
    flex-direction: column;
    justify-content: center;
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

  .trend-card { flex: 1; display: flex; flex-direction: column; }

  .trend-canvas-wrap {
    flex: 1;
    position: relative;
    min-height: 110px;
  }

  #trendGraph {
    position: absolute;
    inset: 0;
    width: 100%;
    height: 100%;
    display: block;
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

  /* ── MOTOR GAUGE ── */
  .motor-gauge-wrap {
    display: flex;
    flex-direction: column;
    align-items: center;
    position: relative;
  }
  #motorGauge { width: 100%; max-width: 240px; }
  .motor-value {
    font-family: 'Orbitron', monospace;
    font-size: 36px;
    font-weight: 900;
    color: var(--orange);
    text-shadow: 0 0 30px var(--orange);
    line-height: 1;
    margin-top: -20px;
  }
  .motor-sub { font-size: 11px; color: var(--muted); margin-top: 3px; letter-spacing: 1px; }
  .motor-status-row {
    display: flex;
    justify-content: space-between;
    margin-top: 10px;
    gap: 8px;
  }
  .motor-stat {
    flex: 1;
    text-align: center;
    padding: 8px 4px;
    border: 1px solid var(--border);
    border-radius: 10px;
    background: rgba(0,200,255,0.04);
  }
  .motor-stat-val {
    font-family: 'Orbitron', monospace;
    font-size: 13px;
    font-weight: 700;
    color: var(--orange);
  }
  .motor-stat-lbl { font-size: 9px; color: var(--muted); letter-spacing: 1px; margin-top: 2px; }

  /* ── LED INDICATOR ── */
  .led-card { text-align: center; flex: 1; display: flex; flex-direction: column; justify-content: center; }
  .led-wrap {
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 24px;
    padding: 14px 0 8px;
  }
  .led-bulb {
    position: relative;
    width: 64px; height: 64px;
  }
  .led-bulb svg { width: 100%; height: 100%; }
  .led-on-glow {
    position: absolute;
    inset: -8px;
    border-radius: 50%;
    background: radial-gradient(circle, rgba(255,220,0,0.5) 0%, transparent 70%);
    opacity: 0;
    transition: opacity 0.4s;
    pointer-events: none;
  }
  .led-bulb.on .led-on-glow { opacity: 1; }
  .led-info { text-align: left; }
  .led-label {
    font-family: 'Orbitron', monospace;
    font-size: 12px;
    letter-spacing: 2px;
    color: var(--muted);
    margin-bottom: 4px;
  }
  .led-state {
    font-family: 'Orbitron', monospace;
    font-size: 20px;
    font-weight: 900;
    letter-spacing: 2px;
    transition: color 0.4s, text-shadow 0.4s;
  }
  .led-state.on  { color: #ffe040; text-shadow: 0 0 20px #ffe040; }
  .led-state.off { color: var(--muted); text-shadow: none; }
  .led-sub { font-size: 10px; color: var(--muted); margin-top: 3px; letter-spacing: 1px; }
  @keyframes ledPulse { 0%,100%{opacity:1} 50%{opacity:0.6} }
  .led-bulb.on { animation: ledPulse 1.5s infinite; }

  /* ── LIVE STATUS DOT ── */
  .dot-live {
    width: 7px; height: 7px;
    border-radius: 50%;
    background: #00c8ff;
    box-shadow: 0 0 7px #00c8ff;
    flex: 0 0 auto;
    animation: pulse 1.8s infinite;
  }

  /* ── SESSION STATUS BAR ── */
  .session-bar {
    position: relative;
    display: flex;
    align-items: stretch;
    margin: 18px 20px 14px;
    padding: 16px 26px;
    background: linear-gradient(180deg, rgba(0,200,255,0.07), rgba(12,18,32,0.85));
    border: 1px solid var(--border);
    border-radius: 16px;
    overflow: hidden;
  }
  .session-bar::before {
    content: '';
    position: absolute;
    top: 0; left: 0; right: 0; height: 2px;
    background: linear-gradient(90deg, var(--accent), var(--green), var(--orange));
    opacity: 0.7;
  }
  .sb-item { flex: 1; display: flex; align-items: center; gap: 16px; min-width: 0; }
  .sb-icon {
    flex: 0 0 auto;
    width: 50px; height: 50px;
    border-radius: 13px;
    display: flex; align-items: center; justify-content: center;
    background: rgba(0,200,255,0.08);
    border: 1px solid var(--border);
  }
  .sb-icon svg { width: 24px; height: 24px; }
  .sb-text { flex: 1; min-width: 0; }
  .sb-label {
    font-family: 'Orbitron', monospace;
    font-size: 10px;
    letter-spacing: 2.5px;
    color: var(--muted);
    margin-bottom: 4px;
  }
  .sb-value {
    font-family: 'Orbitron', monospace;
    font-size: 28px;
    font-weight: 900;
    line-height: 1;
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
  }
  .sb-sub { font-size: 11px; color: var(--muted); margin-top: 5px; letter-spacing: 0.5px; }
  .sb-mini-progress {
    margin-top: 9px;
    height: 5px;
    border-radius: 4px;
    background: rgba(255,255,255,0.07);
    overflow: hidden;
  }
  .sb-mini-progress-fill {
    height: 100%;
    width: 0%;
    border-radius: 4px;
    background: linear-gradient(90deg, var(--accent), var(--green));
    transition: width 1s linear;
  }
  .sb-divider { width: 1px; align-self: stretch; background: var(--border); margin: 0 28px; }

  @media (max-width: 980px) {
    .session-bar { flex-direction: column; gap: 18px; padding: 18px 22px; }
    .sb-divider { display: none; }
  }
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
      <h1>MINDWAVE</h1>
      <p>ATTENTION MONITOR</p>
    </div>
  </div>
  <div class="header-center" style="text-align:center;line-height:1.3;">
    <div style="font-family:'Orbitron',monospace;font-size:15px;font-weight:900;letter-spacing:3px;color:white;text-shadow:0 0 15px rgba(0,200,255,0.5);">TBM FINISHING SCHOOL</div>
    <div style="font-family:'Rajdhani',sans-serif;font-size:11px;letter-spacing:2px;color:var(--accent);opacity:0.8;margin-top:2px;">Nilesh Gunjal  (Psychologist)</div>
    <div style="font-family:'Orbitron',monospace;font-size:12px;color:var(--muted);margin-top:3px;" id="clock">--:--:--</div>
  </div>
   <div class="header-right">
    <div class="status-badge">
      <div class="dot-live"></div>
      <svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="#00c8ff" stroke-width="2">
        <path d="M5 12.55a11 11 0 0 1 14.08 0"/><path d="M1.42 9a16 16 0 0 1 21.16 0"/>
        <path d="M8.53 16.11a6 6 0 0 1 6.95 0"/><circle cx="12" cy="20" r="1" fill="#00c8ff"/>
      </svg>WiFi
    </div>
    <div class="status-badge" style="color:#bf5fff;border-color:rgba(191,95,255,0.2);">
      <div class="dot-live" style="background:#bf5fff;box-shadow:0 0 7px #bf5fff;"></div>
      <svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="#bf5fff" stroke-width="2">
        <path d="M6.5 6.5A7 7 0 0 0 5 10a7 7 0 0 0 7 7 7 7 0 0 0 3.5-.9"/>
        <path d="M6.5 6.5A7 7 0 0 1 17.5 17.5"/><path d="M12 2v4M12 18v4M2 12h4M18 12h4"/>
      </svg>BT
    </div>
  </div>
</div>

<!-- SESSION STATUS BAR (the two numbers that matter most, given real room to breathe) -->
<div class="session-bar">
  <div class="sb-item">
    <div class="sb-icon">
      <svg viewBox="0 0 24 24" fill="none" stroke="var(--accent)" stroke-width="1.8"><circle cx="12" cy="12" r="9"/><path d="M12 7v5l3 3"/></svg>
    </div>
    <div class="sb-text">
      <div class="sb-label">SESSION TIME REMAINING</div>
      <div class="sb-value" id="countdownVal" style="color:var(--green);text-shadow:0 0 16px var(--green);">--:--</div>
      <div class="sb-sub">of __SESSION_MINUTES__ min session</div>
      <div class="sb-mini-progress"><div class="sb-mini-progress-fill" id="sessionProgressFill"></div></div>
    </div>
  </div>

  <div class="sb-divider"></div>

  <div class="sb-item">
    <div class="sb-icon">
      <svg viewBox="0 0 24 24" fill="none" stroke="var(--orange)" stroke-width="1.8"><path d="M13 2 3 14h7l-1 8 10-12h-7l1-8z"/></svg>
    </div>
    <div class="sb-text">
      <div class="sb-label">MOTOR-ON TIME</div>
      <div class="sb-value" id="focusScoreVal" style="color:var(--orange);text-shadow:0 0 16px var(--orange);">0s</div>
      <div class="sb-sub">Total active runtime this session</div>
    </div>
  </div>

  <div class="sb-divider"></div>

  <div class="sb-item">
    <div class="sb-icon">
      <svg viewBox="0 0 24 24" fill="none" stroke="var(--accent)" stroke-width="1.8"><circle cx="12" cy="8" r="4"/><path d="M4 20c0-4 4-6 8-6s8 2 8 6"/></svg>
    </div>
    <div class="sb-text">
      <div class="sb-label">PARTICIPANT</div>
      <div class="sb-value" style="color:var(--accent);text-shadow:0 0 16px var(--accent);">__USER_NAME__</div>
      <div class="sb-sub">__SESSION_MINUTES__ min session</div>
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
          <span id="focusLabel">WAITING...</span>
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
      <div class="trend-canvas-wrap">
        <canvas id="trendGraph"></canvas>
      </div>
      <div class="trend-labels">
        <span>30s ago</span><span>15s</span><span>NOW</span>
      </div>
      <div style="display:flex;justify-content:space-between;margin-top:12px;font-size:11px;color:var(--muted);">
        <div style="text-align:center;">
          <div style="font-family:'Orbitron',monospace;color:var(--green);font-size:14px;" id="trendMax">0</div>
          <div>PEAK</div>
        </div>
        <div style="text-align:center;">
          <div style="font-family:'Orbitron',monospace;color:var(--accent);font-size:14px;" id="trendAvg">0</div>
          <div>AVG</div>
        </div>
        <div style="text-align:center;">
          <div style="font-family:'Orbitron',monospace;color:var(--yellow);font-size:14px;" id="trendMin">0</div>
          <div>MIN</div>
        </div>
      </div>
    </div>

  </div>

  <!-- RIGHT COLUMN -->
  <div class="right-col">

    <!-- MOTOR SPEED GAUGE -->
    <div class="card">
      <div class="card-corner tl"></div><div class="card-corner tr"></div>
      <div class="card-corner bl"></div><div class="card-corner br"></div>
      <div class="card-title" style="justify-content:center;">MOTOR SPEED</div>
      <div class="motor-gauge-wrap">
        <canvas id="motorGauge" height="150"></canvas>
        <div class="motor-value" id="motorVal">0</div>
        <div class="motor-sub">% SPEED</div>
      </div>
      <div class="motor-status-row">
        <div class="motor-stat">
          <div class="motor-stat-val" id="motorPWM">0</div>
          <div class="motor-stat-lbl">PWM</div>
        </div>
        <div class="motor-stat">
          <div class="motor-stat-val" id="motorPeak">0</div>
          <div class="motor-stat-lbl">PEAK%</div>
        </div>
        <div class="motor-stat">
          <div class="motor-stat-val" id="motorState" style="color:var(--muted);">IDLE</div>
          <div class="motor-stat-lbl">STATE</div>
        </div>
      </div>
    </div>

    <!-- RELAY / LED INDICATOR -->
    <div class="card led-card">
      <div class="card-corner tl"></div><div class="card-corner tr"></div>
      <div class="card-corner bl"></div><div class="card-corner br"></div>
      <div class="card-title" style="justify-content:center;">LIGHT INDICATOR</div>
      <div class="led-wrap">
        <div class="led-bulb" id="ledBulb">
          <div class="led-on-glow"></div>
          <svg viewBox="0 0 64 80" xmlns="http://www.w3.org/2000/svg">
            <defs>
              <radialGradient id="bulbGrad" cx="50%" cy="40%" r="55%">
                <stop offset="0%" stop-color="#fff9c4" stop-opacity="1"/>
                <stop offset="60%" stop-color="#ffe040" stop-opacity="0.9"/>
                <stop offset="100%" stop-color="#ff8f00" stop-opacity="0.7"/>
              </radialGradient>
              <radialGradient id="bulbOff" cx="50%" cy="40%" r="55%">
                <stop offset="0%" stop-color="#3a4050"/>
                <stop offset="100%" stop-color="#1a1e2a"/>
              </radialGradient>
            </defs>
            <!-- Bulb glass -->
            <path id="bulbGlass" d="M32 6 C16 6 8 18 8 28 C8 38 14 46 20 52 L20 58 C20 60 22 62 24 62 L40 62 C42 62 44 60 44 58 L44 52 C50 46 56 38 56 28 C56 18 48 6 32 6Z"
              fill="url(#bulbOff)" stroke="rgba(255,255,255,0.1)" stroke-width="1"/>
            <!-- Base -->
            <rect x="22" y="62" width="20" height="5" rx="2" fill="#555"/>
            <rect x="23" y="67" width="18" height="4" rx="2" fill="#444"/>
            <rect x="24" y="71" width="16" height="4" rx="2" fill="#333"/>
            <!-- Filament -->
            <path id="filament" d="M27 45 L27 35 Q32 30 37 35 L37 45" fill="none" stroke="#aaa" stroke-width="1.5" stroke-linecap="round"/>
            <!-- Shine -->
            <ellipse cx="26" cy="22" rx="5" ry="7" fill="rgba(255,255,255,0.08)"/>
          </svg>
        </div>
        <div class="led-info">
          <div class="led-label">LIGHT STATUS</div>
          <div class="led-state off" id="ledState">OFF</div>
          <div class="led-sub" id="ledSub">RELAY HIGH · INACTIVE</div>
        </div>
      </div>
      <!-- threshold note -->
      <div style="border-top:1px solid rgba(255,255,255,0.05);padding-top:10px;margin-top:4px;">
        <div style="display:flex;justify-content:space-between;font-size:11px;color:var(--muted);letter-spacing:1px;">
          <span>LIGHT ON</span><span style="color:var(--accent);font-family:'Orbitron',monospace;">ATT &gt; 40</span>
        </div>
        <div style="display:flex;justify-content:space-between;font-size:11px;color:var(--muted);letter-spacing:1px;margin-top:6px;">
          <span>MOTOR START</span><span style="color:var(--orange);font-family:'Orbitron',monospace;">ATT &gt; 60</span>
        </div>
      </div>
    </div>

  </div>

</div>

<script>
// ── SESSION CONTEXT (filled in by the ESP32 at page render time) ──
const SESSION_USER_NAME = "__USER_NAME__";
const SESSION_MINUTES = __SESSION_MINUTES__;

// ── CONFIG ──
const WS_PORT = 81;
const TOTAL_SEGS = 20;

// ── STATE ──
let trendData = [];
let waveData = { delta:[], theta:[], alpha:[], beta:[], gamma:[] };

// Smooth animated values
let attentionCurrent = 0;
let attentionTarget = 0;

let motorCurrent = 0;
let motorTarget = 0;
let signalCurrent = 0;
let signalTarget = 0;
// ── WEBSOCKET ──
let websocket;
function connectWS() {
  let gateway = `ws://${window.location.hostname}:${WS_PORT}/`;
  websocket = new WebSocket(gateway);
  websocket.onmessage = function(e) {
    try {
      let d = JSON.parse(e.data);
      updateAll(d);
    } catch(err) {}
  };
  websocket.onclose = () => setTimeout(connectWS, 2000);
}

function updateAll(d) {

  if(d.connected === 0){
setWaitingMode();
return;
}
  // Brainwaves
  setWave('Delta', d.delta || 0, '#4dd9ff');
  setWave('Theta', d.theta || 0, '#00e5d4');
  setWave('Alpha', d.alpha || 0, '#39ff6a');
  setWave('Beta',  d.beta  || 0, '#ffe600');
  setWave('Gamma', d.gamma || 0, '#bf5fff');

  // Signal
  let sigVal = d.signal !== undefined ? d.signal : 0;
signalTarget = sigVal;

  // Attention gauge
  let att = d.attention || 0;
attentionTarget = att;
setFocusBadge(att);
  // Trend
  trendData.push(att);
  if (trendData.length > 30) trendData.shift();
  drawTrend();
  updateTrendStats();

  // Motor gauge
  let spd = d.speed !== undefined ? d.speed : 0;
motorTarget = spd;

  // LED / Relay
  let relayOn = d.relay !== undefined ? d.relay : (att > 40 ? 1 : 0);
  updateLED(relayOn);

  // Countdown timer (server-authoritative — survives refresh / 2nd tab)
  if (d.remaining !== undefined) {
    let rem = Math.max(0, d.remaining|0);
    let mm = String(Math.floor(rem / 60)).padStart(2, '0');
    let ss = String(rem % 60).padStart(2, '0');
    document.getElementById('countdownVal').textContent = mm + ':' + ss;

    let totalSec = SESSION_MINUTES * 60;
    let pct = totalSec > 0 ? Math.min(100, Math.max(0, ((totalSec - rem) / totalSec) * 100)) : 0;
    document.getElementById('sessionProgressFill').style.width = pct + '%';
  }

  // Live focus score (motor-on seconds accumulated this session)
  if (d.motorOnSeconds !== undefined) {
    document.getElementById('focusScoreVal').textContent = d.motorOnSeconds + 's';
  }

  // Session end -> hand off to the standalone result page (only once).
  // appState is already STATE_RESULT server-side by the time this frame
  // arrives, so "/" will now render Result_Page.ino instead of this page.
  if (d.sessionEnded === 1 && !resultRedirectStarted) {
    resultRedirectStarted = true;
    window.location.href = '/';
  }
}

let resultRedirectStarted = false;

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
  if (val >= 80)      { label = 'EXCELLENT!'; color = '#00c8ff'; }
  else if (val >= 60) { label = 'HIGH-Great Focus!'; color = '#39ff6a'; }
  else if (val >= 40) { label = 'MODERATE'; color = '#ffe600'; }
  else if (val >= 20) { label = 'LOW'; color = '#ff8c00'; }
  else                { label = 'VERY LOW'; color = '#ff3b5c'; }
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
  let cssW = canvas.clientWidth, cssH = canvas.clientHeight;
  if (cssW > 0 && cssH > 0 && (canvas.width !== cssW || canvas.height !== cssH)) {
    canvas.width = cssW;
    canvas.height = cssH;
  }
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

// ── MOTOR GAUGE ──
let motorPeakVal = 0;
function drawMotorGauge(value) {
  let canvas = document.getElementById('motorGauge');
  let ctx = canvas.getContext('2d');
  let W = canvas.width, H = canvas.height;
  ctx.clearRect(0, 0, W, H);

  let cx = W / 2, cy = H * 0.78, r = Math.min(W, H) * 0.52;

  // Background arc
  ctx.beginPath();
  ctx.arc(cx, cy, r, Math.PI, 0);
  ctx.lineWidth = 14;
  ctx.strokeStyle = 'rgba(255,140,0,0.1)';
  ctx.stroke();

  // Speed zones
  let zones = [
    { start: 0,    end: 0.33, color: '#39ff6a' },
    { start: 0.33, end: 0.66, color: '#ffe600' },
    { start: 0.66, end: 1.0,  color: '#ff8c00' }
  ];
  let frac = value / 100;
  zones.forEach(z => {
    if (frac <= z.start) return;
    let ef = Math.min(frac, z.end);
    ctx.beginPath();
    ctx.arc(cx, cy, r, Math.PI + z.start * Math.PI, Math.PI + ef * Math.PI);
    ctx.lineWidth = 14;
    ctx.strokeStyle = z.color;
    ctx.shadowBlur = 12;
    ctx.shadowColor = z.color;
    ctx.stroke();
    ctx.shadowBlur = 0;
  });

  // Tick marks
  for (let i = 0; i <= 8; i++) {
    let a = Math.PI + (i / 8) * Math.PI;
    let r1 = r - 10, r2 = r + 3;
    ctx.beginPath();
    ctx.moveTo(cx + r1 * Math.cos(a), cy + r1 * Math.sin(a));
    ctx.lineTo(cx + r2 * Math.cos(a), cy + r2 * Math.sin(a));
    ctx.lineWidth = i % 4 === 0 ? 2 : 1;
    ctx.strokeStyle = 'rgba(255,255,255,0.2)';
    ctx.shadowBlur = 0;
    ctx.stroke();
  }

  // Needle
  let na = Math.PI + frac * Math.PI;
  ctx.beginPath();
  ctx.moveTo(cx, cy);
  ctx.lineTo(cx + (r - 18) * Math.cos(na), cy + (r - 18) * Math.sin(na));
  ctx.lineWidth = 3;
  ctx.strokeStyle = '#fff';
  ctx.shadowBlur = 8;
  ctx.shadowColor = '#fff';
  ctx.stroke();
  ctx.shadowBlur = 0;

  ctx.beginPath();
  ctx.arc(cx, cy, 5, 0, Math.PI * 2);
  ctx.fillStyle = '#fff';
  ctx.fill();

  // Update labels
  if (value > motorPeakVal) motorPeakVal = value;
  document.getElementById('motorVal').textContent = value;
  document.getElementById('motorPWM').textContent = Math.round(value * 1.5); // approx PWM 0-150
  document.getElementById('motorPeak').textContent = motorPeakVal;
  let stateEl = document.getElementById('motorState');
  if (value === 0)      { stateEl.textContent = 'IDLE';  stateEl.style.color = 'var(--muted)'; }
  else if (value < 40)  { stateEl.textContent = 'SLOW';  stateEl.style.color = '#39ff6a'; }
  else if (value < 75)  { stateEl.textContent = 'MED';   stateEl.style.color = '#ffe600'; }
  else                  { stateEl.textContent = 'FAST';  stateEl.style.color = '#ff8c00'; }
}
function animateValues() {

  // Attention exact step-by-step
  if (attentionCurrent < attentionTarget) {
    attentionCurrent++;
  }
  else if (attentionCurrent > attentionTarget) {
    attentionCurrent--;
  }

  // Motor exact step-by-step
  if (motorCurrent < motorTarget) {
    motorCurrent++;
  }
  else if (motorCurrent > motorTarget) {
    motorCurrent--;
  }

  // Signal exact step-by-step
if (signalCurrent < signalTarget) {
  signalCurrent++;
}
else if (signalCurrent > signalTarget) {
  signalCurrent--;
}

  // Draw gauges
  drawAttentionGauge(attentionCurrent);
  drawMotorGauge(motorCurrent);

  // Update display values
  document.getElementById('attVal').textContent = attentionCurrent;
  document.getElementById('motorVal').textContent = motorCurrent;

  
  updateSignal(signalCurrent);
  requestAnimationFrame(animateValues);
}
//WAITING UPDATE
function setWaitingMode(){

attentionTarget = 0;
motorTarget = 0;
signalTarget = 0;

trendData = [];

waveData.delta = [];
waveData.theta = [];
waveData.alpha = [];
waveData.beta = [];
waveData.gamma = [];

document.getElementById('focusLabel').innerText = "WAITING";
}
// ── LED / RELAY ──
function updateLED(isOn) {
  let bulb   = document.getElementById('ledBulb');
  let glass  = document.getElementById('bulbGlass');
  let fil    = document.getElementById('filament');
  let state  = document.getElementById('ledState');
  let sub    = document.getElementById('ledSub');

  if (isOn) {
    bulb.classList.add('on');
    glass.setAttribute('fill', 'url(#bulbGrad)');
    fil.setAttribute('stroke', '#ffe040');
    state.textContent = 'ON';
    state.className = 'led-state on';
    sub.textContent = 'RELAY LOW  ACTIVE';
  } else {
    bulb.classList.remove('on');
    glass.setAttribute('fill', 'url(#bulbOff)');
    fil.setAttribute('stroke', '#aaa');
    state.textContent = 'OFF';
    state.className = 'led-state off';
    sub.textContent = 'RELAY HIGH  INACTIVE';
  }
}

// ── CLOCK ──
function updateClock() {
  let now = new Date();
  document.getElementById('clock').textContent =
    now.toLocaleTimeString('en-US', {hour:'2-digit',minute:'2-digit',second:'2-digit'});
}
setInterval(updateClock, 1000);
updateClock();

// ── INIT ──
updateSignal(0);
updateLED(0);
// Start smooth animation
animateValues();

// Keep the trend chart crisp and fully filling its card on layout changes
window.addEventListener('resize', drawTrend);

// ── CONNECT ──
connectWS();

</script>
</body>
</html>
)rawliteral";

html.replace("__USER_NAME__", userName);
html.replace("__SESSION_MINUTES__", String(sessionMinutes));

server.send(200,"text/html",html);
}
