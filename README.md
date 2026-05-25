# Weather Intelligence Platform

ESP32-based weather station platform with persistent storage, live dashboard visualization and containerized deployment.

---

# Project Goals

This project is intentionally built as a learning-oriented full-stack platform.

Primary learning areas:

- Go / TypeScript ecosystem understanding
- ESP32 firmware development
- Backend API development
- Docker & containerized deployment
- GitHub Actions CI/CD
- SQLite persistence
- Time-series visualization
- MQTT architecture (planned)
- Infrastructure & self-hosting

The system is designed to evolve incrementally instead of being overengineered from the beginning.

---

# Current Architecture

```text
ESP32 Sensor Node
        │
        │ HTTP POST
        ▼
Node.js / Express Backend
        │
        ├── SQLite persistence
        ├── REST API
        └── Static dashboard
                │
                ▼
        Chart.js frontend
```

Deployment:

```text
Git Push
   │
   ▼
GitHub Actions
   │
   ▼
Build Docker Image
   │
   ▼
Push to GHCR
   │
   ▼
Synology NAS pulls image
```

---

# Hardware

## Current Hardware Setup

### Controller

- ESP32 Development Board
- USB UART: CH340C

### Sensors

#### BME280

Measures:

- temperature
- air pressure

#### Light Sensor

Currently used for ambient brightness measurements.

### Connectivity

- WiFi-based communication
- HTTP measurement upload to backend

### Planned Hardware Extensions

- humidity sensing
- outdoor enclosure
- battery/solar operation
- additional distributed sensor nodes

---

# Features

## Implemented

### ESP32 Sensor Node

- Temperature measurement
- Pressure measurement
- Light sensor support
- WiFi signal strength reporting
- HTTP-based measurement upload
- Periodic data transmission

### Backend

- Express REST API
- SQLite persistence
- Measurement history endpoint
- Latest measurement endpoint
- Time-range filtering
- Dockerized runtime

### Dashboard

- Live weather dashboard
- Real-time chart updates
- Temperature graph
- Light level graph
- Real timestamp-based chart scaling
- Time range presets
- Custom date/time filtering

### Infrastructure

- Docker image builds via GitHub Actions
- GitHub Container Registry publishing
- Synology deployment workflow
- Persistent SQLite storage
- Reverse proxy compatible

---

# Repository Structure

```text
backend/
├── public/
│   └── index.html
├── data/
├── database.js
├── package.json
├── server.js
└── Dockerfile

firmware/
└── esp32/

.github/
└── workflows/
```

---

# API

## POST /weather

Receives weather measurements from the ESP32.

Example payload:

```json
{
  "temperatureCelsius": 22.4,
  "pressureHpa": 1013.2,
  "lightLevelLux": 120,
  "wifiRssiDbm": -52
}
```

---

## GET /weather/latest

Returns the latest measurement from SQLite.

Example response:

```json
{
  "id": 294,
  "temperatureCelsius": 22.4,
  "pressureHpa": 1013.2,
  "lightLevelLux": 120,
  "wifiRssiDbm": -52,
  "receivedAt": "2026-05-23T21:28:08.614Z"
}
```

---

## GET /weather/history

Returns historical measurements.

Supports optional time filtering:

```text
/weather/history?from=2026-05-23T00:00:00.000Z&to=2026-05-24T00:00:00.000Z
```

---

# SQLite Schema

Current table:

```sql
CREATE TABLE weather_measurements (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  temperature_celsius REAL,
  pressure_hpa REAL,
  light_level_lux REAL,
  wifi_rssi_dbm INTEGER,
  received_at TEXT
);
```

Database values use snake_case.

API responses use camelCase.

---

# Dashboard

The dashboard is intentionally implemented using:

- plain HTML
- plain CSS
- plain JavaScript
- Chart.js

No frontend framework is currently used.

This keeps the architecture simple and educational.

Current dashboard capabilities:

- live measurement updates
- smooth chart updates
- real time-axis scaling
- selectable time windows
- custom date filtering

---

# Docker

## Local Build

```bash
cd backend

docker build -t weather-backend .
```

---

## Local Run

```bash
docker run -p 3000:3000 weather-backend
```

---

# Synology Deployment

Deployment target:

- Synology NAS
- Docker Compose
- GHCR-based image pulls

The SQLite database is persisted through mounted volumes.

Example deployment update flow:

```bash
docker compose pull

docker rm -f weather_backend

docker compose up -d
```

---

# GitHub Actions / CI

Current CI pipeline:

1. Push to repository
2. GitHub Actions builds Docker image
3. Image is pushed to GitHub Container Registry
4. Synology pulls latest image

---

# Important Notes

## sqlite3 + Docker

The `sqlite3` package must be built inside the container image.

Do NOT copy local `node_modules` into Docker images.

This avoids glibc compatibility problems between systems.

---

## Time-Series Visualization

Charts use real timestamp scaling.

This ensures outages or missing measurements appear as actual gaps on the timeline.

---

# Planned Features

## Near-term

- Pressure chart
- Humidity sensor
- Better dashboard styling
- Chart legends & statistics
- Health monitoring endpoint
- Improved deployment tooling

## Mid-term

- MQTT migration
- ESP-side caching/buffering
- Multiple sensor nodes
- Authentication
- Historical aggregation
- Data export

## Long-term

- Go backend migration experiments
- TypeScript frontend/backend exploration
- Alerting system
- Grafana integration
- InfluxDB experiments
- Home Assistant integration

---

# Development Philosophy

This project intentionally prioritizes:

- incremental learning
- understandable architecture
- visible system evolution
- practical infrastructure experience

The goal is not merely building a weather station.

The goal is learning how modern software systems evolve from simple prototypes into robust services.