#include "../../secrets.h"
#include <Wire.h>
#include <Adafruit_BMP280.h>
#include <BH1750.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <HTTPClient.h>

Adafruit_BMP280 bmp;
BH1750 lightMeter;

const int STATUS_LED_PIN = 2;
const int SDA_PIN = 21;
const int SCL_PIN = 22;
const int MEASUREMENT_INTERVAL_MS = 5000;
const uint8_t SENSOR_ADDRESS = 0x76;
const char* FIRMWARE_VERSION = "0.1.0-ota-test";

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

void setupOta() {
  ArduinoOTA.setHostname(OTA_HOSTNAME);
  ArduinoOTA.setPassword(OTA_PASSWORD);

  ArduinoOTA.onStart([]() {
    Serial.print("{\"status\":\"ota_start\",\"type\":\"");
    Serial.print(ArduinoOTA.getCommand() == U_FLASH ? "firmware" : "filesystem");
    Serial.println("\"}");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.print("{\"status\":\"ota_progress\",\"percent\":");
    Serial.print((progress * 100) / total);
    Serial.println("}");
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("{\"status\":\"ota_success\"}");
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.print("{\"status\":\"ota_error\",\"code\":");
    Serial.print(error);
    Serial.print(",\"message\":\"");

    switch (error) {
      case OTA_AUTH_ERROR:
        Serial.print("authentication failed");
        break;
      case OTA_BEGIN_ERROR:
        Serial.print("begin failed");
        break;
      case OTA_CONNECT_ERROR:
        Serial.print("connect failed");
        break;
      case OTA_RECEIVE_ERROR:
        Serial.print("receive failed");
        break;
      case OTA_END_ERROR:
        Serial.print("end failed");
        break;
      default:
        Serial.print("unknown error");
        break;
    }

    Serial.println("\"}");
  });

  ArduinoOTA.begin();

  Serial.print("{\"status\":\"ota_ready\",\"hostname\":\"");
  Serial.print(OTA_HOSTNAME);
  Serial.println("\"}");
}

void waitForNextMeasurement() {
  unsigned long startTime = millis();

  while (millis() - startTime < MEASUREMENT_INTERVAL_MS) {
    ArduinoOTA.handle();
    delay(10);
  }
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
  payload += ",";

  payload += "\"firmwareVersion\":\"";
  payload += FIRMWARE_VERSION;
  payload += "\"";

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
  setupOta();

  if (!bmp.begin(SENSOR_ADDRESS)) {
    Serial.println("{\"status\":\"error\",\"message\":\"BMP280 not found\"}");

    while (true) {
      ArduinoOTA.handle();
      delay(10);
    }
  }

  Serial.println("{\"status\":\"ok\",\"message\":\"BMP280 initialized\"}");
}

void loop() {
  ArduinoOTA.handle();

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

  waitForNextMeasurement();
}
