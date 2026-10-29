/************************************************************
   NEUROSKY_PARSER.INO
   Reads and decodes raw packets from the MindWave Mobile 2
   over Bluetooth SPP. Only compiled/used when DEMO_MODE is 0
   (see BrainControlMotorSystem.ino) — Demo_Mode.ino stands in
   for this when testing without a headset.
************************************************************/

/************************************************************
   📥 READ ONE BYTE FROM BLUETOOTH
************************************************************/
byte ReadOneByte() {
  while (!SerialBT.available());
  return SerialBT.read();
}

/************************************************************
   📥 PARSE ONE NEUROSKY PACKET (non-blocking-ish: each
   ReadOneByte() call blocks briefly waiting for the next byte
   over Bluetooth, same as the original inline version — this
   is just that same logic pulled into its own function so
   loop() stays readable).
************************************************************/
void parseNeuroSkyPacket() {
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
{
  i++;

  delta = ((unsigned long)payloadData[i] << 16) |
          ((unsigned long)payloadData[i+1] << 8) |
          payloadData[i+2];
  i += 3;

  theta = ((unsigned long)payloadData[i] << 16) |
          ((unsigned long)payloadData[i+1] << 8) |
          payloadData[i+2];
  i += 3;

  unsigned long lowAlpha =
    ((unsigned long)payloadData[i] << 16) |
    ((unsigned long)payloadData[i+1] << 8) |
    payloadData[i+2];
  i += 3;

  unsigned long highAlpha =
    ((unsigned long)payloadData[i] << 16) |
    ((unsigned long)payloadData[i+1] << 8) |
    payloadData[i+2];
  i += 3;

  unsigned long lowBeta =
    ((unsigned long)payloadData[i] << 16) |
    ((unsigned long)payloadData[i+1] << 8) |
    payloadData[i+2];
  i += 3;

  unsigned long highBeta =
    ((unsigned long)payloadData[i] << 16) |
    ((unsigned long)payloadData[i+1] << 8) |
    payloadData[i+2];
  i += 3;

  unsigned long lowGamma =
    ((unsigned long)payloadData[i] << 16) |
    ((unsigned long)payloadData[i+1] << 8) |
    payloadData[i+2];
  i += 3;

  unsigned long midGamma =
    ((unsigned long)payloadData[i] << 16) |
    ((unsigned long)payloadData[i+1] << 8) |
    payloadData[i+2];

  alpha = lowAlpha + highAlpha;
  beta  = lowBeta + highBeta;
  gammaWave = lowGamma + midGamma;

  break;
}
          }
        }
      }
    }
  }
}
