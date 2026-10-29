/************************************************************
   DEMO_MODE.INO
   Synthetic sensor data so you can test the entire session
   flow (setup -> dashboard -> countdown -> result -> email)
   without a real MindWave headset. Only compiled when
   DEMO_MODE is set to 1 in BrainControlMotorSystem.ino.
************************************************************/

#if DEMO_MODE
/************************************************************
   🎭 DEMO MODE — fake sensor data (no headset required)
   Drives attention through a slow wave so you can see the
   relay/motor thresholds (att>40, att>60) trigger naturally,
   and lets a full session run end-to-end for testing.
************************************************************/
void simulateSensorData() {
  poorQuality = 0;   // always report a "good" signal in demo mode

  unsigned long t = millis();
  float base = 55 + 35 * sin(t / 4000.0);      // wanders 20-90 over ~25s
  int a = (int)base + random(-6, 6);
  if (a < 0) a = 0;
  if (a > 100) a = 100;
  attention = (byte)a;
  meditation = (byte)constrain(50 + 20 * sin(t / 6000.0), 0, 100);

  delta     = 200000 + random(0, 50000);
  theta     = 150000 + random(0, 50000);
  alpha     = 100000 + random(0, 50000);
  beta      = 120000 + random(0, 50000);
  gammaWave = 60000  + random(0, 30000);
}
#endif
