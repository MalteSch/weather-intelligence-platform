#include <Wire.h>

void setup() {
  Wire.begin(21, 22);

  Serial.begin(115200);

  Serial.println();
  Serial.println("Scanning...");

  for (byte address = 1; address < 127; address++) {
    Wire.beginTransmission(address);

    if (Wire.endTransmission() == 0) {
      Serial.print("Found device at: 0x");

      if (address < 16) {
        Serial.print("0");
      }

      Serial.println(address, HEX);

      delay(10);
    }
  }

  Serial.println("Scan complete.");
}

void loop() {
}