/************************************************************
   RESULT_PAGE.INO
   The standalone "session complete" result page. Served at
   "/" (and explicitly at "/result") whenever
   systemState == STATE_SESSION_ENDED (see handleRoot() in
   Setup_Page.ino) — completely separate from the live
   dashboard in Dashboard_Page.ino. Shows the frozen session
   numbers (set once in endSession(), Session_Manager.ino),
   lets the user email the result, or start a new session.
   These values are NEVER recalculated here — this page does
   not depend on live EEG data in any way (req. #16/#17).
************************************************************/
void sendResultPage() {
String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>MindWave - Session Complete</title>
<link href="https://fonts.googleapis.com/css2?family=Orbitron:wght@400;700;900&family=Rajdhani:wght@400;500;600;700&display=swap" rel="stylesheet">
<style>
  :root {
    --bg: #05070d;
    --panel: rgba(12,18,32,0.55);
    --panel2: #0a0f1d;
    --border: rgba(0,200,255,0.18);
    --accent: #00c8ff;
    --green: #39ff6a;
    --text: #e0f0ff;
    --muted: #6d80a0;
    --red: #ff4d6a;
  }
  * { margin:0; padding:0; box-sizing:border-box; }
  html, body { height:100%; }
  body {
    font-family:'Rajdhani',sans-serif;
    background: var(--bg);
    color: var(--text);
    overflow-x:hidden;
    position:relative;
    padding-bottom:40px;
  }

  /* ── AMBIENT BACKGROUND ── */
  .bg-glow {
    position:fixed; inset:0; z-index:0; pointer-events:none;
    background:
      radial-gradient(600px 400px at 10% 15%, rgba(0,200,255,0.07), transparent 60%),
      radial-gradient(700px 500px at 90% 80%, rgba(57,255,106,0.06), transparent 60%),
      radial-gradient(500px 500px at 92% 10%, rgba(0,200,255,0.05), transparent 55%);
  }

  .wrap { position:relative; z-index:1; max-width:1180px; margin:0 auto; padding:0 28px; min-height:100vh; display:flex; flex-direction:column; justify-content:center; }

  .headline { text-align:center; padding:6px 0 26px; }
  .eyebrow { display:flex; align-items:center; justify-content:center; gap:12px; font-size:11px; letter-spacing:4px; color:var(--muted); margin-bottom:10px; }
  .eyebrow .line { width:38px; height:1px; background:var(--border); }
  .headline h1 {
    font-family:'Orbitron',monospace; font-weight:900; font-size:38px; letter-spacing:6px;
    color:#fff;
  }
  .headline p { margin-top:10px; font-size:15px; color:var(--muted); }

  /* ── INFO CARDS ── */
  .info-row { display:grid; grid-template-columns:repeat(3, 1fr); gap:16px; margin-bottom:26px; }
  .info-card {
    display:flex; align-items:center; gap:14px;
    background: var(--panel);
    backdrop-filter: blur(14px); -webkit-backdrop-filter: blur(14px);
    border:1px solid var(--border);
    border-radius:16px;
    padding:16px 18px;
  }
  .info-icon {
    flex:0 0 auto; width:46px; height:46px; border-radius:12px;
    display:flex; align-items:center; justify-content:center;
    background:rgba(0,200,255,0.08);
    border:1px solid var(--border);
    color:var(--accent);
  }
  .info-icon svg { width:22px; height:22px; }
  .info-label { font-size:10.5px; letter-spacing:2px; color:var(--muted); margin-bottom:3px; }
  .info-val { font-size:17px; font-weight:600; color:var(--text); word-break:break-word; }

  /* ── SECTION HEADER ── */
  .section-head { display:flex; align-items:baseline; justify-content:space-between; margin-bottom:14px; }
  .section-title { font-family:'Orbitron',monospace; font-size:14px; font-weight:700; letter-spacing:2px; color:var(--text); }
  .section-tag { font-size:10.5px; letter-spacing:2px; color:var(--muted); }

  /* ── PERFORMANCE SUMMARY ── */
  .stat-row { display:grid; grid-template-columns:repeat(4, 1fr); gap:16px; margin-bottom:26px; }
  .stat-card {
    position:relative; overflow:hidden;
    background: var(--panel);
    backdrop-filter: blur(14px); -webkit-backdrop-filter: blur(14px);
    border:1px solid var(--border);
    border-radius:16px;
    padding:18px;
  }
  .stat-card::before {
    content:''; position:absolute; top:0; left:16px; right:16px; height:2px;
    background:linear-gradient(90deg, var(--accent), var(--green));
    opacity:0.7; border-radius:2px;
  }
  .stat-icon {
    width:38px; height:38px; border-radius:10px;
    display:flex; align-items:center; justify-content:center;
    background:rgba(0,200,255,0.08);
    color:var(--accent);
    margin-bottom:14px;
  }
  .stat-icon svg { width:19px; height:19px; }
  .stat-label { font-size:10.5px; letter-spacing:1.5px; color:var(--muted); margin-bottom:6px; }
  .stat-val { font-family:'Orbitron',monospace; font-size:26px; font-weight:900; color:var(--accent); text-shadow:0 0 20px rgba(0,200,255,0.35); }
  .stat-val .unit { font-size:14px; font-weight:700; color:var(--muted); text-shadow:none; margin-left:2px; }
  .stat-sub { font-size:9.5px; letter-spacing:1.5px; color:var(--muted); margin-top:4px; }

  /* ── BRAINWAVE CARD ── */
  .wave-card {
    background: var(--panel);
    backdrop-filter: blur(14px); -webkit-backdrop-filter: blur(14px);
    border:1px solid var(--border);
    border-radius:16px;
    padding:22px 24px;
    margin-bottom:26px;
  }
  .wave-row { display:flex; align-items:center; gap:14px; padding:9px 0; }
  .wave-label { display:flex; align-items:center; gap:10px; width:150px; flex:0 0 auto; }
  .wave-dot { width:10px; height:10px; border-radius:50%; box-shadow:0 0 8px currentColor; flex:0 0 auto; }
  .wave-name { font-weight:700; font-size:14.5px; letter-spacing:1px; }
  .wave-hz { font-size:10.5px; color:var(--muted); }
  .wave-track { flex:1; height:9px; background:rgba(255,255,255,0.06); border-radius:6px; overflow:hidden; }
  .wave-fill { height:100%; border-radius:6px; }
  .wave-pct { font-family:'Orbitron',monospace; font-size:14px; font-weight:700; width:48px; text-align:right; flex:0 0 auto; }

  /* ── QUOTE ── */
  .quote-card {
    text-align:center;
    border:1px solid var(--border);
    border-radius:16px;
    padding:22px;
    margin-bottom:22px;
    background:rgba(6,10,20,0.35);
  }
  .quote-card .q-mark { font-family:'Orbitron',monospace; font-size:20px; color:var(--accent); margin-right:6px; }
  .quote-card .q-text { font-size:17px; font-weight:600; color:var(--text); }
  .quote-card .q-sub { margin-top:8px; font-size:10.5px; letter-spacing:2px; color:var(--muted); }

  /* ── ACTIONS ── */
  .actions { display:flex; gap:16px; }
  .btn {
    flex:1; padding:16px; border-radius:12px; cursor:pointer;
    font-family:'Orbitron',monospace; font-weight:900; letter-spacing:1.5px; font-size:13px;
    display:flex; align-items:center; justify-content:center; gap:10px;
    border:none;
  }
  .btn svg { width:17px; height:17px; }
  .btn.primary { background:linear-gradient(90deg, var(--accent), var(--green)); color:#04060c; }
  .btn.secondary { background:transparent; border:1px solid var(--border); color:var(--text); }
  .btn.accent {
    background:transparent;
    border:1px solid rgba(0,200,255,0.55);
    color:var(--accent);
  }
  .btn.accent:hover { background:rgba(0,200,255,0.12); box-shadow:0 0 18px rgba(0,200,255,0.25); }
  .btn:disabled { opacity:0.6; cursor:not-allowed; }
  .email-status { text-align:center; font-size:12px; letter-spacing:1px; margin-top:12px; min-height:16px; }
  .email-status.ok { color:var(--green); }
  .email-status.err { color:var(--red); }

  .footer-tag { text-align:left; font-size:11px; letter-spacing:2px; color:var(--muted); line-height:1.7; margin-top:22px; }
  .footer-tag .bar { width:44px; height:2px; background:linear-gradient(90deg, var(--accent), var(--green)); margin-bottom:10px; }

  @media (max-width: 900px) {
    .info-row { grid-template-columns:1fr; }
    .stat-row { grid-template-columns:repeat(2, 1fr); }
    .actions { flex-direction:column; }
  }
</style>
</head>
<body>
<div class="bg-glow"></div>

<div class="wrap">

  <!-- HEADLINE -->
  <div class="headline">
    <div class="eyebrow"><div class="line"></div>FOCUS SESSION<div class="line"></div></div>
    <h1>SESSION COMPLETE</h1>
    <p>Well done! You have successfully completed your focus session.</p>
  </div>

  <!-- INFO CARDS -->
  <div class="info-row">
    <div class="info-card">
      <div class="info-icon">
        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8"><circle cx="12" cy="8" r="4"/><path d="M4 20c0-4 4-6 8-6s8 2 8 6"/></svg>
      </div>
      <div>
        <div class="info-label">NAME</div>
        <div class="info-val">__USER_NAME__</div>
      </div>
    </div>
    <div class="info-card">
      <div class="info-icon">
        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8"><rect x="3" y="5" width="18" height="14" rx="2"/><path d="M3 7l9 6 9-6"/></svg>
      </div>
      <div>
        <div class="info-label">EMAIL</div>
        <div class="info-val">__USER_EMAIL__</div>
      </div>
    </div>
    <div class="info-card">
      <div class="info-icon">
        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8"><circle cx="12" cy="12" r="9"/><path d="M12 7v5l3 3"/></svg>
      </div>
      <div>
        <div class="info-label">SESSION TIME</div>
        <div class="info-val">__SESSION_MINUTES__ min</div>
      </div>
    </div>
  </div>

  <!-- PERFORMANCE SUMMARY -->
  <div class="section-head">
    <div class="section-title">PERFORMANCE SUMMARY</div>
    <div class="section-tag">FOCUS TODAY. A BETTER TOMORROW.</div>
  </div>
  <div class="stat-row">
    <div class="stat-card">
      <div class="stat-icon"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8"><circle cx="12" cy="12" r="9"/><path d="M12 7v5l3 3"/></svg></div>
      <div class="stat-label">TOTAL TIME</div>
      <div class="stat-val">__TOTAL_TIME__</div>
      <div class="stat-sub">MIN : SEC</div>
    </div>
    <div class="stat-card">
      <div class="stat-icon"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.6"><path d="M9 4a3 3 0 0 0-3 3 3 3 0 0 0-2 5 3 3 0 0 0 2 5 3 3 0 0 0 3 3V4z"/><path d="M15 4a3 3 0 0 1 3 3 3 3 0 0 1 2 5 3 3 0 0 1-2 5 3 3 0 0 1-3 3V4z"/></svg></div>
      <div class="stat-label">AVG ATTENTION</div>
      <div class="stat-val">__AVG_ATTENTION__<span class="unit">%</span></div>
    </div>
    <div class="stat-card">
      <div class="stat-icon"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M3 17l6-6 4 4 8-8M21 7h-6v6"/></svg></div>
      <div class="stat-label">PEAK ATTENTION</div>
      <div class="stat-val">__PEAK_ATTENTION__<span class="unit">%</span></div>
    </div>
    <div class="stat-card">
      <div class="stat-icon"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M13 2 3 14h7l-1 8 10-12h-7l1-8z"/></svg></div>
      <div class="stat-label">MOTOR ON TIME</div>
      <div class="stat-val">__MOTOR_ON_TIME__</div>
      <div class="stat-sub">MIN : SEC</div>
    </div>
  </div>

  <!-- BRAINWAVE ACTIVITY -->
  <div class="wave-card">
    <div class="section-head" style="margin-bottom:6px;">
      <div class="section-title">BRAINWAVE ACTIVITY (AVERAGE)</div>
      <div class="section-tag">MENTAL ACTIVITY OVERVIEW</div>
    </div>

    <div class="wave-row">
      <div class="wave-label">
        <div class="wave-dot" style="color:#4dd9ff;background:#4dd9ff;"></div>
        <div>
          <div class="wave-name">DELTA</div>
          <div class="wave-hz">0.5 &ndash; 4 Hz</div>
        </div>
      </div>
      <div class="wave-track"><div class="wave-fill" style="width:__DELTA__%;background:#4dd9ff;"></div></div>
      <div class="wave-pct" style="color:#4dd9ff;">__DELTA__%</div>
    </div>

    <div class="wave-row">
      <div class="wave-label">
        <div class="wave-dot" style="color:#00e5d4;background:#00e5d4;"></div>
        <div>
          <div class="wave-name">THETA</div>
          <div class="wave-hz">4 &ndash; 8 Hz</div>
        </div>
      </div>
      <div class="wave-track"><div class="wave-fill" style="width:__THETA__%;background:#00e5d4;"></div></div>
      <div class="wave-pct" style="color:#00e5d4;">__THETA__%</div>
    </div>

    <div class="wave-row">
      <div class="wave-label">
        <div class="wave-dot" style="color:#39ff6a;background:#39ff6a;"></div>
        <div>
          <div class="wave-name">ALPHA</div>
          <div class="wave-hz">8 &ndash; 12 Hz</div>
        </div>
      </div>
      <div class="wave-track"><div class="wave-fill" style="width:__ALPHA__%;background:#39ff6a;"></div></div>
      <div class="wave-pct" style="color:#39ff6a;">__ALPHA__%</div>
    </div>

    <div class="wave-row">
      <div class="wave-label">
        <div class="wave-dot" style="color:#a6ff4d;background:#a6ff4d;"></div>
        <div>
          <div class="wave-name">BETA</div>
          <div class="wave-hz">12 &ndash; 30 Hz</div>
        </div>
      </div>
      <div class="wave-track"><div class="wave-fill" style="width:__BETA__%;background:#a6ff4d;"></div></div>
      <div class="wave-pct" style="color:#a6ff4d;">__BETA__%</div>
    </div>

    <div class="wave-row">
      <div class="wave-label">
        <div class="wave-dot" style="color:#bf5fff;background:#bf5fff;"></div>
        <div>
          <div class="wave-name">GAMMA</div>
          <div class="wave-hz">30 &ndash; 45 Hz</div>
        </div>
      </div>
      <div class="wave-track"><div class="wave-fill" style="width:__GAMMA__%;background:#bf5fff;"></div></div>
      <div class="wave-pct" style="color:#bf5fff;">__GAMMA__%</div>
    </div>
  </div>

  <!-- QUOTE -->
  <div class="quote-card">
    <div><span class="q-mark">&ldquo;</span><span class="q-text">Consistency today builds a sharper tomorrow.</span></div>
    <div class="q-sub">KEEP TRAINING. A BRIGHTER YOU AWAITS.</div>
  </div>

  <!-- ACTIONS -->
  <div class="actions">
    <button class="btn primary" id="sendEmailBtn">
      <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="3" y="5" width="18" height="14" rx="2"/><path d="M3 7l9 6 9-6"/></svg>
      SEND RESULT VIA EMAIL
    </button>
    <button class="btn accent" id="downloadBtn">
      <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M12 3v12M7 11l5 5 5-5"/><path d="M4 19h16"/></svg>
      DOWNLOAD RESULT
    </button>
    <button class="btn secondary" id="newSessionBtn">
      <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M4 4v6h6M20 20v-6h-6"/><path d="M5.5 9A7 7 0 0 1 19 9M18.5 15a7 7 0 0 1-13.5 0"/></svg>
      START NEW SESSION
    </button>
  </div>
  <div class="email-status" id="emailStatus"></div>

  <div class="footer-tag">
    <div class="bar"></div>
    DISCIPLINE TODAY<br>A BRIGHTER TOMORROW
  </div>

</div>

<script>
document.getElementById('sendEmailBtn').addEventListener('click', function() {
  let btn = this;
  let statusEl = document.getElementById('emailStatus');
  btn.disabled = true;
  btn.textContent = 'SENDING...';
  statusEl.textContent = '';
  statusEl.className = 'email-status';

  fetch('/send-email', { method: 'POST' })
    .then(r => {
      if (!r.ok) throw new Error('failed');
      return r.json();
    })
    .then(() => {
      statusEl.textContent = 'Email sent successfully.';
      statusEl.className = 'email-status ok';
      btn.textContent = 'SENT';
    })
    .catch(() => {
      statusEl.textContent = 'Could not send email. Check connection and try again.';
      statusEl.className = 'email-status err';
      btn.disabled = false;
      btn.textContent = 'RETRY SEND';
    });
});

document.getElementById('downloadBtn').addEventListener('click', function() {
  // /download-pdf regenerates the same report the email attaches and
  // streams it straight to the browser as a file download.
  let statusEl = document.getElementById('emailStatus');
  statusEl.textContent = 'Preparing your PDF report...';
  statusEl.className = 'email-status';
  window.location.href = '/download-pdf';
  setTimeout(function() {
    statusEl.textContent = '';
    statusEl.className = 'email-status';
  }, 4000);
});

document.getElementById('newSessionBtn').addEventListener('click', function() {
  // POST /new-session returns JSON with the resulting state (READY or
  // NO_SIGNAL per req. #18) — either way the setup page is the right
  // deterministic landing spot; it polls /status and shows the correct
  // banner itself.
  fetch('/new-session', { method: 'POST' })
    .then(() => { window.location.href = '/setup'; })
    .catch(() => { window.location.href = '/setup'; });
});
</script>
</body>
</html>
)rawliteral";

String totalTimeStr;
{
  long totalSec = (long)sessionMinutes * 60;
  int mm = (int)(totalSec / 60);
  int ss = (int)(totalSec % 60);
  totalTimeStr = String(mm) + ":" + (ss < 10 ? "0" : "") + String(ss);
}

String motorOnTimeStr;
{
  unsigned long onSec = motorOnSeconds;
  int mm = (int)(onSec / 60);
  int ss = (int)(onSec % 60);
  motorOnTimeStr = String(mm) + ":" + (ss < 10 ? "0" : "") + String(ss);
}

html.replace("__USER_NAME__", userName.length() ? userName : "-");
html.replace("__USER_EMAIL__", userEmail.length() ? userEmail : "-");
html.replace("__SESSION_MINUTES__", String(sessionMinutes));
html.replace("__TOTAL_TIME__", totalTimeStr);
html.replace("__AVG_ATTENTION__", String(avgAttentionResult));
html.replace("__PEAK_ATTENTION__", String(peakAttentionResult));
html.replace("__MOTOR_ON_TIME__", motorOnTimeStr);
html.replace("__DELTA__", String(resultDeltaPercent));
html.replace("__THETA__", String(resultThetaPercent));
html.replace("__ALPHA__", String(resultAlphaPercent));
html.replace("__BETA__", String(resultBetaPercent));
html.replace("__GAMMA__", String(resultGammaPercent));

server.send(200, "text/html", html);
}
