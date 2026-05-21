#include "../../secrets.h"
#include <Wire.h>
#include <Adafruit_BMP280.h>
#include <WiFi.h>

Adafruit_BMP280 bmp;

const int SDA_PIN = 21;
const int SCL_PIN = 22;
const int MEASUREMENT_INTERVAL_MS = 5000;
const uint8_t SENSOR_ADDRESS = 0x76;

void connectToWiFi() {
  Serial.print("{\"status\":\"wifi_connecting\",\"ssid\":\"");
  Serial.print(WIFI_SSID);
  Serial.println("\"}");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("{\"status\":\"wifi_connected\",\"ip\":\"");
  Serial.print(WiFi.localIP());
  Serial.println("\"}");
}

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);
  connectToWiFi();

  if (!bmp.begin(SENSOR_ADDRESS)) {
    Serial.println("{\"status\":\"error\",\"message\":\"BMP280 not found\"}");
    while (true) {
      delay(1000);
    }
  }

  Serial.println("{\"status\":\"ok\",\"message\":\"BMP280 initialized\"}");
}

void loop() {
  float temperature = bmp.readTemperature();
  float pressure = bmp.readPressure() / 100.0;

  Serial.print("{");
  Serial.print("\"temperatureCelsius\":");
  Serial.print(temperature, 2);
  Serial.print(",");
  Serial.print("\"pressureHpa\":");
  Serial.print(pressure, 2);
  Serial.print(",");
  Serial.print("\"sensor\":\"BMP280\"");
  Serial.println("}");

  delay(MEASUREMENT_INTERVAL_MS);
}