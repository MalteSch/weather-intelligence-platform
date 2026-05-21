# Weather Intelligence Platform

Personal weather station and backend learning project based on ESP32.

## Goals

- Learn Go
- Improve TypeScript skills
- Collect real-world weather data
- Build event-driven backend systems
- Generate AI-supported forecasts

## Current status

- [x] ESP32 setup
- [x] Sensor communication
- [x] JSON output
- [x] GitHub repository setup
- [X] WiFi connectivity
- [ ] OLED display
- [ ] BME280 integration
- [ ] Backend API
- [ ] Dashboard
- [ ] AI forecast generation

## Project structure

```text
sensors/
    esp32-weather-node/

backend/

dashboard/

docs/
```

## Local configuration

Create a `secrets.h` file in the project root:

```cpp
#pragma once

const char* WIFI_SSID = "your_wifi_name";
const char* WIFI_PASSWORD = "your_password";
```

This file is intentionally excluded from Git via `.gitignore`.

## Example sensor output

```json
{
  "temperatureCelsius":26.42,
  "pressureHpa":1016.81,
  "sensor":"BMP280",
  "wifiIp":"192.168.0.123",
  "wifiRssiDbm":-54
}
```