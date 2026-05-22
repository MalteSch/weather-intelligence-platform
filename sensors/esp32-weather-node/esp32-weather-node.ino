#include "../../secrets.h"
#include <Wire.h>
#include <Adafruit_BMP280.h>
#include <BH1750.h>
#include <WiFi.h>
#include <HTTPClient.h>

Adafruit_BMP280 bmp;
BH1750 lightMeter;

const int STATUS_LED_PIN = 2;
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

String buildWeatherPayload(float temperature, float pressure, float lightLevelLux) {
  String payload = "{";

  payload += "\"temperatureCelsius\":";
  payload += String(temperature, 2);
  payload += ",";

  payload += "\"pressureHpa\":";
  payload += String(pressure, 2);
  payload += ",";
  
  payload += "\"wifiIp\":\"";
  payload += WiFi.localIP().toString();
  payload += "\",";
  
  payload += "\"wifiRssiDbm\":";
  payload += String(WiFi.RSSI());

  payload += ",";
  payload += "\"lightLevelLux\":";
  payload += String(lightLevelLux, 2);

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

  if (httpResponseCode > 0) {
  digitalWrite(STATUS_LED_PIN, HIGH);
  delay(100);
  digitalWrite(STATUS_LED_PIN, LOW);
  }

  Serial.print("{\"httpStatus\":");
  Serial.print(httpResponseCode);
  Serial.println("}");

  http.end();
}

void setup() {
  Serial.begin(115200);

  pinMode(STATUS_LED_PIN, OUTPUT);

  Wire.begin(SDA_PIN, SCL_PIN);
  lightMeter.begin();

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
  float lightLevelLux = lightMeter.readLightLevel();

  String payload = buildWeatherPayload(
    temperature,
    pressure,
    lightLevelLux
  );

  printJson(payload);
  sendWeatherPayload(payload);

  delay(MEASUREMENT_INTERVAL_MS);
}