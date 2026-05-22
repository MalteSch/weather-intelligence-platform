# Weather Intelligence Platform

Personal weather station and backend learning project based on ESP32.

The goal of this project is to collect local weather data from real sensors, send it to backend services and later generate AI-supported weather insights based on historical measurements and trends.

## Goals

- Learn Go
- Improve TypeScript skills
- Work with ESP32-based sensor hardware
- Collect real-world weather data
- Build event-driven backend systems
- Create a dashboard for local weather data
- Generate AI-supported weather interpretations

## Current status

- [x] ESP32 setup
- [x] BMP280 sensor communication
- [x] JSON output
- [x] WiFi connectivity
- [x] Local secrets handling
- [x] GitHub repository setup
- [ ] BME280 integration
- [ ] OLED display
- [ ] HTTP data upload
- [ ] Backend API
- [ ] Dashboard
- [ ] AI forecast generation

## Current sensor output

```json
{
  "temperatureCelsius": 26.42,
  "pressureHpa": 1016.81,
  "sensor": "BMP280",
  "wifiIp": "192.168.0.148",
  "wifiRssiDbm": -54
}
```

## Project structure

```text
sensors/
  esp32-weather-node/

backend/

dashboard/

docs/
```

## Local configuration

WiFi credentials are stored in a local `secrets.h` file. This file is intentionally excluded from Git via `.gitignore`.

Create a local `secrets.h` file in the project root:

```cpp
#pragma once

const char* WIFI_SSID = "your_wifi_name";
const char* WIFI_PASSWORD = "your_wifi_password";
const char* API_URL = "https://httpbin.org/post";
```

`API_URL` defines the endpoint used by the ESP32 to upload weather measurements. During development this can point to a test endpoint such as httpbin. Later it will point to the local backend API.

A public template is provided as:

```text
secrets.example.h
```

## Hardware

Currently used:

- ESP32 development board
- BMP280 temperature and pressure sensor

Planned / available:

- BME280 temperature, pressure and humidity sensor
- OLED display
- Hall sensors and magnets
- Light sensor

## Roadmap

### Phase 1: Sensor node

- Read temperature and pressure
- Output structured JSON
- Connect to WiFi
- Add humidity support via BME280
- Display current values on OLED

### Phase 2: Backend

- Send weather data via HTTP
- Store incoming measurements
- Provide latest and historical weather data through an API

### Phase 3: Dashboard

- Display current weather values
- Visualize historical trends
- Show sensor and connectivity status

### Phase 4: Intelligence

- Generate AI-supported weather summaries
- Interpret pressure, temperature and humidity trends
- Compare local measurements with external weather forecasts