#include "../../secrets.h"
#include <Wire.h>
#include <Adafruit_BME280.h>
#include <BH1750.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <HTTPClient.h>

Adafruit_BME280 bme;
BH1750 lightMeter;

const int STATUS_LED_PIN = 2;
const int SDA_PIN = 21;
const int SCL_PIN = 22;
const unsigned long MEASUREMENT_INTERVAL_MS = 5000;
const unsigned long WIFI_RECONNECT_INTERVAL_MS = 10000;
const unsigned long SENSOR_RETRY_INTERVAL_MS = 30000;
const unsigned long I2C_FAILURE_SCAN_INTERVAL_MS = 5UL * 60UL * 1000UL;
const unsigned long UPLOAD_RECOVERY_TIMEOUT_MS = 10UL * 60UL * 1000UL;
const unsigned long UPLOAD_FAILURE_LOG_INTERVAL_MS = 60UL * 1000UL;
const uint16_t HTTP_TIMEOUT_MS = 4000;
const uint16_t DEVICE_LOG_HTTP_TIMEOUT_MS = 750;
const uint8_t PRIMARY_SENSOR_ADDRESS = 0x76;
const uint8_t SECONDARY_SENSOR_ADDRESS = 0x77;
const char* FIRMWARE_VERSION = "0.2.3-i2c-diagnostics";

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
unsigned long lastSensorInitializationAttemptAt = 0;
unsigned long lastI2cScanAt = 0;
unsigned long lastSuccessfulUploadAt = 0;
unsigned long lastUploadFailureLogAt = 0;
unsigned long wifiReconnectCount = 0;
unsigned long uploadFailureCount = 0;
bool hasSuccessfulUpload = false;
bool hasReportedUploadFailure = false;
bool hasReportedSensorState = false;
bool reportedSensorReady = false;
bool hasReportedBoot = false;
bool wifiWasConnected = false;
bool otaStarted = false;
bool bmeReady = false;
bool hasCompletedI2cScan = false;
bool hasPendingI2cScanLog = false;
uint8_t bmeAddress = 0;
String latestI2cScanMessage;

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

String escapeJsonString(String value) {
  value.replace("\\", "\\\\");
  value.replace("\"", "\\\"");
  value.replace("\r", "\\r");
  value.replace("\n", "\\n");
  return value;
}

String deviceLogsUrl() {
  String url = API_URL;
  const String weatherPath = "/weather";

  while (url.endsWith("/")) {
    url.remove(url.length() - 1);
  }

  if (url.endsWith(weatherPath)) {
    url.remove(url.length() - weatherPath.length());
  }

  url += "/device/logs";
  return url;
}

void sendDeviceLog(const char* level, const char* event, const String& message = "") {
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  String payload = "{";
  payload += "\"level\":\"";
  payload += escapeJsonString(String(level));
  payload += "\",\"event\":\"";
  payload += escapeJsonString(String(event));
  payload += "\",\"message\":\"";
  payload += escapeJsonString(message);
  payload += "\",\"firmwareVersion\":\"";
  payload += escapeJsonString(String(FIRMWARE_VERSION));
  payload += "\",\"uptimeSeconds\":";
  payload += String(millis() / 1000);
  payload += "}";

  HTTPClient http;
  http.setTimeout(DEVICE_LOG_HTTP_TIMEOUT_MS);
  http.begin(deviceLogsUrl());
  http.addHeader("Content-Type", "application/json");
  http.POST(payload);
  http.end();
}

void scanI2cBus() {
  String detectedAddresses;

  for (uint8_t address = 1; address <= 126; address++) {
    Wire.beginTransmission(address);
    uint8_t transmissionResult = Wire.endTransmission();

    if (transmissionResult == 0) {
      if (detectedAddresses.length() > 0) {
        detectedAddresses += ", ";
      }

      String addressHex = String(address, HEX);
      addressHex.toUpperCase();
      detectedAddresses += "0x";
      detectedAddresses += addressHex;
    }
  }

  if (detectedAddresses.length() > 0) {
    latestI2cScanMessage = String("Detected I2C devices: ") + detectedAddresses;
  } else {
    latestI2cScanMessage = "No I2C devices detected";
  }

  lastI2cScanAt = millis();
  hasCompletedI2cScan = true;

  Serial.print("{\"status\":\"i2c_scan_result\",\"message\":\"");
  Serial.print(latestI2cScanMessage);
  Serial.println("\"}");

  if (WiFi.status() == WL_CONNECTED) {
    sendDeviceLog("info", "i2c_scan_result", latestI2cScanMessage);
    hasPendingI2cScanLog = false;
  } else {
    hasPendingI2cScanLog = true;
  }
}

void sendPendingI2cScanResult() {
  if (hasPendingI2cScanLog && WiFi.status() == WL_CONNECTED) {
    sendDeviceLog("info", "i2c_scan_result", latestI2cScanMessage);
    hasPendingI2cScanLog = false;
  }
}

void reportSensorState() {
  if (
    WiFi.status() != WL_CONNECTED ||
    (hasReportedSensorState && reportedSensorReady == bmeReady)
  ) {
    return;
  }

  if (bmeReady) {
    sendDeviceLog(
      "info",
      "sensor_init_success",
      String("BME280 initialized at I2C address ") +
        (bmeAddress == PRIMARY_SENSOR_ADDRESS ? "0x76" : "0x77")
    );
  } else {
    sendDeviceLog(
      "error",
      "sensor_init_failed",
      "BME280 initialization failed at I2C addresses 0x76 and 0x77"
    );
  }

  hasReportedSensorState = true;
  reportedSensorReady = bmeReady;
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

const char* otaErrorMessage(ota_error_t error) {
  switch (error) {
    case OTA_AUTH_ERROR:
      return "authentication failed";
    case OTA_BEGIN_ERROR:
      return "begin failed";
    case OTA_CONNECT_ERROR:
      return "connect failed";
    case OTA_RECEIVE_ERROR:
      return "receive failed";
    case OTA_END_ERROR:
      return "end failed";
    default:
      return "unknown error";
  }
}

void setupOta() {
  ArduinoOTA.setHostname(OTA_HOSTNAME);
  ArduinoOTA.setPassword(OTA_PASSWORD);

  ArduinoOTA.onStart([]() {
    Serial.print("{\"status\":\"ota_start\",\"type\":\"");
    Serial.print(ArduinoOTA.getCommand() == U_FLASH ? "firmware" : "filesystem");
    Serial.println("\"}");
    sendDeviceLog(
      "info",
      "ota_start",
      ArduinoOTA.getCommand() == U_FLASH ? "Firmware update started" : "Filesystem update started"
    );
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.print("{\"status\":\"ota_progress\",\"percent\":");
    Serial.print((progress * 100) / total);
    Serial.println("}");
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("{\"status\":\"ota_success\"}");
    sendDeviceLog("info", "ota_success", "OTA update completed");
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.print("{\"status\":\"ota_error\",\"code\":");
    Serial.print(error);
    Serial.print(",\"message\":\"");
    Serial.print(otaErrorMessage(error));
    Serial.println("\"}");
    sendDeviceLog("error", "ota_error", otaErrorMessage(error));
  });

  ArduinoOTA.begin();
  otaStarted = true;

  Serial.print("{\"status\":\"ota_ready\",\"hostname\":\"");
  Serial.print(OTA_HOSTNAME);
  Serial.println("\"}");
  sendDeviceLog("info", "ota_ready", String("OTA ready at hostname ") + OTA_HOSTNAME);
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
    sendDeviceLog(
      "info",
      "wifi_connected",
      String("IP ") + WiFi.localIP().toString() + ", RSSI " + String(WiFi.RSSI()) + " dBm"
    );

    if (!hasReportedBoot) {
      sendDeviceLog("info", "boot", String("Firmware ") + FIRMWARE_VERSION + " booted");
      hasReportedBoot = true;
    }

    sendPendingI2cScanResult();
    reportSensorState();

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

bool initializeBme280() {
  lastSensorInitializationAttemptAt = millis();

  if (bme.begin(PRIMARY_SENSOR_ADDRESS)) {
    bmeAddress = PRIMARY_SENSOR_ADDRESS;
    Serial.println("{\"status\":\"ok\",\"message\":\"BME280 initialized at I2C address 0x76\"}");
    return true;
  }

  if (bme.begin(SECONDARY_SENSOR_ADDRESS)) {
    bmeAddress = SECONDARY_SENSOR_ADDRESS;
    Serial.println("{\"status\":\"ok\",\"message\":\"BME280 initialized at I2C address 0x77\"}");
    return true;
  }

  bmeAddress = 0;
  Serial.println(
    "{\"status\":\"sensor_error\",\"sensor\":\"BME280\",\"message\":\"BME280 initialization failed at I2C addresses 0x76 and 0x77\"}"
  );

  if (
    !hasCompletedI2cScan ||
    millis() - lastI2cScanAt >= I2C_FAILURE_SCAN_INTERVAL_MS
  ) {
    scanI2cBus();
  }

  return false;
}

String buildWeatherPayload(
  float temperature,
  float pressure,
  float humidityPercent,
  float lightLevelLux
) {
  String payload = "{";

  payload += "\"temperatureCelsius\":";
  payload += String(temperature, 2);
  payload += ",";

  payload += "\"pressureHpa\":";
  payload += String(pressure, 2);
  payload += ",";

  payload += "\"humidityPercent\":";
  payload += String(humidityPercent, 2);
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
  bool shouldReportUploadFailure = false;

  if (uploadSucceeded) {
    hasSuccessfulUpload = true;
    hasReportedUploadFailure = false;
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

    if (
      !hasReportedUploadFailure ||
      millis() - lastUploadFailureLogAt >= UPLOAD_FAILURE_LOG_INTERVAL_MS
    ) {
      shouldReportUploadFailure = true;
      hasReportedUploadFailure = true;
      lastUploadFailureLogAt = millis();
    }
  }

  http.end();

  if (shouldReportUploadFailure) {
    sendDeviceLog(
      "error",
      "upload_failed",
      String("Weather upload HTTP status ") + String(httpResponseCode) +
        ", failure count " + String(uploadFailureCount)
    );
  }
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
  sendDeviceLog(
    "error",
    "recovery_restart",
    String("No successful upload for ") + String(sinceSuccessfulUpload / 1000) + " seconds"
  );
  delay(20);
  ESP.restart();
}

void setup() {
  Serial.begin(115200);
  Serial.print("{\"status\":\"boot\",\"firmwareVersion\":\"");
  Serial.print(FIRMWARE_VERSION);
  Serial.println("\"}");

  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);

  WiFi.mode(WIFI_STA);
  beginWiFiConnection(false);

  Wire.begin(SDA_PIN, SCL_PIN);
  bmeReady = initializeBme280();

  if (!hasCompletedI2cScan) {
    scanI2cBus();
  }

  lightMeter.begin();
  lastMeasurementAt = millis() - MEASUREMENT_INTERVAL_MS;
}

void loop() {
  serviceConnectivity();
  serviceOta();
  serviceStatusLed();
  serviceRecoveryWatchdog();

  if (!bmeReady) {
    if (millis() - lastSensorInitializationAttemptAt >= SENSOR_RETRY_INTERVAL_MS) {
      bmeReady = initializeBme280();
      reportSensorState();
    }

    delay(10);
    return;
  }

  if (millis() - lastMeasurementAt < MEASUREMENT_INTERVAL_MS) {
    delay(10);
    return;
  }

  lastMeasurementAt = millis();

  float temperature = bme.readTemperature();
  float pressure = bme.readPressure() / 100.0;
  float humidityPercent = bme.readHumidity();
  float lightLevelLux = lightMeter.readLightLevel();
  String payload = buildWeatherPayload(temperature, pressure, humidityPercent, lightLevelLux);

  Serial.println(payload);
  sendWeatherPayload(payload);
}
