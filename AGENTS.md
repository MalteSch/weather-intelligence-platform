# Weather Intelligence Platform - Agent Guidelines

## Project Context

This is a personal ESP32-based weather station project focused on:

* reliable long-term operation
* OTA firmware updates
* weather data collection
* historical analysis
* lightweight observability
* simple deployment and maintenance

Current architecture:

* ESP32 sensor node sends measurements via HTTP.
* ESP32 supports OTA updates.
* Node/Express backend receives measurements and device logs.
* SQLite stores measurements and device diagnostics.
* Static dashboard is served from `backend/public/index.html`.
* Docker image is built by GitHub Actions and published to GHCR.
* Synology NAS runs the backend via Docker Compose.

---

## Project Philosophy

Primary goal:

* Weather data first.
* Diagnostics second.

The dashboard should primarily help observe weather conditions and trends.

Diagnostics are important but should remain visually secondary and collapsible where practical.

Avoid turning the dashboard into a debugging console unless explicitly requested.

---

## Technology Constraints

* Backend remains CommonJS JavaScript.
* Do not migrate to TypeScript unless explicitly requested.
* Do not introduce frontend frameworks.
* Keep the dashboard as plain HTML/CSS/JavaScript.
* Keep SQLite persistence.
* Keep Chart.js.
* Do not replace the current deployment workflow.
* Prefer small, reviewable changes.
* Avoid large refactors unless requested.

---

## Firmware Versioning

Any change affecting ESP32 firmware behavior MUST increment `FIRMWARE_VERSION`.

This includes:

* sensor logic changes
* payload changes
* OTA behavior
* connectivity logic
* recovery/watchdog behavior
* telemetry fields
* device logging
* timing changes
* LED behavior
* display/OLED behavior
* I2C behavior

Examples:

* 0.1.0
* 0.1.1
* 0.2.0-bme280
* 0.2.1-device-logs
* 0.3.0-oled

Never modify ESP firmware without updating the firmware version.

---

## OTA Workflow

Codex cannot perform OTA uploads.

After firmware changes:

1. Explain the changes.
2. Ask the user to perform OTA upload manually through Arduino IDE.
3. Explain how runtime validation should be performed.
4. Use device logs and dashboard status to verify deployment.

Do not claim OTA functionality has been verified until the user performs an OTA upload.

---

## ESP32 Hardware

Current sensor configuration:

* ESP32-C3-WROOM-02U
* BME280

  * temperature
  * pressure
  * humidity
* BH1750

  * ambient light level

Communication:

* I2C bus shared between sensors
* SDA = GPIO21
* SCL = GPIO22

Current telemetry includes:

* temperature
* pressure
* humidity
* light level
* WiFi RSSI
* firmware version
* uptime
* heap usage
* reconnect count
* upload failure count

---

## Backend

Main files:

* `backend/server.js`
* `backend/database.js`
* `backend/public/index.html`

Expected endpoints:

### Weather

* `POST /weather`
* `GET /weather/latest`
* `GET /weather/history`

### Health

* `GET /health`

### Device Diagnostics

* `POST /device/logs`
* `GET /device/logs`
* `GET /device/status`

---

## Database

SQLite remains the source of truth.

Weather measurements and device diagnostics are stored separately.

Avoid destructive migrations.

Prefer:

* `CREATE TABLE IF NOT EXISTS`
* additive schema changes
* safe `ALTER TABLE` migrations

---

## API Conventions

Database:

* snake_case

API JSON:

* camelCase

Examples:

Database:

* temperature_celsius
* pressure_hpa
* humidity_percent
* light_level_lux
* wifi_rssi_dbm
* received_at

API:

* temperatureCelsius
* pressureHpa
* humidityPercent
* lightLevelLux
* wifiRssiDbm
* receivedAt

Maintain this convention consistently.

---

## Dashboard Guidelines

Dashboard priorities:

1. Current weather conditions
2. Historical trends
3. Observation controls
4. Device status
5. Diagnostics

Keep charts and weather values as the primary visual focus.

Diagnostics should:

* be available
* be useful
* not dominate the layout

Avoid duplicated information.

Examples:

Bad:

* displaying selected time range multiple times

Good:

* one clear source of truth for selected range

---

## Chart Guidelines

* Use real timestamps.
* Never use category labels for time series.
* Preserve zoom functionality.
* Preserve overlay comparison functionality.
* Preserve auto-refresh behavior.
* Preserve Min / Avg / Max calculations.

Charts should remain synchronized to the same selected time range.

---

## Device Logging

Device logs are intended for:

* OTA troubleshooting
* sensor initialization issues
* connectivity issues
* recovery watchdog events

Avoid excessive log spam.

Prefer:

* state transitions
* errors
* warnings
* important milestones

Avoid logging every successful measurement.

---

## Docker / Deployment

Requirements:

* Docker image builds in GitHub Actions.
* Backend image uses Node 20.
* SQLite data persists via `/app/data`.
* Do not copy local `node_modules`.
* `sqlite3` must build from source inside Docker.

Deployment target:

* Synology NAS
* Docker Compose
* GHCR image source

---

## Deployment Reality

Always remember:

A working local environment does NOT prove the deployed environment is updated.

When debugging deployment issues, verify:

* GitHub Action completed successfully
* image exists in GHCR
* Synology pulled the new image
* container was recreated
* correct image tag is running
* SQLite volume is mounted
* ports are exposed correctly
* reverse proxy is correct

---

## Working Style

Before editing:

1. Inspect relevant files.
2. Summarize planned changes.
3. Identify affected files.

After editing:

1. List changed files.
2. Summarize behavior changes.
3. Explain validation steps.
4. Suggest git commands.

For firmware changes additionally:

1. Confirm `FIRMWARE_VERSION` was incremented.
2. Ask the user to perform OTA upload.
3. Explain how to verify the new firmware through device status and logs.
