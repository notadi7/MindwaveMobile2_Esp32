/************************************************************
   NEUROSKY_PARSER.INO
   Reads and decodes raw packets from the MindWave Mobile 2
   over Bluetooth SPP. Only compiled/used when DEMO_MODE is 0
   (see BrainControlMotorSystem.ino) — Demo_Mode.ino stands in
   for this when testing without a headset.

   NON-BLOCKING REWRITE (req. #6):
   The original ReadOneByte() did:

       while (!SerialBT.available());
       return SerialBT.read();

   which blocks the ENTIRE loop() — WebServer, WebSocket,
   GPIO13, TFT, session timing — for however long it takes the
   next Bluetooth byte to arrive. That is replaced here with a
   byte-by-byte parser STATE MACHINE: processMindWaveBluetooth()
   drains whatever bytes are already available and returns
   immediately, carrying partial-packet state between calls in
   `parserState`. It never waits for a future byte.
************************************************************/

enum NeuroSkyParserState {
  NS_WAIT_SYNC_1,
  NS_WAIT_SYNC_2,
  NS_READ_LENGTH,
  NS_READ_PAYLOAD,
  NS_READ_CHECKSUM
};

NeuroSkyParserState nsParserState = NS_WAIT_SYNC_1;
int nsPayloadIndex = 0;

void parseNeuroSkyPayload();   // defined below; forward-declared since it's
                                // used (in processMindWaveByte()) before its
                                // definition later in this same file, and
                                // not every Arduino IDE/CLI version's
                                // auto-prototype pass catches that.

/************************************************************
   📥 DRAIN WHATEVER IS AVAILABLE RIGHT NOW — NEVER BLOCKS.
   Call every loop(). Processes zero or more complete/partial
   bytes and returns; if a full, checksum-valid packet is
   assembled it is decoded immediately.
************************************************************/
void processMindWaveBluetooth() {
  while (SerialBT.available()) {
    uint8_t b = (uint8_t)SerialBT.read();
    processMindWaveByte(b);
  }
}

/************************************************************
   📥 PARSER STATE MACHINE — one byte per call, no waiting.
************************************************************/
void processMindWaveByte(uint8_t b) {
  switch (nsParserState) {

    case NS_WAIT_SYNC_1:
      if (b == 0xAA) nsParserState = NS_WAIT_SYNC_2;
      break;

    case NS_WAIT_SYNC_2:
      if (b == 0xAA) {
        nsParserState = NS_READ_LENGTH;
      } else {
        // Not a second sync byte — resync from scratch.
        nsParserState = NS_WAIT_SYNC_1;
      }
      break;

    case NS_READ_LENGTH:
      payloadLength = b;
      if (payloadLength > 64) {
        // Malformed/garbage length — drop and resync.
        nsParserState = NS_WAIT_SYNC_1;
        break;
      }
      nsPayloadIndex = 0;
      generatedChecksum = 0;
      nsParserState = (payloadLength == 0) ? NS_READ_CHECKSUM : NS_READ_PAYLOAD;
      break;

    case NS_READ_PAYLOAD:
      payloadData[nsPayloadIndex++] = b;
      generatedChecksum += b;
      if (nsPayloadIndex >= payloadLength) {
        nsParserState = NS_READ_CHECKSUM;
      }
      break;

    case NS_READ_CHECKSUM: {
      checksum = b;
      byte computed = 255 - generatedChecksum;
      if (checksum == computed) {
        parseNeuroSkyPayload();   // decode payloadData[] into attention/meditation/brainwaves
      }
      nsParserState = NS_WAIT_SYNC_1;
      break;
    }
  }
}

/************************************************************
   📥 DECODE ONE VALIDATED PAYLOAD
   Only reached when the checksum has already been verified —
   this is therefore exactly "a valid packet" per req. #7, so
   lastValidEEGPacketMillis is stamped unconditionally here.
   updateSensorState() (Sensor_State.ino) is the only place
   that turns that timestamp into eegSignalValid.
************************************************************/
void parseNeuroSkyPayload() {
  lastValidEEGPacketMillis = millis();

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

      case 0x83: {
        i++;

        delta = ((unsigned long)payloadData[i] << 16) |
                ((unsigned long)payloadData[i + 1] << 8) |
                payloadData[i + 2];
        i += 3;

        theta = ((unsigned long)payloadData[i] << 16) |
                ((unsigned long)payloadData[i + 1] << 8) |
                payloadData[i + 2];
        i += 3;

        unsigned long lowAlpha =
          ((unsigned long)payloadData[i] << 16) |
          ((unsigned long)payloadData[i + 1] << 8) |
          payloadData[i + 2];
        i += 3;

        unsigned long highAlpha =
          ((unsigned long)payloadData[i] << 16) |
          ((unsigned long)payloadData[i + 1] << 8) |
          payloadData[i + 2];
        i += 3;

        unsigned long lowBeta =
          ((unsigned long)payloadData[i] << 16) |
          ((unsigned long)payloadData[i + 1] << 8) |
          payloadData[i + 2];
        i += 3;

        unsigned long highBeta =
          ((unsigned long)payloadData[i] << 16) |
          ((unsigned long)payloadData[i + 1] << 8) |
          payloadData[i + 2];
        i += 3;

        unsigned long lowGamma =
          ((unsigned long)payloadData[i] << 16) |
          ((unsigned long)payloadData[i + 1] << 8) |
          payloadData[i + 2];
        i += 3;

        unsigned long midGamma =
          ((unsigned long)payloadData[i] << 16) |
          ((unsigned long)payloadData[i + 1] << 8) |
          payloadData[i + 2];

        alpha = lowAlpha + highAlpha;
        beta  = lowBeta + highBeta;
        gammaWave = lowGamma + midGamma;

        break;
      }
    }
  }
}
