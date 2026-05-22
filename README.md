# Weather Intelligence Platform

Personal weather station and backend learning project based on ESP32.

The goal of this project is to collect local weather data from real sensors, send it to backend services and later generate AI-supported weather insights based on historical measurements and trends.

---

# Goals

- Learn Go
- Improve TypeScript skills
- Work with ESP32-based sensor hardware
- Collect real-world weather data
- Build event-driven backend systems
- Create a dashboard for local weather data
- Generate AI-supported weather interpretations

---

# Current status

- [x] ESP32 setup
- [x] BMP280 sensor communication
- [x] BH1750 light sensor integration
- [x] JSON output
- [x] WiFi connectivity
- [x] HTTP sensor uploads
- [x] Local backend API
- [x] Live dashboard
- [x] Local secrets handling
- [x] LED upload status indicator
- [x] GitHub repository setup
- [ ] BME280 integration
- [ ] OLED display
- [ ] Historical data storage
- [ ] OTA updates
- [ ] AI forecast generation

---

# Current sensor output

```json
{
  "temperatureCelsius": 27.71,
  "pressureHpa": 1018.26,
  "sensor": "BMP280",
  "wifiIp": "192.168.0.148",
  "wifiRssiDbm": -71,
  "lightLevelLux": 28.33
}
```

---

# Project structure

```text
sensors/
  esp32-weather-node/

backend/
  public/

docs/

tools/
```

---

# Local configuration

WiFi credentials and local API configuration are stored in a local `secrets.h` file.

This file is intentionally excluded from Git via `.gitignore`.

Create a local `secrets.h` file in the project root:

```cpp
#pragma once

const char* WIFI_SSID = "your_wifi_name";
const char* WIFI_PASSWORD = "your_wifi_password";
const char* API_URL = "http://your_local_backend:3000/weather";
```

A public template is provided as:

```text
secrets.example.h
```

`API_URL` defines the endpoint used by the ESP32 to upload weather measurements.

During development this can point to:
- a public test endpoint
- a local backend server
- later potentially a cloud API

---

# Hardware

## Currently used

- ESP32 development board
- BMP280 temperature and pressure sensor
- BH1750 light sensor
- USB power bank

## Planned / available

- BME280 temperature, pressure and humidity sensor
- OLED display
- Hall sensors and magnets
- Outdoor housing via 3D printing
- Solar power supply

---

# Backend API

## POST `/weather`

Receives weather measurements from the ESP32 sensor node.

## GET `/weather/latest`

Returns the latest received weather measurement.

## GET `/health`

Simple backend health check endpoint.

---

# Dashboard

The dashboard currently displays:

- Temperature
- Pressure
- WiFi signal strength
- Last update timestamp

Planned:
- historical charts
- light visualization
- outdoor status
- device health monitoring

---

# Roadmap

## Phase 1: Sensor node

- Read temperature and pressure
- Output structured JSON
- Connect to WiFi
- Add light sensor support
- Add humidity support via BME280
- Display current values on OLED

## Phase 2: Backend

- Send weather data via HTTP
- Store incoming measurements
- Provide latest and historical weather data through an API

## Phase 3: Dashboard

- Display current weather values
- Visualize historical trends
- Show sensor and connectivity status
- Auto-refresh live values

## Phase 4: Outdoor operation

- Powerbank operation
- Outdoor sensor placement
- OTA firmware updates
- Connection monitoring
- Upload retry handling

## Phase 5: Advanced sensors

- Humidity via BME280
- Wind speed via hall sensor
- Rain detection
- Day/night detection

## Phase 6: Data & intelligence

- Historical measurement storage
- Weather trend visualization
- AI-supported weather summaries
- Forecast comparison

---

# Development notes

The project intentionally combines:

- Embedded development
- Networking
- Backend APIs
- Frontend dashboards
- Dev tooling
- Git/GitHub workflows

The focus is not only the final weather station itself, but also learning modern software engineering practices through a real-world project.