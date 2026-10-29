#include "BluetoothSerial.h"

BluetoothSerial SerialBT;

// 👉 Your NeuroSky MAC
uint8_t address[6] = {0xC4, 0x64, 0xE3, 0xE6, 0xEA, 0x9A};

byte payloadData[64];
int payloadLength = 0;
byte generatedChecksum = 0;
byte checksum = 0;

byte attention = 0;
byte meditation = 0;
byte poorQuality = 200;

bool connected = false;

unsigned long lastPrintTime = 0;

// 🔥 MOTOR PINS
#define IN1 26
#define IN2 27
#define ENA 14   // PWM

//////////////////////////
// Read from Bluetooth  //
//////////////////////////
byte ReadOneByte() {
  while (!SerialBT.available());
  return SerialBT.read();
}

void setup() {
  Serial.begin(115200);

  // Motor setup
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENA, OUTPUT);

  digitalWrite(IN1, HIGH);  // Forward direction
  digitalWrite(IN2, LOW);

  SerialBT.begin("ESP32_Master", true);
  SerialBT.setPin("0000");

  delay(2000);

  Serial.println("Connecting to MindWave...");

  connected = SerialBT.connect(address);

  if (connected) {
    Serial.println("✅ Connected!");
  } else {
    Serial.println("❌ Connection Failed!");
  }
}

void loop() {

  if (!connected) {
    Serial.println("Reconnecting...");
    connected = SerialBT.connect(address);
    delay(3000);
    return;
  }

  // Parse NeuroSky packets
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
              i++;
              poorQuality = payloadData[i];
              break;

            case 4:
              i++;
              attention = payloadData[i];
              break;

            case 5:
              i++;
              meditation = payloadData[i];
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
  }

  // 🔥 PRINT + MOTOR CONTROL EVERY 1 SECOND
  if (millis() - lastPrintTime >= 1000) {
    lastPrintTime = millis();

    if (poorQuality < 50) {

      // 🧠 MOTOR LOGIC
      int speed = 0;

      if (attention > 30) {
        speed = map(attention, 30, 100, 50, 150);
      } else {
        speed = 0;
      }

      // Apply speed
      analogWrite(ENA, speed);

      Serial.print("🧠 Attention: ");
      Serial.print(attention);
      Serial.print("   ⚙️ Motor Speed: ");
      Serial.println(speed);

    } else {
      Serial.println("❌ NO SIGNAL - WEAR SENSOR PROPERLY");

      // Stop motor if no signal
      analogWrite(ENA, 0);
    }
  }
}
