#include "BluetoothSerial.h"

BluetoothSerial SerialBT;

// 👉 Your NeuroSky MAC
uint8_t address[6] = {0xC4, 0x64, 0xE3, 0xE6, 0xEA, 0x9A};

bool connected = false;

void setup() {
  Serial.begin(115200);

  Serial.println("Starting Bluetooth...");

  // Start in MASTER mode
  SerialBT.begin("ESP32_Master", true);
  SerialBT.setPin("0000");

  delay(2000);

  Serial.println("Connecting to MindWave...");

  // Try connecting using MAC
  connected = SerialBT.connect(address);

  if (connected) {
    Serial.println("✅ Connected to MindWave!");
  } else {
    Serial.println("❌ Failed to connect!");
  }
}

void loop() {

  if (connected) {
    while (SerialBT.available()) {
      byte data = SerialBT.read();

      Serial.print(data, HEX);
      Serial.print(" ");
    }
  } else {
    Serial.println("Reconnecting...");
    connected = SerialBT.connect(address);
    delay(3000);
  }
}
