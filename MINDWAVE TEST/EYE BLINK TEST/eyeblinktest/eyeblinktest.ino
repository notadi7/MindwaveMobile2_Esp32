#include "BluetoothSerial.h"

BluetoothSerial SerialBT;

uint8_t address[6] = {0xC4, 0x64, 0xE3, 0xE6, 0xEA, 0x9A};

byte payloadData[64];
int payloadLength = 0;
byte generatedChecksum = 0;
byte checksum = 0;

byte poorQuality = 200;
byte attention = 0;

int prevAttention = 0;

bool connected = false;

unsigned long lastBlinkTime = 0;
unsigned long lastPrintTime = 0;

void setup() {
  Serial.begin(115200);

  SerialBT.begin("ESP32_Master", true);
  SerialBT.setPin("0000");

  delay(2000);

  Serial.println("Connecting...");

  connected = SerialBT.connect(address);

  if (connected) Serial.println("✅ Connected!");
  else Serial.println("❌ Failed!");
}

void loop() {

  if (!connected) {
    connected = SerialBT.connect(address);
    delay(3000);
    return;
  }

  while (SerialBT.available()) {

    if (SerialBT.read() == 0xAA) {

      if (!SerialBT.available()) return;
      if (SerialBT.read() != 0xAA) return;

      if (!SerialBT.available()) return;
      payloadLength = SerialBT.read();

      if (payloadLength > 64) return;

      while (SerialBT.available() < payloadLength + 1);

      generatedChecksum = 0;

      for (int i = 0; i < payloadLength; i++) {
        payloadData[i] = SerialBT.read();
        generatedChecksum += payloadData[i];
      }

      checksum = SerialBT.read();
      generatedChecksum = 255 - generatedChecksum;

      if (checksum != generatedChecksum) return;

      for (int i = 0; i < payloadLength; i++) {

        switch (payloadData[i]) {

          case 2:
            i++;
            poorQuality = payloadData[i];
            break;

          case 4:
            i++;
            attention = payloadData[i];
            break;

          case 0x80:
            i += 3;
            break;

          case 0x83:
            i += 25;
            break;
        }
      }
    }
  }

  // 🔥 SIGNAL CHECK
  if (poorQuality == 200) {
    Serial.println("❌ NO SIGNAL - WEAR SENSOR PROPERLY");
    delay(1000);
    return;
  }

  // 🔥 PRINT ATTENTION EVERY 1 SEC
  if (millis() - lastPrintTime > 1000) {
    Serial.print("Attention: ");
    Serial.println(attention);
    lastPrintTime = millis();
  }

  // 🔥 BLINK DETECTION USING SPIKE
  int diff = abs(attention - prevAttention);

  if (diff > 20) {   // threshold (adjust if needed)
    if (millis() - lastBlinkTime > 600) {
      Serial.println("👁️ EYE BLINK");
      lastBlinkTime = millis();
    }
  }

  prevAttention = attention;
}
