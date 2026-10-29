/************************************************************
   MINDWAVE PROFESSIONAL PDF EMAIL REPORT
   TBM FINISHING SCHOOL

   Creates a professional A4 PDF similar to the
   MINDWAVE Session Performance Report design.

   Existing project variables used:
   userName
   userEmail
   sessionMinutes
   avgAttentionResult
   peakAttentionResult
   motorOnSeconds
   resultFocusScore
   resultDeltaPercent
   resultThetaPercent
   resultAlphaPercent
   resultBetaPercent
   resultGammaPercent

   Requires:
   ESP-Mail-Client
************************************************************/

static const char *PDF_REPORT_PATH = "/MINDWAVE_Report.pdf";


/************************************************************
   PDF COLOR HELPERS
************************************************************/

void pdfSetFill(String &c, float r, float g, float b)
{
  c += String(r, 3) + " "
      + String(g, 3) + " "
      + String(b, 3) + " rg\n";
}

void pdfSetStroke(String &c, float r, float g, float b)
{
  c += String(r, 3) + " "
      + String(g, 3) + " "
      + String(b, 3) + " RG\n";
}

void pdfSetLineWidth(String &c, float w)
{
  c += String(w, 2) + " w\n";
}


/************************************************************
   PDF BASIC DRAWING
************************************************************/

void pdfRect(
  String &c,
  float x,
  float y,
  float w,
  float h,
  float r,
  float g,
  float b,
  bool fill = true,
  bool stroke = false
)
{
  if (fill)
    pdfSetFill(c, r, g, b);

  if (stroke)
    pdfSetStroke(c, r, g, b);

  c += String(x, 1) + " "
     + String(y, 1) + " "
     + String(w, 1) + " "
     + String(h, 1) + " re\n";

  if (fill && stroke)
    c += "B\n";
  else if (fill)
    c += "f\n";
  else if (stroke)
    c += "S\n";
}


void pdfLineColor(
  String &c,
  float x1,
  float y1,
  float x2,
  float y2,
  float r,
  float g,
  float b,
  float width = 1
)
{
  pdfSetStroke(c, r, g, b);
  pdfSetLineWidth(c, width);

  c += String(x1, 1) + " "
     + String(y1, 1) + " m\n";

  c += String(x2, 1) + " "
     + String(y2, 1) + " l\n";

  c += "S\n";
}


void pdfCircle(
  String &c,
  float cx,
  float cy,
  float radius,
  float r,
  float g,
  float b
)
{
  // Circle using 4 cubic Bezier curves
  const float k = 0.5522848;

  pdfSetFill(c, r, g, b);

  c += String(cx + radius, 1) + " "
     + String(cy, 1) + " m\n";

  c += String(cx + radius, 1) + " "
     + String(cy + radius * k, 1) + " "
     + String(cx + radius * k, 1) + " "
     + String(cy + radius, 1) + " "
     + String(cx, 1) + " "
     + String(cy + radius, 1) + " c\n";

  c += String(cx - radius * k, 1) + " "
     + String(cy + radius, 1) + " "
     + String(cx - radius, 1) + " "
     + String(cy + radius * k, 1) + " "
     + String(cx - radius, 1) + " "
     + String(cy, 1) + " c\n";

  c += String(cx - radius, 1) + " "
     + String(cy - radius * k, 1) + " "
     + String(cx - radius * k, 1) + " "
     + String(cy - radius, 1) + " "
     + String(cx, 1) + " "
     + String(cy - radius, 1) + " c\n";

  c += String(cx + radius * k, 1) + " "
     + String(cy - radius, 1) + " "
     + String(cx + radius, 1) + " "
     + String(cy - radius * k, 1) + " "
     + String(cx + radius, 1) + " "
     + String(cy, 1) + " c\n";

  c += "f\n";
}


/************************************************************
   PDF TEXT
************************************************************/

String pdfEscape(const String &input)
{
  String out;
  out.reserve(input.length() + 8);

  for (size_t i = 0; i < input.length(); i++)
  {
    uint8_t ch = (uint8_t)input[i];

    if (ch == '(' || ch == ')' || ch == '\\')
      out += '\\';

    if (ch >= 32 && ch <= 126)
      out += (char)ch;
    else
      out += '?';
  }

  return out;
}


void pdfText(
  String &c,
  const String &text,
  float x,
  float y,
  int size,
  bool bold,
  float r,
  float g,
  float b
)
{
  pdfSetFill(c, r, g, b);

  c += "BT\n";

  if (bold)
    c += "/F2 ";
  else
    c += "/F1 ";

  c += String(size);
  c += " Tf\n";

  c += String(x, 1);
  c += " ";
  c += String(y, 1);
  c += " Td\n";

  c += "(";
  c += pdfEscape(text);
  c += ") Tj\n";

  c += "ET\n";
}


/************************************************************
   CURRENT DATE / TIME
************************************************************/

String getReportDate()
{
  struct tm timeinfo;

  if (!getLocalTime(&timeinfo, 100))
    return "--/--/----";

  char buffer[20];

  snprintf(
    buffer,
    sizeof(buffer),
    "%02d/%02d/%04d",
    timeinfo.tm_mday,
    timeinfo.tm_mon + 1,
    timeinfo.tm_year + 1900
  );

  return String(buffer);
}


String getReportTime()
{
  struct tm timeinfo;

  if (!getLocalTime(&timeinfo, 100))
    return "--:--:--";

  char buffer[20];

  snprintf(
    buffer,
    sizeof(buffer),
    "%02d:%02d:%02d",
    timeinfo.tm_hour,
    timeinfo.tm_min,
    timeinfo.tm_sec
  );

  return String(buffer);
}


/************************************************************
   MOTOR TIME FORMAT
************************************************************/

String getMotorTime()
{
  unsigned long sec = motorOnSeconds;

  unsigned long minutes = sec / 60;
  unsigned long seconds = sec % 60;

  String result;

  result += String(minutes);
  result += ":";

  if (seconds < 10)
    result += "0";

  result += String(seconds);

  return result;
}


/************************************************************
   SIMPLE PROGRESS BAR
************************************************************/

void pdfProgressBar(
  String &c,
  float x,
  float y,
  float width,
  float height,
  int value,
  float r,
  float g,
  float b
)
{
  if (value < 0)
    value = 0;

  if (value > 100)
    value = 100;

  // Background
  pdfRect(
    c,
    x,
    y,
    width,
    height,
    0.82,
    0.87,
    0.90,
    true,
    false
  );

  // Filled part
  float fillWidth = width * value / 100.0;

  if (fillWidth > 0)
  {
    pdfRect(
      c,
      x,
      y,
      fillWidth,
      height,
      r,
      g,
      b,
      true,
      false
    );
  }
}


/************************************************************
   METRIC CARD
************************************************************/

void pdfMetricCard(
  String &c,
  float x,
  float y,
  float w,
  float h,
  const String &title,
  const String &value,
  const String &sub,
  int progress,
  float r,
  float g,
  float b
)
{
  // Card
  pdfRect(
    c,
    x,
    y,
    w,
    h,
    0.96,
    0.98,
    0.99,
    true,
    false
  );

  // Border
  pdfSetStroke(c, r, g, b);
  pdfSetLineWidth(c, 0.7);

  c += String(x, 1) + " "
     + String(y, 1) + " "
     + String(w, 1) + " "
     + String(h, 1) + " re S\n";

  // Icon circle
  pdfCircle(
    c,
    x + w / 2,
    y + h - 34,
    18,
    r,
    g,
    b
  );

  // Title
  pdfText(
    c,
    title,
    x + 10,
    y + h - 72,
    10,
    true,
    0.04,
    0.10,
    0.22
  );

  // Main value
  pdfText(
    c,
    value,
    x + 18,
    y + 42,
    22,
    true,
    r,
    g,
    b
  );

  if (sub.length())
  {
    pdfText(
      c,
      sub,
      x + 20,
      y + 25,
      7,
      false,
      0.25,
      0.30,
      0.38
    );
  }

  if (progress >= 0)
  {
    pdfProgressBar(
      c,
      x + 12,
      y + 10,
      w - 24,
      7,
      progress,
      r,
      g,
      b
    );
  }
}


/************************************************************
   BRAINWAVE ROW
************************************************************/

void pdfBrainwaveRow(
  String &c,
  float x,
  float y,
  const String &name,
  int value,
  float r,
  float g,
  float b
)
{
  pdfCircle(
    c,
    x + 8,
    y + 5,
    5,
    r,
    g,
    b
  );

  pdfText(
    c,
    name,
    x + 20,
    y,
    10,
    true,
    0.06,
    0.12,
    0.22
  );

  pdfProgressBar(
    c,
    x + 75,
    y + 1,
    135,
    7,
    value,
    r,
    g,
    b
  );

  pdfText(
    c,
    String(value) + " %",
    x + 220,
    y - 1,
    9,
    true,
    0.06,
    0.12,
    0.22
  );
}


/************************************************************
   ATTENTION CHART
************************************************************/

void pdfAttentionChart(String &c)
{
  float chartX = 315;
  float chartY = 300;
  float chartW = 235;
  float chartH = 125;

  // Chart background
  pdfRect(
    c,
    chartX,
    chartY,
    chartW,
    chartH,
    1.0,
    1.0,
    1.0,
    true,
    false
  );

  // Border
  pdfSetStroke(c, 0.70, 0.82, 0.92);
  pdfSetLineWidth(c, 0.7);

  c += String(chartX) + " "
     + String(chartY) + " "
     + String(chartW) + " "
     + String(chartH) + " re S\n";

  pdfText(
    c,
    "Brainwave Activity",
    chartX + 70,
    chartY + chartH - 20,
    10,
    true,
    0.04,
    0.10,
    0.22
  );

  // Graph coordinates
  float gx = chartX + 35;
  float gy = chartY + 25;
  float gw = chartW - 50;
  float gh = chartH - 55;

  // Horizontal grid
  for (int i = 0; i <= 4; i++)
  {
    float yy = gy + (gh / 4.0) * i;

    pdfLineColor(
      c,
      gx,
      yy,
      gx + gw,
      yy,
      0.86,
      0.90,
      0.94,
      0.5
    );
  }

  // Vertical grid
  for (int i = 0; i <= 5; i++)
  {
    float xx = gx + (gw / 5.0) * i;

    pdfLineColor(
      c,
      xx,
      gy,
      xx,
      gy + gh,
      0.90,
      0.93,
      0.96,
      0.4
    );
  }

  // Y axis labels
  pdfText(c, "100", chartX + 5, gy + gh - 4, 7, false,
          0.20, 0.30, 0.42);

  pdfText(c, "75", chartX + 10, gy + gh * 0.75 - 3, 7, false,
          0.20, 0.30, 0.42);

  pdfText(c, "50", chartX + 10, gy + gh * 0.50 - 3, 7, false,
          0.20, 0.30, 0.42);

  pdfText(c, "25", chartX + 10, gy + gh * 0.25 - 3, 7, false,
          0.20, 0.30, 0.42);

  pdfText(c, "0", chartX + 13, gy - 3, 7, false,
          0.20, 0.30, 0.42);


  /********************************************************
     USE THE FIVE BRAINWAVE VALUES AS THE BAR CHART
  ********************************************************/

  int values[5] =
  {
    resultDeltaPercent,
    resultThetaPercent,
    resultAlphaPercent,
    resultBetaPercent,
    resultGammaPercent
  };

  String labels[5] =
  {
    "Delta",
    "Theta",
    "Alpha",
    "Beta",
    "Gamma"
  };

  float colors[5][3] =
  {
    {0.90, 0.25, 0.25},
    {0.98, 0.60, 0.08},
    {0.20, 0.68, 0.48},
    {0.18, 0.55, 0.92},
    {0.46, 0.30, 0.85}
  };

  float barW = 27;
  float gap = 14;

  for (int i = 0; i < 5; i++)
  {
    float bx = gx + 10 + i * (barW + gap);

    float bh = gh * values[i] / 100.0;

    if (bh < 2)
      bh = 2;

    pdfRect(
      c,
      bx,
      gy,
      barW,
      bh,
      colors[i][0],
      colors[i][1],
      colors[i][2],
      true,
      false
    );

    // Percentage
    pdfText(
      c,
      String(values[i]) + "%",
      bx - 1,
      gy + bh + 7,
      8,
      true,
      0.04,
      0.10,
      0.22
    );

    // Label
    pdfText(
      c,
      labels[i],
      bx - 4,
      gy - 15,
      7,
      false,
      0.10,
      0.20,
      0.32
    );
  }
}


/************************************************************
   DECORATIVE WAVE
************************************************************/

void pdfWave(String &c)
{
  pdfSetStroke(c, 0.30, 0.65, 0.95);
  pdfSetLineWidth(c, 0.6);

  for (int wave = 0; wave < 5; wave++)
  {
    float base = 92 + wave * 5;  // pulled up closer to the footer text so the
                                  // big empty gap between them closes up

    c += "0.30 0.65 0.95 RG\n";
    c += "0.55 w\n";

    c += "0 " + String(base) + " m\n";

    // First curve
    c += "80 " + String(base + 18) + " "
       + "140 " + String(base - 18) + " "
       + "220 " + String(base) + " c\n";

    // Second curve
    c += "300 " + String(base + 18) + " "
       + "380 " + String(base - 18) + " "
       + "470 " + String(base) + " c\n";

    // Third curve
    c += "520 " + String(base + 12) + " "
       + "555 " + String(base - 12) + " "
       + "595 " + String(base) + " c\n";

    c += "S\n";
  }
}


/************************************************************
   CREATE PROFESSIONAL PDF
************************************************************/

String makeMindwavePDF()
{
  String c;
  c.reserve(24000);

  // ── page dimensions: A4 = 595 × 842 pt ──────────────────

  /* ========================================================
     1.  PAGE BACKGROUND  (white)
  ======================================================== */
  pdfRect(c,  0, 0, 595, 842,  0.99, 0.99, 1.0,  true);

  /* ========================================================
     2.  HEADER BAND  (dark navy, height 167 pt, y=675..842)
  ======================================================== */
  // dark navy fill
  pdfRect(c,  0, 675, 595, 167,  0.015, 0.075, 0.165,  true);

  // thin cyan accent strip at very top
  pdfRect(c,  0, 836,  595,  6,  0.02, 0.78, 0.95,  true);

  // ------- left side text -----------------------------------
  // "TBM FINISHING SCHOOL" — large white bold
  pdfText(c, "TBM FINISHING SCHOOL", 30, 800, 22, true,  1.0, 1.0, 1.0);

  // "MINDWAVE ATTENTION MONITOR" — cyan medium bold
  pdfText(c, "MINDWAVE ATTENTION MONITOR", 30, 774, 13, true,  0.04, 0.80, 0.96);

  // "SESSION PERFORMANCE REPORT" — light-blue light
  pdfText(c, "SESSION PERFORMANCE REPORT", 30, 752, 10, false,  0.60, 0.80, 0.92);

  // thin horizontal rule under subtitle
  pdfLineColor(c,  30, 742,  350, 742,  0.10, 0.65, 0.88,  0.9);

  // tagline
  pdfText(c, "TRAIN  .  FOCUS  .  GROW  .  EXCEL", 30, 718, 8, false,  0.45, 0.60, 0.72);

  // ------- right side: date / time / quote box --------------
  // "DATE" label
  pdfText(c, "DATE",  462, 812, 8, true,   0.55, 0.72, 0.86);
  pdfText(c, getReportDate(), 462, 798, 9, false, 1.0, 1.0, 1.0);

  // thin separator
  pdfLineColor(c,  462, 790,  568, 790,  0.15, 0.50, 0.72,  0.6);

  // "TIME" label
  pdfText(c, "TIME",  462, 775, 8, true,   0.55, 0.72, 0.86);
  pdfText(c, getReportTime(), 462, 761, 9, false, 1.0, 1.0, 1.0);

  // thin separator
  pdfLineColor(c,  462, 753,  568, 753,  0.15, 0.50, 0.72,  0.6);

  // quote text (3 lines)
  pdfText(c, "\"A FOCUSED MIND", 462, 736, 7, false,  0.82, 0.92, 0.97);
  pdfText(c, "CREATES A",        462, 723, 7, false,  0.82, 0.92, 0.97);
  pdfText(c, "BRIGHTER TOMORROW\"", 462, 710, 7, false,  0.82, 0.92, 0.97);

  // tiny cyan underline accent below quote
  pdfLineColor(c,  462, 703,  520, 703,  0.02, 0.78, 0.92,  1.5);

  /* ========================================================
     3.  PARTICIPANT INFORMATION card  (y = 578..668)
  ======================================================== */
  float piX = 18,  piY = 572,  piW = 559,  piH = 96;

  // card background (very light blue)
  pdfRect(c,  piX, piY, piW, piH,  0.955, 0.975, 0.995,  true);
  // card border (blue)
  pdfSetStroke(c,  0.30, 0.62, 0.85);
  pdfSetLineWidth(c,  0.8);
  c += String(piX,1)+" "+String(piY,1)+" "+String(piW,1)+" "+String(piH,1)+" re S\n";

  // section icon circle
  pdfCircle(c,  44, 638,  13,  0.08, 0.48, 0.80);
  // small white "person" silhouette dots
  pdfCircle(c,  44, 645,   4,  1.0, 1.0, 1.0);
  pdfRect(c,  38, 630, 12, 7,  1.0, 1.0, 1.0,  true);

  // heading
  pdfText(c, "PARTICIPANT INFORMATION",  66, 647, 13, true,  0.04, 0.18, 0.34);
  pdfLineColor(c,  66, 636,  290, 636,  0.22, 0.50, 0.76,  0.9);

  // rows
  pdfText(c, "Name",             30, 614, 10, false, 0.08, 0.20, 0.34);
  pdfText(c, ":",               148, 614, 10, true,  0.08, 0.20, 0.34);
  pdfText(c, userName.length() ? userName : "-", 162, 614, 10, true, 0.04, 0.10, 0.22);

  pdfText(c, "Email",            30, 597, 10, false, 0.08, 0.20, 0.34);
  pdfText(c, ":",               148, 597, 10, true,  0.08, 0.20, 0.34);
  pdfText(c, userEmail.length() ? userEmail : "-", 162, 597, 9, true, 0.04, 0.10, 0.22);

  pdfText(c, "Session Duration", 30, 580, 10, false, 0.08, 0.20, 0.34);
  pdfText(c, ":",               148, 580, 10, true,  0.08, 0.20, 0.34);
  pdfText(c, String(sessionMinutes) + " minute" + (sessionMinutes == 1 ? "" : "s"),
             162, 580, 10, true, 0.04, 0.10, 0.22);

  // vertical divider
  pdfLineColor(c,  385, 584,  385, 660,  0.40, 0.68, 0.88,  0.9);

  // right-side motto
  pdfText(c, "FOCUS",    400, 638, 13, true,  0.20, 0.48, 0.75);
  pdfText(c, "LEADS TO", 400, 620, 13, true,  0.20, 0.48, 0.75);
  pdfText(c, "PROGRESS", 400, 602, 13, true,  0.20, 0.48, 0.75);
  pdfLineColor(c,  400, 592,  445, 592,  0.02, 0.78, 0.92,  2.0);

  /* ========================================================
     4.  SESSION RESULTS heading + 4 metric cards  (y=460..570)
  ======================================================== */
  // "SESSION RESULTS" section header bar — boxed like every other
  // section title in the report, instead of floating bare text
  pdfRect(c, 18, 546, 559, 20, 0.93, 0.97, 1.0, true);
  pdfSetStroke(c, 0.30, 0.62, 0.85);
  pdfSetLineWidth(c, 0.7);
  c += "18.0 546.0 559.0 20.0 re S\n";
  pdfRect(c, 18, 546, 4, 20, 0.04, 0.52, 0.80, true);  // left accent stripe
  pdfText(c, "SESSION RESULTS", 30, 552, 12, true, 0.04, 0.18, 0.34);

  // helper: draw one metric card
  // card colour scheme:  [avgAtt]=green, [peak]=blue, [motor]=purple, [focus]=teal
  struct MetricDef {
    float x; float y; float w; float h;
    const char* title; String value; const char* sub; int progress;
    float r; float g; float b;   // accent colour
  };

  int totalSec   = sessionMinutes * 60;
  int motorRatio = totalSec > 0
                   ? (int)((motorOnSeconds * 100UL) / totalSec) : 0;

  MetricDef metrics[4] = {
    { 18,  460, 133, 80, "AVERAGE ATTENTION",
      String(avgAttentionResult)+" %",  "", avgAttentionResult,
      0.05, 0.55, 0.25 },
    { 157, 460, 133, 80, "PEAK ATTENTION",
      String(peakAttentionResult)+" %", "", peakAttentionResult,
      0.05, 0.48, 0.90 },
    { 296, 460, 133, 80, "MOTOR-ON TIME",
      getMotorTime(),                   "(MM:SS)", -1,
      0.42, 0.22, 0.82 },
    { 435, 460, 142, 80, "FOCUS SCORE",
      String(resultFocusScore)+" / 100","", resultFocusScore,
      0.04, 0.52, 0.68 },
  };

  for (auto& m : metrics) {
    // card bg
    pdfRect(c, m.x, m.y, m.w, m.h,  0.96, 0.98, 1.0, true);
    // border
    pdfSetStroke(c, m.r, m.g, m.b);
    pdfSetLineWidth(c, 0.8);
    c += String(m.x,1)+" "+String(m.y,1)+" "+String(m.w,1)+" "+String(m.h,1)+" re S\n";

    // title (small caps, upper area)
    pdfText(c, m.title,  m.x+8, m.y+m.h-18, 8, true, 0.20, 0.30, 0.40);

    // thin rule under title
    pdfLineColor(c, m.x+8, m.y+m.h-22, m.x+m.w-8, m.y+m.h-22,  0.75, 0.86, 0.93, 0.5);

    // main large value
    pdfText(c, m.value,  m.x+10, m.y+36, 20, true, m.r, m.g, m.b);

    // sub-label (MM:SS etc)
    if (String(m.sub).length())
      pdfText(c, m.sub,  m.x+10, m.y+22, 7, false, 0.28, 0.36, 0.46);

    // progress bar (skip if progress < 0)
    if (m.progress >= 0)
      pdfProgressBar(c, m.x+8, m.y+7, m.w-16, 7, m.progress, m.r, m.g, m.b);
  }

  /* ========================================================
     5.  BRAINWAVE SUMMARY (left)  +  MEANING (right)
         y = 285..455
  ======================================================== */
  // ---- left card ----
  pdfRect(c,  18, 285, 272, 165,  0.96, 0.98, 1.0, true);
  pdfSetStroke(c, 0.35, 0.65, 0.88);  pdfSetLineWidth(c, 0.8);
  c += "18 285 272 165 re S\n";

  pdfText(c, "BRAINWAVE SUMMARY",  28, 430, 11, true,  0.04, 0.18, 0.34);
  pdfLineColor(c, 28, 422, 282, 422,  0.70, 0.82, 0.93, 0.6);

  struct BWDef { const char* name; int pct; float r; float g; float b; };
  BWDef bwRows[5] = {
    {"Delta", resultDeltaPercent,  0.90, 0.25, 0.25},
    {"Theta", resultThetaPercent,  0.98, 0.60, 0.08},
    {"Alpha", resultAlphaPercent,  0.15, 0.68, 0.35},
    {"Beta",  resultBetaPercent,   0.12, 0.48, 0.92},
    {"Gamma", resultGammaPercent,  0.50, 0.22, 0.82},
  };

  for (int i = 0; i < 5; i++) {
    float ry = 400 - i * 24;
    auto& bw = bwRows[i];

    // coloured dot
    pdfCircle(c,  32, ry+4, 5,  bw.r, bw.g, bw.b);

    // name
    pdfText(c, bw.name,  44, ry, 10, false,  0.06, 0.14, 0.24);

    // percentage value (bold)
    char buf[8];  sprintf(buf, "%d %%", bw.pct);
    pdfText(c, buf,  92, ry, 10, true,  bw.r, bw.g, bw.b);

    // progress bar  (x=125, width=145)
    pdfProgressBar(c,  128, ry+1, 145, 7,  bw.pct,  bw.r, bw.g, bw.b);
  }

  // ---- right card: BRAINWAVE ACTIVITY (MEANING) ----
  pdfRect(c,  298, 285, 279, 165,  0.96, 0.98, 1.0, true);
  pdfSetStroke(c, 0.35, 0.65, 0.88);  pdfSetLineWidth(c, 0.8);
  c += "298 285 279 165 re S\n";

  pdfText(c, "BRAINWAVE ACTIVITY (MEANING)",  308, 430, 10, true,  0.04, 0.18, 0.34);
  pdfLineColor(c,  308, 422,  570, 422,  0.70, 0.82, 0.93, 0.6);

  struct BWMeaning { const char* name; const char* meaning; float r; float g; float b; };
  BWMeaning meanings[5] = {
    {"Delta", "Deep-sleep/restorative activity",           0.90, 0.25, 0.25},
    {"Theta", "Drowsiness & memory-related activity",      0.98, 0.60, 0.08},
    {"Alpha", "Relaxed wakefulness",                       0.15, 0.68, 0.35},
    {"Beta",  "Active cognitive processing",               0.12, 0.48, 0.92},
    {"Gamma", "Higher-frequency cognitive/sensory",        0.50, 0.22, 0.82},
  };
  // second line for Gamma
  const char* gammaMeaning2 = "processing";

  for (int i = 0; i < 5; i++) {
    float ry = 400 - i * 24;
    auto& m = meanings[i];
    pdfCircle(c,  310, ry+4, 5,  m.r, m.g, m.b);
    pdfText(c, m.name,    320, ry, 10, true,   0.06, 0.14, 0.24);
    pdfLineColor(c, 356, ry, 358, ry+10,  0.70, 0.82, 0.93, 0.7);
    pdfText(c, m.meaning, 362, ry, 8, false,  0.12, 0.22, 0.34);
    if (i == 4)   // Gamma wraps — its meaning text is the longest of the five
      pdfText(c, gammaMeaning2, 362, ry-10, 8, false, 0.12, 0.22, 0.34);
  }

  /* ========================================================
     6.  DISCLAIMER card  (y = 210..278)
  ======================================================== */
  pdfRect(c,  18, 210, 559, 65,  0.94, 0.97, 1.0, true);
  pdfSetStroke(c, 0.30, 0.65, 0.88);  pdfSetLineWidth(c, 0.8);
  c += "18 210 559 65 re S\n";

  // info icon circle
  pdfCircle(c,  40, 244, 11,  0.04, 0.52, 0.80);
  pdfText(c, "i",  38, 240, 12, true,  1.0, 1.0, 1.0);

  pdfText(c, "Disclaimer",  60, 255, 11, true,  0.04, 0.18, 0.34);
  pdfText(c,
    "This report is directional and not meant for clinical purposes. It also depends on the state of mind",
    60, 238, 8, false,  0.14, 0.22, 0.32);
  pdfText(c,
    "of the individual while participating in a particular activity.",
    60, 225, 8, false,  0.14, 0.22, 0.32);

  /* ========================================================
     7.  FOOTER  (y = 55..200)
  ======================================================== */
  // divider line
  pdfLineColor(c,  18, 190,  577, 190,  0.18, 0.40, 0.62,  0.8);

  pdfText(c, "Regards,",         30, 172, 10, false, 0.04, 0.18, 0.34);
  pdfText(c, "TBM Finishing School", 30, 154, 13, true, 0.04, 0.18, 0.34);

  // right-side footer motto
  pdfLineColor(c,  380, 170,  577, 170,  0.40, 0.65, 0.85,  0.7);
  pdfText(c, "TRAINING MINDS",       390, 155, 8, false, 0.10, 0.28, 0.46);
  pdfText(c, "FOR A BRIGHTER FUTURE", 390, 142, 8, false, 0.10, 0.28, 0.46);

  // decorative wave at the very bottom
  pdfWave(c);

  /* ========================================================
     8.  BUILD PDF BINARY
  ======================================================== */
  String objects[6];

  objects[0] =
    "<< /Type /Catalog /Pages 2 0 R >>";

  objects[1] =
    "<< /Type /Pages /Kids [3 0 R] /Count 1 >>";

  objects[2] =
    "<< /Type /Page "
    "/Parent 2 0 R "
    "/MediaBox [0 0 595 842] "
    "/Resources << "
    "/Font << "
    "/F1 4 0 R "
    "/F2 5 0 R "
    ">> "
    ">> "
    "/Contents 6 0 R >>";

  objects[3] =
    "<< /Type /Font "
    "/Subtype /Type1 "
    "/BaseFont /Helvetica >>";

  objects[4] =
    "<< /Type /Font "
    "/Subtype /Type1 "
    "/BaseFont /Helvetica-Bold >>";

  objects[5] =
    "<< /Length "
    + String(c.length())
    + " >>\n"
      "stream\n"
    + c
    + "endstream";


  String pdf = "%PDF-1.4\n";

  unsigned long offsets[6];

  for (int i = 0; i < 6; i++)
  {
    offsets[i] = pdf.length();

    pdf += String(i + 1);
    pdf += " 0 obj\n";

    pdf += objects[i];

    pdf += "\nendobj\n";
  }


  unsigned long xrefOffset = pdf.length();

  pdf += "xref\n";
  pdf += "0 7\n";

  pdf += "0000000000 65535 f \n";

  for (int i = 0; i < 6; i++)
  {
    char line[32];

    snprintf(
      line,
      sizeof(line),
      "%010lu 00000 n \n",
      offsets[i]
    );

    pdf += line;
  }

  pdf += "trailer\n";
  pdf += "<< /Size 7 /Root 1 0 R >>\n";

  pdf += "startxref\n";
  pdf += String(xrefOffset);
  pdf += "\n%%EOF\n";

  return pdf;
}


/************************************************************
   CREATE PDF ON ESP32 FLASH
************************************************************/

bool createPDFReport()
{
  String pdf = makeMindwavePDF();

  File file =
    ESP_MAIL_DEFAULT_FLASH_FS.open(
      PDF_REPORT_PATH,
      FILE_WRITE
    );

  if (!file)
  {
    Serial.println("ERROR: Cannot create PDF file");
    return false;
  }

  size_t written = file.print(pdf);

  file.close();

  if (written != pdf.length())
  {
    Serial.println("ERROR: PDF write incomplete");

    ESP_MAIL_DEFAULT_FLASH_FS.remove(
      PDF_REPORT_PATH
    );

    return false;
  }

  Serial.print("Professional PDF created: ");
  Serial.print(written);
  Serial.println(" bytes");

  return true;
}


/************************************************************
   SMTP CALLBACK
************************************************************/

void smtpCallback(SMTP_Status status)
{
  Serial.println(status.info());

  if (status.success())
    emailLastResult = true;

  emailInProgress = false;
}


/************************************************************
   SEND PDF EMAIL
************************************************************/

bool sendResultEmailNow()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("WiFi not connected");
    return false;
  }

  if (userEmail.length() == 0)
  {
    Serial.println("No recipient email");
    return false;
  }

  /**********************************************************
     CREATE PDF
  **********************************************************/

  if (!createPDFReport())
    return false;


  /**********************************************************
     SMTP CONFIG
  **********************************************************/

  smtp.debug(1);
  smtp.callback(smtpCallback);

  Session_Config config;

  config.server.host_name = SMTP_HOST;
  config.server.port = SMTP_PORT;

  config.login.email = SMTP_USER;
  config.login.password = SMTP_PASS;
  config.login.user_domain = "";

  config.time.ntp_server =
    "pool.ntp.org,time.nist.gov";

  config.time.gmt_offset = 19800;
  config.time.day_light_offset = 0;


  /**********************************************************
     EMAIL MESSAGE
  **********************************************************/

  SMTP_Message message;

  message.sender.name =
    SMTP_SENDER_NAME;

  message.sender.email =
    SMTP_USER;

  message.subject =
    "MINDWAVE Session Report - " +
    userName;

  message.addRecipient(
    userName.c_str(),
    userEmail.c_str()
  );


  /**********************************************************
     EMAIL BODY
  **********************************************************/

  String body;

  body += "<div style='font-family:Arial,sans-serif;'>";
  body += "<h2 style='color:#07557A;margin-bottom:2px;'>TBM FINISHING SCHOOL</h2>";
  body += "<h3 style='margin-top:2px;'>MINDWAVE ATTENTION MONITOR</h3>";

  body += "<hr>";

  body += "<p>Hello ";
  body += userName;
  body += ",</p>";

  body += "<p>Your MINDWAVE session has been completed.</p>";

  body += "<p>Your professional session performance report "
          "is attached as a PDF.</p>";

  body += "<p>Regards,<br>";
  body += "TBM Finishing School.</p>";

  body += "</div>";


  message.html.content =
    body.c_str();

  message.html.charSet =
    "utf-8";

  message.html.transfer_encoding =
    Content_Transfer_Encoding::enc_qp;

  /**********************************************************
     PDF ATTACHMENT
  **********************************************************/

  SMTP_Attachment attachment;

  attachment.descr.filename =
    "MINDWAVE_Session_Report.pdf";

  attachment.descr.mime =
    "application/pdf";

  attachment.file.path =
    PDF_REPORT_PATH;

  attachment.file.storage_type =
    esp_mail_file_storage_type_flash;

  attachment.descr.transfer_encoding =
    Content_Transfer_Encoding::enc_base64;

  message.addAttachment(
    attachment
  );


  /**********************************************************
     CONNECT SMTP
  **********************************************************/

  if (!smtp.connect(&config))
  {
    Serial.println(
      "SMTP connect() failed:"
    );

    Serial.println(
      smtp.errorReason()
    );

    ESP_MAIL_DEFAULT_FLASH_FS.remove(
      PDF_REPORT_PATH
    );

    return false;
  }


  /**********************************************************
     SEND
  **********************************************************/

  bool ok =
    MailClient.sendMail(
      &smtp,
      &message,
      true
    );


  if (!ok)
  {
    Serial.println(
      "PDF email send failed:"
    );

    Serial.println(
      smtp.errorReason()
    );
  }
  else
  {
    Serial.println(
      "Professional PDF email sent successfully."
    );
  }


  /**********************************************************
     CLEANUP
  **********************************************************/

  smtp.closeSession();

  ESP_MAIL_DEFAULT_FLASH_FS.remove(
    PDF_REPORT_PATH
  );

  return ok;
}


/************************************************************
   HTTP SEND EMAIL HANDLER
************************************************************/

void handleSendEmail()
{
  if (appState != STATE_RESULT)
  {
    server.send(
      409,
      "text/plain",
      "No completed session to email"
    );

    return;
  }

  emailInProgress = true;
  emailAttempted = true;
  emailLastResult = false;

  bool ok =
    sendResultEmailNow();

  emailLastResult = ok;
  emailInProgress = false;


  if (ok)
  {
    showText(
      "PDF Email Sent",
      userEmail
    );

    server.send(
      200,
      "application/json",
      "{\"ok\":true,\"pdf\":true}"
    );
  }
  else
  {
    showText(
      "PDF Email Failed",
      "Check SMTP config"
    );

    server.send(
      502,
      "application/json",
      "{\"ok\":false,\"pdf\":false}"
    );
  }
}
