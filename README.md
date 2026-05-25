# Weather Intelligence Platform

## 1. Project Overview

This repository contains a personal ESP32 weather station and its supporting
web service. An ESP32 samples environmental sensors, sends measurements and
device health telemetry by HTTP, and supports local-network over-the-air
(OTA) firmware updates. A Node.js/Express backend stores readings in SQLite
and serves a static browser dashboard.

The project is intentionally learning-oriented: it uses a small, inspectable
architecture while exercising embedded programming, REST APIs, persistent
storage, visualization, containers, and self-hosted deployment.

## 2. Architecture Overview

```text
BMP280 + BH1750 sensors
        |
        | I2C
        v
ESP32 weather node
  - samples every 5 seconds
  - reports firmware/device health
  - services ArduinoOTA
        |
        | HTTP POST /weather over WiFi
        v
Node.js 20 / Express backend
  - validates and stores measurements
  - exposes REST endpoints
  - serves static dashboard
        |
        +---- SQLite database in /app/data
        |
        +---- HTML/CSS/JavaScript dashboard using Chart.js
```

Deployment path:

```text
Push backend changes to main
        |
        v
GitHub Actions builds backend Docker image
        |
        v
GHCR publishes latest and SHA-tagged images
        |
        v
Operator runs Docker Compose pull/recreate on Synology
        |
        v
Mounted SQLite volume preserves measurements
```

Firmware upload and backend deployment are separate operations. The current
GitHub Actions workflow publishes the backend container; it does not flash
the ESP32.

## 3. Hardware

The implemented firmware in
`sensors/esp32-weather-node/esp32-weather-node.ino` uses:

| Component | Purpose | Firmware configuration |
| --- | --- | --- |
| ESP32 development board | Sensor controller, WiFi client and OTA target | WiFi station mode |
| BMP280 | Temperature and barometric pressure | I2C address `0x76` |
| BH1750 | Ambient light measurement | I2C |
| On-board/external status LED | Connection and upload feedback | GPIO `2` |

I2C wiring configured in firmware:

| Signal | ESP32 pin |
| --- | --- |
| SDA | GPIO `21` |
| SCL | GPIO `22` |

Humidity measurement is not currently implemented.

## 4. Features

Implemented functionality:

- ESP32 acquisition of temperature, pressure and light level.
- WiFi RSSI reporting alongside each measurement.
- HTTP measurement uploads every 5 seconds when WiFi is connected.
- Firmware version reporting through `firmwareVersion`.
- Device health telemetry for uptime, heap memory, reconnects and upload
  failures.
- ArduinoOTA service with password configuration and serial status events.
- Upload recovery watchdog that restarts an ESP32 unable to complete a
  successful upload for 10 minutes.
- Express REST API with validation and SQLite persistence.
- Backward-compatible SQLite startup migrations for added optional columns.
- Static dashboard with current readings, firmware/device state, timestamped
  charts and selectable time ranges.
- Docker packaging and GitHub Container Registry (GHCR) publishing.
- Docker Compose deployment configuration for a Synology-hosted instance.

## 5. Repository Structure

```text
.
|-- .github/
|   `-- workflows/
|       `-- docker-image.yml       # Builds and publishes the backend image
|-- backend/
|   |-- public/
|   |   `-- index.html             # Static dashboard
|   |-- database.js                # SQLite initialization and migrations
|   |-- server.js                  # Express API and static hosting
|   |-- Dockerfile                 # Node 20 backend image
|   |-- package.json
|   `-- weather.db                 # Existing local database snapshot
|-- sensors/
|   `-- esp32-weather-node/
|       `-- esp32-weather-node.ino # Main ESP32 firmware sketch
|-- tools/
|   `-- i2c-scanner/
|       `-- i2c-scanner.ino       # Sensor bus diagnostic sketch
|-- docker-compose.yml             # Synology/container runtime definition
|-- secrets.example.h              # Firmware configuration template
`-- README.md
```

The Docker runtime database is not `backend/weather.db`. Inside the container,
the backend opens `data/weather.db`, which is persisted through the
`./data:/app/data` Compose volume mount.

## 6. Firmware

The firmware is a single Arduino sketch. It uses these libraries:

- `Wire`
- `Adafruit_BMP280`
- `BH1750`
- `WiFi`
- `ArduinoOTA`
- `HTTPClient`

Create a local `secrets.h` based on `secrets.example.h` before compiling:

```cpp
#pragma once

const char* WIFI_SSID = "your_wifi_name";
const char* WIFI_PASSWORD = "your_wifi_password";
const char* API_URL = "http://your-backend-address:3000/weather";
const char* OTA_HOSTNAME = "esp32-weather-node";
const char* OTA_PASSWORD = "choose_a_strong_ota_password";
```

`secrets.h` is intentionally ignored by Git. Do not commit WiFi credentials
or OTA passwords.

Current runtime behavior:

- Initializes I2C on GPIO `21`/`22`, the BH1750, and the BMP280 at `0x76`.
- Attempts WiFi reconnects at 10-second intervals after disconnection.
- Starts ArduinoOTA after a WiFi connection is established.
- Takes and uploads readings every 5 seconds.
- Uses a 4-second HTTP client timeout.
- Logs structured JSON-style status messages to serial at `115200` baud.
- Continues servicing WiFi, OTA, LED state and recovery logic if BMP280
  initialization fails.

The firmware currently identifies itself as `0.1.0-ota-test` through the
`FIRMWARE_VERSION` constant. Change that constant when producing a new
firmware build so reported versions remain useful.

## 7. OTA Updates

ArduinoOTA support is implemented in the ESP32 firmware. OTA is enabled only
after the device joins WiFi, using `OTA_HOSTNAME` and `OTA_PASSWORD` from
`secrets.h`.

Practical OTA workflow:

1. Flash a firmware build containing the ArduinoOTA support and correct WiFi
   and OTA settings by USB at least once.
2. Confirm the serial log reports `ota_ready` with the expected hostname.
3. Keep the development computer and ESP32 reachable on the same network.
4. Select the network OTA target in an Arduino-compatible upload tool and
   upload the new sketch using the configured OTA password.
5. Confirm `ota_start`, `ota_progress` and `ota_success` serial events, then
   check the dashboard's reported firmware version after a new measurement.

The sketch services `ArduinoOTA.handle()` during ordinary operation and while
waiting on a missing BMP280. OTA failures are reported over serial with the
ArduinoOTA error category.

There is currently no automated firmware build or OTA deployment workflow
under `.github/workflows/`; OTA deployment is a local device operation.

## 8. Health Telemetry & Recovery Watchdog

Each firmware upload includes device health data in addition to sensor
measurements:

| API field | Meaning |
| --- | --- |
| `firmwareVersion` | Value of the compiled firmware version constant |
| `uptimeSeconds` | ESP32 uptime at sampling time |
| `freeHeapBytes` | Free heap reported by the ESP32 |
| `wifiReconnectCount` | Number of reconnect attempts since boot |
| `uploadFailureCount` | Failed HTTP upload attempts since boot |
| `lastSuccessfulUploadSecondsAgo` | Age of the prior successful upload, or `null` before one succeeds |

An HTTP response in the `200` through `299` range is considered a successful
upload. Failed HTTP uploads increment `uploadFailureCount`; an upload skipped
because WiFi is disconnected is logged but does not increment that counter.

The recovery watchdog calls `ESP.restart()` when there has been no successful
upload for 10 minutes. This applies both after startup before the first
successful upload and after a previously working upload path stops succeeding.

Status LED behavior on GPIO `2`:

| State | LED pattern |
| --- | --- |
| Successful upload | One short 100 ms pulse |
| Failed upload | Two short pulses |
| WiFi disconnected/reconnecting | One pulse approximately every second |
| Idle while connected | Off |

## 9. REST API

The backend listens on port `3000`. API JSON uses `camelCase`, while SQLite
columns use `snake_case`.

### `POST /weather`

Stores one measurement. The four measurement fields are required. Firmware
and health fields are optional to preserve compatibility with older sensor
payloads.

```json
{
  "temperatureCelsius": 22.41,
  "pressureHpa": 1013.27,
  "lightLevelLux": 118.5,
  "wifiRssiDbm": -52,
  "firmwareVersion": "0.1.0-ota-test",
  "uptimeSeconds": 420,
  "freeHeapBytes": 214368,
  "wifiReconnectCount": 0,
  "uploadFailureCount": 1,
  "lastSuccessfulUploadSecondsAgo": 5
}
```

Successful response:

```json
{
  "status": "ok",
  "message": "Weather data stored",
  "id": 295
}
```

The firmware also includes `wifiIp` in its outgoing JSON. The current backend
does not store or return that property.

### `GET /weather/latest`

Returns the newest stored row, ordered by `received_at`. It returns HTTP `404`
when no measurement exists.

```json
{
  "id": 295,
  "temperatureCelsius": 22.41,
  "pressureHpa": 1013.27,
  "lightLevelLux": 118.5,
  "wifiRssiDbm": -52,
  "firmwareVersion": "0.1.0-ota-test",
  "uptimeSeconds": 420,
  "freeHeapBytes": 214368,
  "wifiReconnectCount": 0,
  "uploadFailureCount": 1,
  "lastSuccessfulUploadSecondsAgo": 5,
  "receivedAt": "2026-05-25T18:42:00.000Z"
}
```

### `GET /weather/history`

Returns measurement rows in ascending timestamp order. Supplying both `from`
and `to` applies an inclusive `received_at BETWEEN ? AND ?` time range.

```http
GET /weather/history?from=2026-05-24T00:00:00.000Z&to=2026-05-25T00:00:00.000Z
```

Without both query parameters, the endpoint returns the newest 2000 rows,
ordered chronologically for display. A single supplied boundary does not
currently apply partial filtering.

### `GET /health`

Provides backend process liveness only:

```json
{
  "status": "ok"
}
```

This endpoint is distinct from ESP32 health telemetry, which is stored with
weather measurements and returned by the weather read endpoints.

## 10. SQLite Schema

The backend creates or upgrades the `weather_measurements` table during
startup. The schema expected after current startup migrations is:

```sql
CREATE TABLE IF NOT EXISTS weather_measurements (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  temperature_celsius REAL NOT NULL,
  pressure_hpa REAL NOT NULL,
  light_level_lux REAL NOT NULL,
  wifi_rssi_dbm INTEGER NOT NULL,
  firmware_version TEXT,
  uptime_seconds INTEGER,
  free_heap_bytes INTEGER,
  wifi_reconnect_count INTEGER,
  upload_failure_count INTEGER,
  last_successful_upload_seconds_ago INTEGER,
  received_at TEXT NOT NULL
);
```

Older persistent databases are upgraded in place. On startup,
`backend/database.js` checks `PRAGMA table_info(weather_measurements)` and
runs `ALTER TABLE ... ADD COLUMN` for missing optional columns:

```sql
ALTER TABLE weather_measurements ADD COLUMN firmware_version TEXT;
ALTER TABLE weather_measurements ADD COLUMN uptime_seconds INTEGER;
ALTER TABLE weather_measurements ADD COLUMN free_heap_bytes INTEGER;
ALTER TABLE weather_measurements ADD COLUMN wifi_reconnect_count INTEGER;
ALTER TABLE weather_measurements ADD COLUMN upload_failure_count INTEGER;
ALTER TABLE weather_measurements
  ADD COLUMN last_successful_upload_seconds_ago INTEGER;
```

The existing local SQLite snapshots in the repository/workspace may still
show the earlier six-column schema until opened by the current backend.
Mounted deployment data should be backed up before container upgrades.

Field naming boundary:

| SQLite column | API property |
| --- | --- |
| `temperature_celsius` | `temperatureCelsius` |
| `pressure_hpa` | `pressureHpa` |
| `light_level_lux` | `lightLevelLux` |
| `wifi_rssi_dbm` | `wifiRssiDbm` |
| `firmware_version` | `firmwareVersion` |
| `uptime_seconds` | `uptimeSeconds` |
| `free_heap_bytes` | `freeHeapBytes` |
| `wifi_reconnect_count` | `wifiReconnectCount` |
| `upload_failure_count` | `uploadFailureCount` |
| `last_successful_upload_seconds_ago` | `lastSuccessfulUploadSecondsAgo` |
| `received_at` | `receivedAt` |

## 11. Dashboard

The dashboard is served from `backend/public/index.html` by Express static
hosting. It deliberately uses plain HTML, CSS and JavaScript, with no
frontend framework or build step.

External browser-side libraries:

- Chart.js for rendering line charts.
- Luxon and `chartjs-adapter-luxon` for Chart.js time-axis support.

Implemented UI behavior:

- Current cards for temperature, pressure, light level, WiFi RSSI and last
  update.
- Online/stale state based on whether the latest reading is newer than
  5 minutes.
- Display of reported firmware version and device health telemetry.
- Temperature, pressure and light-level history charts.
- Chart.js `time` x-axes driven by real `receivedAt` timestamps, preserving
  visible time gaps when data is missing.
- Rolling presets for 12 hours, 24 hours, 3 days and 7 days.
- Custom `From`/`To` time filtering with validation against invalid and
  future ranges.
- Automatic data refresh every 5 seconds.

## 12. Docker Usage

The backend image is based on Node 20 (`node:20-bookworm`). The Dockerfile
installs build tools and runs:

```bash
npm install --omit=dev --build-from-source=sqlite3
```

Building `sqlite3` inside the image avoids native binary compatibility issues,
including glibc mismatches between local environments and the deployed
container. Local `node_modules` is excluded through `backend/.dockerignore`
and must not be copied into the image.

Build and run the backend locally with a persistent database directory:

```bash
docker build -t weather-backend ./backend
mkdir -p data
docker run --rm \
  -p 3000:3000 \
  -v "$(pwd)/data:/app/data" \
  weather-backend
```

For native Node development:

```bash
cd backend
mkdir -p data
npm install
npm start
```

The `data` directory is required because the application opens
`data/weather.db` relative to its working directory.

## 13. Synology Deployment

`docker-compose.yml` defines the deployed backend service:

```yaml
services:
  weather-backend:
    image: ghcr.io/maltesch/weather-backend:latest
    container_name: weather_backend
    ports:
      - "3000:3000"
    volumes:
      - ./data:/app/data
    restart: unless-stopped
```

Typical update flow on the Synology host:

```bash
docker compose pull weather-backend
docker compose up -d --force-recreate weather-backend
docker compose ps
```

After an update, verify the deployment rather than assuming the newest image
is running:

- Confirm the container uses the intended `latest` or SHA-tagged GHCR image.
- Confirm the container was recreated after pulling the image.
- Confirm the `./data:/app/data` mount is active and contains the SQLite file.
- Confirm the expected host port is exposed.
- Confirm any Synology reverse proxy route still points to the active backend.
- Call `/health` and `/weather/latest`, then inspect dashboard firmware and
  health values to confirm traffic is reaching the updated system.

## 14. GitHub Actions / CI

The implemented workflow is `.github/workflows/docker-image.yml`. It runs on
pushes to `main` that change `backend/**` or the workflow file itself.

The workflow:

1. Checks out the repository.
2. Logs in to `ghcr.io` using `GITHUB_TOKEN`.
3. Produces `latest` and commit-SHA Docker image tags.
4. Builds the image from the `./backend` context.
5. Pushes `ghcr.io/maltesch/weather-backend` to GHCR.

Changes limited to firmware do not trigger this backend image workflow.
There is no implemented CI test job or automated ESP32 OTA deployment job in
the current repository.

## 15. Important Operational Notes

- Measurement persistence depends on the `/app/data` volume mount. Recreating
  a container without that mount creates a different SQLite database.
- Starting a new backend image performs additive SQLite migrations; back up
  deployed data before an upgrade.
- API responses are `camelCase`; direct SQLite inspection exposes
  `snake_case` columns.
- `/health` verifies backend liveness, not freshness of ESP32 measurements.
- The dashboard's online indicator is based on the newest stored measurement,
  and reports stale data after 5 minutes without a reading.
- Firmware changes require a USB or OTA flash and are independent of backend
  container deployment.
- The OTA password protects OTA uploads on the local network and should not
  be committed to source control.
- If local behavior differs from the NAS, inspect the running image tag,
  container recreation, SQLite mount, exposed ports and reverse proxy first.

## 16. Planned Features

The following are possible future directions, not current functionality:

- Humidity measurement and expanded outdoor hardware packaging.
- Authenticated ingestion and dashboard access.
- Export, aggregation or longer-term time-series analysis.
- Buffered sensor uploads during network interruptions.
- Multiple sensor node support.
- Alerting or external monitoring integrations.

## 17. Development Philosophy

This project favors incremental, observable improvements over large
rewrites. The present stack is intentionally approachable: Arduino firmware,
CommonJS Node.js, SQLite, a plain JavaScript dashboard, Docker, and one
targeted deployment workflow.

Contributions should preserve that clarity: document behavior that exists,
keep changes reviewable, protect persistent measurement data, and treat
embedded, backend and deployment behavior as parts of one operational system.
