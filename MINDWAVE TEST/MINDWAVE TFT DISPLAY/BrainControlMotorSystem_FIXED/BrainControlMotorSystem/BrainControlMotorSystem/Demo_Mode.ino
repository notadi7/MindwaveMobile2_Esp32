/************************************************************
   DEMO_MODE.INO
   Synthetic sensor data so the entire state machine (BOOT ->
   SENSOR_CONNECTING -> NO_SIGNAL/READY -> SESSION_STARTED ->
   ACTIVE_SESSION -> SESSION_ENDED -> NEW_SESSION, plus the
   emergency-stop and WiFi-setup paths) can be exercised without
   a real MindWave headset. Only compiled when DEMO_MODE is set
   to 1 in BrainControlMotorSystem.ino.

   Demo mode feeds the EXACT SAME globals the real Bluetooth
   parser fills (attention, poorQuality, brainwaves,
   lastValidEEGPacketMillis) so Sensor_State.ino, Motor_Control.ino,
   Session_Manager.ino, Telemetry.ino and TFT_Display.ino all run
   completely unaware they're not talking to real hardware
   (req. #1). Demo mode CANNOT bypass READY / SESSION START /
   ACTIVE SESSION / SESSION END / RESULT / NEW SESSION / EMERGENCY
   STOP — it only ever writes attention/brainwave/quality data,
   never systemState.
************************************************************/

#if DEMO_MODE
/************************************************************
   🎭 DEMO MODE — fake sensor data (no headset required)
   Drives attention through a slow wave so you can see the
   relay/motor thresholds (att>40, att>60) trigger naturally,
   and lets a full session run end-to-end for testing.

   Set demoSimulateSignalLoss = true (e.g. from the serial
   monitor, or wire it to a debug route) to test the NO_SIGNAL
   path on demand instead of permanently forcing a "perfect"
   signal (req. #35 — an optional, deliberate signal-loss test
   mechanism, rather than poorQuality being hardcoded to 0).
************************************************************/
void simulateSensorData() {
  if (demoSimulateSignalLoss) {
    poorQuality = 200;   // report a bad/absent signal
    // Deliberately do NOT update lastValidEEGPacketMillis here —
    // eegSignalValid will time out exactly like a real dropped
    // headset would, exercising the same NO_SIGNAL path as
    // real hardware.
    return;
  }

  poorQuality = 0;   // report a "good" signal
  lastValidEEGPacketMillis = millis();

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
