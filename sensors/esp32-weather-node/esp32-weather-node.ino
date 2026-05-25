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
const unsigned long MEASUREMENT_INTERVAL_MS = 5000;
const unsigned long WIFI_RECONNECT_INTERVAL_MS = 10000;
const unsigned long UPLOAD_RECOVERY_TIMEOUT_MS = 10UL * 60UL * 1000UL;
const uint16_t HTTP_TIMEOUT_MS = 4000;
const uint8_t SENSOR_ADDRESS = 0x76;
const char* FIRMWARE_VERSION = "0.1.0-ota-test";

enum LedSignal {
  LED_IDLE,
  LED_UPLOAD_SUCCESS,
  LED_UPLOAD_FAILURE,
  LED_WIFI_DISCONNECTED
};

LedSignal ledSignal = LED_IDLE;
unsigned long ledSignalStartedAt = 0;
unsigned long lastMeasurementAt = 0;
unsigned long lastWiFiAttemptAt = 0;
unsigned long lastSuccessfulUploadAt = 0;
unsigned long wifiReconnectCount = 0;
unsigned long uploadFailureCount = 0;
bool hasSuccessfulUpload = false;
bool wifiWasConnected = false;
bool otaStarted = false;

void setLedSignal(LedSignal signal) {
  ledSignal = signal;
  ledSignalStartedAt = millis();
}

void serviceStatusLed() {
  unsigned long elapsed = millis() - ledSignalStartedAt;
  bool ledOn = false;

  switch (ledSignal) {
    case LED_UPLOAD_SUCCESS:
      ledOn = elapsed < 100;
      if (elapsed >= 100) {
        ledSignal = LED_IDLE;
      }
      break;
    case LED_UPLOAD_FAILURE:
      ledOn = elapsed < 120 || (elapsed >= 240 && elapsed < 360);
      if (elapsed >= 480) {
        ledSignal = LED_IDLE;
      }
      break;
    case LED_WIFI_DISCONNECTED:
      ledOn = (elapsed % 1000) < 180;
      break;
    case LED_IDLE:
    default:
      break;
  }

  digitalWrite(STATUS_LED_PIN, ledOn ? HIGH : LOW);
}

void beginWiFiConnection(bool reconnect) {
  if (reconnect) {
    wifiReconnectCount++;
    Serial.print("{\"status\":\"wifi_reconnecting\",\"attempt\":");
    Serial.print(wifiReconnectCount);
    Serial.println("}");
    WiFi.reconnect();
  } else {
    Serial.print("{\"status\":\"wifi_connecting\",\"ssid\":\"");
    Serial.print(WIFI_SSID);
    Serial.println("\"}");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  }

  lastWiFiAttemptAt = millis();
  setLedSignal(LED_WIFI_DISCONNECTED);
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
  otaStarted = true;

  Serial.print("{\"status\":\"ota_ready\",\"hostname\":\"");
  Serial.print(OTA_HOSTNAME);
  Serial.println("\"}");
}

void serviceConnectivity() {
  bool wifiConnected = WiFi.status() == WL_CONNECTED;

  if (wifiConnected && !wifiWasConnected) {
    wifiWasConnected = true;
    setLedSignal(LED_IDLE);
    Serial.print("{\"status\":\"wifi_connected\",\"ip\":\"");
    Serial.print(WiFi.localIP());
    Serial.print("\",\"rssiDbm\":");
    Serial.print(WiFi.RSSI());
    Serial.println("}");

    if (!otaStarted) {
      setupOta();
    }
  }

  if (!wifiConnected && wifiWasConnected) {
    wifiWasConnected = false;
    setLedSignal(LED_WIFI_DISCONNECTED);
    Serial.println("{\"status\":\"wifi_disconnected\"}");
  }

  if (!wifiConnected && millis() - lastWiFiAttemptAt >= WIFI_RECONNECT_INTERVAL_MS) {
    beginWiFiConnection(true);
  }
}

void serviceOta() {
  if (otaStarted) {
    ArduinoOTA.handle();
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
  payload += "\",";

  payload += "\"uptimeSeconds\":";
  payload += String(millis() / 1000);
  payload += ",";

  payload += "\"freeHeapBytes\":";
  payload += String(ESP.getFreeHeap());
  payload += ",";

  payload += "\"wifiReconnectCount\":";
  payload += String(wifiReconnectCount);
  payload += ",";

  payload += "\"uploadFailureCount\":";
  payload += String(uploadFailureCount);
  payload += ",";

  payload += "\"lastSuccessfulUploadSecondsAgo\":";
  if (hasSuccessfulUpload) {
    payload += String((millis() - lastSuccessfulUploadAt) / 1000);
  } else {
    payload += "null";
  }

  payload += "}";

  return payload;
}

void sendWeatherPayload(const String& payload) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("{\"status\":\"upload_skipped\",\"reason\":\"wifi_disconnected\"}");
    return;
  }

  HTTPClient http;
  http.setTimeout(HTTP_TIMEOUT_MS);
  http.begin(API_URL);
  http.addHeader("Content-Type", "application/json");

  int httpResponseCode = http.POST(payload);
  bool uploadSucceeded = httpResponseCode >= 200 && httpResponseCode < 300;

  if (uploadSucceeded) {
    hasSuccessfulUpload = true;
    lastSuccessfulUploadAt = millis();
    setLedSignal(LED_UPLOAD_SUCCESS);
    Serial.print("{\"status\":\"upload_success\",\"httpStatus\":");
    Serial.print(httpResponseCode);
    Serial.println("}");
  } else {
    uploadFailureCount++;
    setLedSignal(LED_UPLOAD_FAILURE);
    Serial.print("{\"status\":\"upload_failed\",\"httpStatus\":");
    Serial.print(httpResponseCode);
    Serial.print(",\"failureCount\":");
    Serial.print(uploadFailureCount);
    Serial.println("}");
  }

  http.end();
}

void serviceRecoveryWatchdog() {
  unsigned long sinceSuccessfulUpload = hasSuccessfulUpload
    ? millis() - lastSuccessfulUploadAt
    : millis();

  if (sinceSuccessfulUpload < UPLOAD_RECOVERY_TIMEOUT_MS) {
    return;
  }

  Serial.print("{\"status\":\"restarting\",\"reason\":\"no_successful_upload\",\"seconds\":");
  Serial.print(sinceSuccessfulUpload / 1000);
  Serial.println("}");
  delay(20);
  ESP.restart();
}

void setup() {
  Serial.begin(115200);

  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);

  WiFi.mode(WIFI_STA);
  beginWiFiConnection(false);

  Wire.begin(SDA_PIN, SCL_PIN);
  lightMeter.begin();

  if (!bmp.begin(SENSOR_ADDRESS)) {
    Serial.println("{\"status\":\"error\",\"message\":\"BMP280 not found\"}");

    while (true) {
      serviceConnectivity();
      serviceOta();
      serviceStatusLed();
      serviceRecoveryWatchdog();
      delay(10);
    }
  }

  Serial.println("{\"status\":\"ok\",\"message\":\"BMP280 initialized\"}");
  lastMeasurementAt = millis() - MEASUREMENT_INTERVAL_MS;
}

void loop() {
  serviceConnectivity();
  serviceOta();
  serviceStatusLed();
  serviceRecoveryWatchdog();

  if (millis() - lastMeasurementAt < MEASUREMENT_INTERVAL_MS) {
    delay(10);
    return;
  }

  lastMeasurementAt = millis();

  float temperature = bmp.readTemperature();
  float pressure = bmp.readPressure() / 100.0;
  float lightLevelLux = lightMeter.readLightLevel();
  String payload = buildWeatherPayload(temperature, pressure, lightLevelLux);

  Serial.println(payload);
  sendWeatherPayload(payload);
}
