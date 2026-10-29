/************************************************************
   SETUP_PAGE.INO
   "/" route handler (decides setup vs dashboard vs result) and
   the credential-capture page HTML/CSS/JS itself.
************************************************************/

/******** HANDLE ROOTS ********/
void handleRoot() {
  if (appState == STATE_SETUP) {
    sendSetupPage();
  } else if (appState == STATE_RESULT) {
    sendResultPage();
  } else {
    sendDashboardPage();
  }
}

/************************************************************
   📝 SETUP / CREDENTIAL CAPTURE PAGE
************************************************************/
void sendSetupPage() {
String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>MindWave - Start Session</title>
<link href="https://fonts.googleapis.com/css2?family=Orbitron:wght@400;700;900&family=Rajdhani:wght@400;500;600;700&display=swap" rel="stylesheet">
<style>
  :root {
    --bg: #05070d;
    --panel: rgba(12,18,32,0.55);
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
  }

  /* ── AMBIENT BACKGROUND ── */
  .bg-glow {
    position:fixed; inset:0; z-index:0; pointer-events:none;
    background:
      radial-gradient(600px 400px at 12% 20%, rgba(0,200,255,0.08), transparent 60%),
      radial-gradient(700px 500px at 85% 75%, rgba(57,255,106,0.07), transparent 60%),
      radial-gradient(500px 500px at 90% 15%, rgba(0,200,255,0.06), transparent 55%);
  }

  .page {
    position:relative; z-index:1;
    min-height:100vh;
    display:grid;
    grid-template-columns: minmax(300px, 1fr) minmax(560px, 650px) minmax(300px, 1fr);
    align-items:center;
    gap:42px;
    padding:32px 42px;
    min-height:100vh;
  }

  /* ── LEFT PANEL ── */
  .left-panel { display:flex; flex-direction:column; height:100%; justify-content:space-between; }
  .brand-row { display:flex; align-items:center; gap:14px; }
  .brand-icon {
    width:52px; height:52px; border-radius:14px;
    display:flex; align-items:center; justify-content:center;
    background:rgba(0,200,255,0.08);
    border:1px solid var(--border);
  }
  .brand-icon svg { width:30px; height:30px; }
  .brand-name {
    font-family:'Orbitron',monospace; font-weight:900; font-size:27px; letter-spacing:2px;
    background:linear-gradient(90deg, var(--accent), var(--green));
    -webkit-background-clip:text; background-clip:text; color:transparent;
  }
  .brand-sub { font-size:12px; letter-spacing:3px; color:#8295b5; margin-top:4px; }

  .features { display:flex; flex-direction:column; gap:34px; margin:auto 0; }
  .feature { display:flex; gap:18px; align-items:flex-start; }
  .feature-icon {
    flex:0 0 auto; width:56px; height:56px; border-radius:50%;
    border:1px solid var(--border);
    display:flex; align-items:center; justify-content:center;
    color:var(--accent);
    background:rgba(0,200,255,0.05);
  }
  .feature-icon svg { width:25px; height:25px; }
  .feature-title { font-family:'Orbitron',monospace; font-size:15px; font-weight:700; letter-spacing:1.5px; color:#edf7ff; margin-bottom:6px; }
  .feature-desc { font-size:16px; color:#8ea2c2; line-height:1.45; max-width:280px; }

  .footer-tag { font-size:12px; letter-spacing:2px; color:#7d91b2; line-height:1.7; }
  .footer-tag .bar { width:46px; height:2px; background:linear-gradient(90deg, var(--accent), var(--green)); margin-bottom:10px; }

  /* ── RIGHT PANEL ── */
  .right-panel { display:flex; flex-direction:column; height:100%; justify-content:space-between; align-items:flex-end; text-align:right; min-width:0; }
  .right-tag { font-size:12px; letter-spacing:2.5px; color:#8799b6; line-height:1.8; }
  .right-tag .bar { width:46px; height:2px; background:linear-gradient(90deg, var(--green), var(--accent)); margin-top:10px; margin-left:auto; }

  .quote-block { margin:auto 0; position:relative; padding-right:4px; width:100%; max-width:430px; }
  .quote-mark { font-family:'Orbitron',monospace; font-size:46px; color:var(--accent); opacity:0.55; line-height:0.4; }
  .quote-text { font-family:'Orbitron',monospace; font-size:27px; font-weight:700; letter-spacing:1.2px; color:#edf7ff; line-height:1.45; margin-top:10px; }

  .right-visual {
    margin:auto 0;
    width:100%;
    max-width:460px;
    display:flex;
    flex-direction:column;
    align-items:flex-end;
  }
  .brain-svg {
    width:100%;
    max-width:420px;
    height:auto;
    margin-bottom:8px;
    filter:drop-shadow(0 0 18px rgba(0,200,255,0.16));
  }

  /* ── CENTER CARD ── */
  .card-wrap { display:flex; justify-content:center; }
  .setup-card {
    width:100%;
    background: var(--panel);
    backdrop-filter: blur(16px);
    -webkit-backdrop-filter: blur(16px);
    border:1px solid var(--border);
    border-radius:24px;
    padding:38px 42px 30px;
    box-shadow:0 0 70px rgba(0,200,255,0.13), inset 0 0 45px rgba(0,200,255,0.04);
    max-width:650px;
  }
  .card-eyebrow { text-align:center; font-size:13px; letter-spacing:4px; color:#8fa3c1; }
  .card-title {
    text-align:center; font-family:'Orbitron',monospace; font-weight:900; font-size:42px; letter-spacing:2px;
    background:linear-gradient(90deg, var(--accent), var(--green));
    -webkit-background-clip:text; background-clip:text; color:transparent;
    margin-top:4px;
    text-shadow:0 0 30px rgba(0,200,255,0.25);
  }
  .card-sub { text-align:center; font-size:13px; letter-spacing:3px; color:#8296b6; margin-top:8px; }
  .divider { display:flex; align-items:center; justify-content:center; gap:10px; margin:16px 0; }
  .divider .line { width:44px; height:1px; background:var(--border); }
  .divider svg { width:16px; height:16px; color:var(--accent); opacity:0.8; }
  .card-lead { text-align:center; font-size:16px; color:#9aacc7; margin-bottom:24px; }

  .field { margin-bottom:18px; }
  .field-box {
    display:flex; align-items:center; gap:12px;
    border:1px solid var(--border);
    border-radius:12px;
    padding:16px 18px;
    background:rgba(6,10,20,0.4);
    transition:border-color 0.2s, box-shadow 0.2s;
  }
  .field-box:focus-within { border-color:var(--accent); box-shadow:0 0 0 3px rgba(0,200,255,0.10); }
  .field-box svg { width:23px; height:23px; color:var(--accent); flex:0 0 auto; opacity:0.95; }
  .field-text { flex:1; min-width:0; }
  .field-label { font-size:12px; letter-spacing:1.5px; color:#91a4c1; display:block; margin-bottom:4px; }
  .field-box input {
    width:100%; background:none; border:none; outline:none;
    color:#eef7ff; font-family:'Rajdhani',sans-serif; font-size:19px; font-weight:600;
  }
  .field-box input::placeholder { color:#3f4c66; }

  .duration-box { display:flex; gap:14px; align-items:flex-start; border:1px solid var(--border); border-radius:14px; padding:17px 18px; background:rgba(6,10,20,0.48); }
  .duration-box svg { width:23px; height:23px; color:var(--accent); margin-top:2px; flex:0 0 auto; opacity:0.95; }
  .duration-body { flex:1; min-width:0; }
  .duration-top-row { display:flex; justify-content:space-between; align-items:center; margin-bottom:8px; }
  .duration-label { font-size:12px; letter-spacing:1.5px; color:#91a4c1; }
  .duration-controls { display:flex; align-items:center; gap:8px; }
  .minute-btn { width:32px; height:32px; border:1px solid rgba(0,200,255,.30); border-radius:8px; background:rgba(0,200,255,.07); color:#e8faff; font-size:22px; font-weight:700; cursor:pointer; }
  .minute-btn:hover { border-color:var(--accent); background:rgba(0,200,255,.15); }
  .minute-btn:active { transform:scale(.94); }
  .minute-btn:disabled { opacity:.35; cursor:not-allowed; }
  .duration-val { font-family:'Orbitron',monospace; font-size:19px; font-weight:700; color:#39eaff; }
  input[type=range] {
    width:100%; -webkit-appearance:none; appearance:none;
    height:6px; border-radius:5px;
    background:linear-gradient(90deg, var(--accent), var(--green));
    outline:none;
  }
  input[type=range]::-webkit-slider-thumb {
    -webkit-appearance:none; appearance:none;
    width:20px; height:20px; border-radius:50%;
    background:#ffffff; cursor:pointer;
    box-shadow:0 0 0 4px rgba(0,200,255,0.25), 0 0 12px rgba(0,200,255,0.5);
  }
  input[type=range]::-moz-range-thumb {
    width:20px; height:20px; border-radius:50%; border:none;
    background:#ffffff; cursor:pointer;
    box-shadow:0 0 0 4px rgba(0,200,255,0.25), 0 0 12px rgba(0,200,255,0.5);
  }
  .duration-minmax { display:flex; justify-content:space-between; font-size:12px; color:#8296b6; margin-top:7px; }

  .err { color:#ff7188; font-size:13px; letter-spacing:0.5px; text-align:center; min-height:18px; margin-top:12px; }

  .start-btn {
    width:100%; margin-top:7px; padding:18px;
    border:none; border-radius:12px; cursor:pointer;
    background:linear-gradient(90deg, var(--accent), var(--green));
    color:#04060c;
    font-family:'Orbitron',monospace; font-weight:900; letter-spacing:2px; font-size:15px;
    display:flex; align-items:center; justify-content:center; gap:10px;
  }
  .start-btn:active { transform:scale(0.985); }
  .start-btn svg { width:16px; height:16px; }

  .card-footer { text-align:center; font-size:13px; color:#7f93b3; margin-top:18px; letter-spacing:0.7px; }

  /* ── RESPONSIVE ── */
  /* ── RESPONSIVE ── */
  @media (max-width: 1180px) {
    .page {
      grid-template-columns: minmax(240px, 0.75fr) minmax(500px, 620px);
      gap:30px;
      padding:28px 30px;
    }
    .right-panel { display:none; }
  }

  @media (max-width: 760px) {
    .page {
      grid-template-columns:1fr;
      padding:22px 16px;
    }
    .left-panel { display:none; }
    .setup-card {
      padding:30px 22px 26px;
      border-radius:20px;
    }
    .card-title { font-size:34px; }
    .card-lead { font-size:15px; }
  }
</style>
</head>
<body>
<div class="bg-glow"></div>

<div class="page">

  <!-- LEFT PANEL -->
  <div class="left-panel">
    <div>
      <div class="brand-row">
        <div class="brand-icon">
          <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.6" style="color:var(--accent)">
            <path d="M9 4a3 3 0 0 0-3 3 3 3 0 0 0-2 5 3 3 0 0 0 2 5 3 3 0 0 0 3 3V4z"/>
            <path d="M15 4a3 3 0 0 1 3 3 3 3 0 0 1 2 5 3 3 0 0 1-2 5 3 3 0 0 1-3 3V4z"/>
            <path d="M9 8h1M9 12h1.5M9 16h1M14 8h1M13.5 12H15M14 16h1"/>
          </svg>
        </div>
        <div>
          <div class="brand-name">TBM FINISHING SCHOOL</div>
          <div class="brand-sub">FOCUS &middot; PERFORM &middot; GROW</div>
        </div>
      </div>
    </div>

    <div class="features">
      <div class="feature">
        <div class="feature-icon">
          <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8"><circle cx="12" cy="12" r="8"/><circle cx="12" cy="12" r="4"/><circle cx="12" cy="12" r="0.6" fill="currentColor"/></svg>
        </div>
        <div>
          <div class="feature-title">STAY FOCUSED</div>
          <div class="feature-desc">Eliminate distractions and get more done.</div>
        </div>
      </div>

      <div class="feature">
        <div class="feature-icon">
          <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8"><path d="M4 20V10M11 20V4M18 20v-7"/></svg>
        </div>
        <div>
          <div class="feature-title">TRACK PROGRESS</div>
          <div class="feature-desc">See your improvement over time.</div>
        </div>
      </div>

      <div class="feature">
        <div class="feature-icon">
          <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.6">
            <path d="M9 4a3 3 0 0 0-3 3 3 3 0 0 0-2 5 3 3 0 0 0 2 5 3 3 0 0 0 3 3V4z"/>
            <path d="M15 4a3 3 0 0 1 3 3 3 3 0 0 1 2 5 3 3 0 0 1-2 5 3 3 0 0 1-3 3V4z"/>
          </svg>
        </div>
        <div>
          <div class="feature-title">BUILD A BETTER YOU</div>
          <div class="feature-desc">Small sessions. Big results.</div>
        </div>
      </div>
    </div>

    <div class="footer-tag">
      <div class="bar"></div>
      DISCIPLINE TODAY<br>A BRIGHTER TOMORROW
    </div>
  </div>

  <!-- CENTER CARD -->
  <div class="card-wrap">
    <div class="setup-card">
      <div class="card-eyebrow">WELCOME TO</div>
      <div class="card-title">MINDWAVE</div>
      <div class="card-sub">FOCUS &middot; TRACK &middot; IMPROVE</div>

      <div class="divider">
        <div class="line"></div>
        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M2 12h4l2-7 4 14 3-10 2 3h5"/></svg>
        <div class="line"></div>
      </div>

      <div class="card-lead">Enter your details to start a focused session</div>

      <form id="setupForm">
        <div class="field">
          <div class="field-box">
            <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8"><circle cx="12" cy="8" r="4"/><path d="M4 20c0-4 4-6 8-6s8 2 8 6"/></svg>
            <div class="field-text">
              <span class="field-label">FULL NAME</span>
              <input type="text" id="name" required maxlength="40" placeholder="Your name">
            </div>
          </div>
        </div>

        <div class="field">
          <div class="field-box">
            <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8"><rect x="3" y="5" width="18" height="14" rx="2"/><path d="M3 7l9 6 9-6"/></svg>
            <div class="field-text">
              <span class="field-label">EMAIL ADDRESS</span>
              <input type="email" id="email" required maxlength="60" placeholder="you@example.com">
            </div>
          </div>
        </div>

        <div class="field">
          <div class="duration-box">
            <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8"><circle cx="12" cy="12" r="9"/><path d="M12 7v5l3 3"/></svg>
            <div class="duration-body">
              <div class="duration-top-row">
                <span class="duration-label">SESSION DURATION</span>
                <div class="duration-controls">
                   <button type="button" class="minute-btn" id="minusBtn" aria-label="Decrease">&#8722;</button>
                   <span class="duration-val" id="minutesVal">1 min</span>
                   <button type="button" class="minute-btn" id="plusBtn" aria-label="Increase">+</button>
                 </div>
              </div>
              <input type="range" id="minutes" min="1" max="30" step="1" value="1">
              <div class="duration-minmax"><span>1 min</span><span>30 min</span></div>
            </div>
          </div>
        </div>

        <div class="err" id="formErr"></div>

        <button type="submit" class="start-btn">
          <svg viewBox="0 0 24 24" fill="currentColor"><path d="M8 5v14l11-7z"/></svg>
          START SESSION
          <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.2"><path d="M5 12h14M13 6l6 6-6 6"/></svg>
        </button>
      </form>

      <div class="card-footer">Clear mind. Higher performance.</div>
    </div>
  </div>

  <!-- RIGHT PANEL -->
  <div class="right-panel">
    <div class="right-tag">
      A SMARTER YOU<br>STARTS HERE
      <div class="bar"></div>
    </div>

    <div class="right-visual">
      <svg class="brain-svg" viewBox="0 0 420 330" aria-hidden="true">
        <defs>
          <linearGradient id="brainStroke" x1="0" y1="0" x2="1" y2="1">
            <stop offset="0%" stop-color="#00c8ff"/>
            <stop offset="55%" stop-color="#18e8ff"/>
            <stop offset="100%" stop-color="#39ff6a"/>
          </linearGradient>
          <filter id="brainGlow">
            <feGaussianBlur stdDeviation="3.5" result="blur"/>
            <feMerge><feMergeNode in="blur"/><feMergeNode in="SourceGraphic"/></feMerge>
          </filter>
        </defs>
        <g fill="none" stroke="url(#brainStroke)" stroke-width="1.4" opacity="0.72" filter="url(#brainGlow)">
          <path d="M209 48c-27-24-69-18-83 14-29-9-55 12-50 42-25 17-18 54 9 64-7 31 17 52 43 48 12 31 49 31 65 9 18 18 53 9 58-17 31 2 50-28 34-53 20-26 3-61-26-64 0-25-20-45-50-43z"/>
          <path d="M211 49v181"/>
          <path d="M127 77c25 4 37 20 38 44-1 27-12 44-31 57"/>
          <path d="M92 111c27-2 47 8 57 29"/>
          <path d="M87 151c26-3 45 6 57 27"/>
          <path d="M119 196c21-12 39-11 53 8"/>
          <path d="M295 76c-22 5-35 21-35 43 1 23 10 39 29 52"/>
          <path d="M330 111c-26-1-44 9-54 30"/>
          <path d="M332 151c-24-3-42 6-54 27"/>
          <path d="M302 196c-20-12-37-10-51 8"/>
          <path d="M150 61c7 17 20 24 38 25"/>
          <path d="M274 63c-8 15-20 23-36 24"/>
        </g>
        <g fill="#18e8ff">
          <circle cx="150" cy="93" r="3"/><circle cx="112" cy="125" r="3"/>
          <circle cx="145" cy="178" r="3"/><circle cx="183" cy="68" r="3"/>
          <circle cx="270" cy="96" r="3"/><circle cx="310" cy="130" r="3"/>
          <circle cx="276" cy="180" r="3"/><circle cx="238" cy="73" r="3"/>
        </g>
        <path d="M205 229c-7 18-8 33-8 48h27c0-15-1-31-8-48" fill="none" stroke="url(#brainStroke)" stroke-width="2"/>
      </svg>
      <div class="quote-block">
        <div class="quote-mark">&ldquo;</div>
        <div class="quote-text">A CALMER MIND<br>A BRIGHTER <span style="color:var(--green)">YOU</span></div>
      </div>
    </div>

    <div></div>
  </div>

</div>

<script>
const minutesEl = document.getElementById('minutes');
const minutesVal = document.getElementById('minutesVal');
const minusBtn = document.getElementById('minusBtn');
const plusBtn = document.getElementById('plusBtn');

function updateMinutes(value) {
  const min = parseInt(minutesEl.min, 10);
  const max = parseInt(minutesEl.max, 10);
  const v = Math.max(min, Math.min(max, parseInt(value, 10) || min));
  minutesEl.value = v;
  minutesVal.textContent = v + ' min';
  minusBtn.disabled = v <= min;
  plusBtn.disabled = v >= max;
}

minutesEl.addEventListener('input', () => updateMinutes(minutesEl.value));
minusBtn.addEventListener('click', () => updateMinutes(+minutesEl.value - 1));
plusBtn.addEventListener('click', () => updateMinutes(+minutesEl.value + 1));
updateMinutes(minutesEl.value);

document.getElementById('setupForm').addEventListener('submit', function(e) {
  e.preventDefault();
  const name = document.getElementById('name').value.trim();
  const email = document.getElementById('email').value.trim();
  const minutes = parseInt(minutesEl.value, 10);
  const errEl = document.getElementById('formErr');
  errEl.textContent = '';

  const emailOk = /^[^\s@]+@[^\s@]+\.[^\s@]+$/.test(email);
  if (!name) { errEl.textContent = 'Please enter your name.'; return; }
  if (!emailOk) { errEl.textContent = 'Please enter a valid email.'; return; }
  if (!minutes || minutes < 1) { errEl.textContent = 'Session must be at least 1 minute.'; return; }

  const body = new URLSearchParams({ name, email, minutes });
  fetch('/start-session', { method: 'POST', headers: {'Content-Type':'application/x-www-form-urlencoded'}, body })
    .then(() => { window.location.href = '/'; })
    .catch(() => { errEl.textContent = 'Could not reach the device. Try again.'; });
});
</script>
</body>
</html>
)rawliteral";

server.send(200, "text/html", html);
}
