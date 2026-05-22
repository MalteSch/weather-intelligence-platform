#include "../../secrets.h"
#include <Wire.h>
#include <Adafruit_BMP280.h>
#include <WiFi.h>
#include <HTTPClient.h>

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

String buildWeatherPayload(float temperature, float pressure) {
  String payload = "{";

  payload += "\"temperatureCelsius\":";
  payload += String(temperature, 2);
  payload += ",";

  payload += "\"pressureHpa\":";
  payload += String(pressure, 2);
  payload += ",";

  payload += "\"sensor\":\"BMP280\",";
  
  payload += "\"wifiIp\":\"";
  payload += WiFi.localIP().toString();
  payload += "\",";
  
  payload += "\"wifiRssiDbm\":";
  payload += String(WiFi.RSSI());

  payload += "}";

  return payload;
}

void printJson(String json) {
  Serial.println(json);
}

void sendWeatherPayload(String payload) {
  HTTPClient http;

  http.begin(API_URL);
  http.addHeader("Content-Type", "application/json");

  int httpResponseCode = http.POST(payload);

  Serial.print("{\"httpStatus\":");
  Serial.print(httpResponseCode);
  Serial.println("}");

  http.end();
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

  String payload = buildWeatherPayload(temperature, pressure);

  printJson(payload);
  sendWeatherPayload(payload);

  delay(MEASUREMENT_INTERVAL_MS);
}