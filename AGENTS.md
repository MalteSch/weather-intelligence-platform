# Weather Intelligence Platform - Agent Guidelines

## Project context

This is a personal ESP32-based weather station project.

Current architecture:

- ESP32 sensor node sends weather measurements via HTTP.
- Node/Express backend receives measurements.
- SQLite stores measurements persistently.
- Static dashboard is served from `backend/public/index.html`.
- Docker image is built by GitHub Actions and published to GHCR.
- Synology runs the container via Docker Compose.

## Important conventions

- Backend code is CommonJS JavaScript for now.
- Do not migrate to TypeScript unless explicitly requested.
- Do not introduce frontend frameworks.
- Keep the dashboard as plain HTML/CSS/JavaScript for now.
- Do not remove SQLite persistence.
- Do not replace the current deploy workflow.
- Prefer small, reviewable changes.
- Avoid broad refactors unless requested.

## Firmware Versioning

Any change affecting ESP32 firmware behavior must increment `FIRMWARE_VERSION`.

This includes:
- sensor logic changes
- payload changes
- OTA behavior
- connectivity/recovery logic
- telemetry fields
- LED/status behavior
- timing changes

The firmware version is used to verify OTA deployments and runtime state in the dashboard.

Use semantic-ish incremental versions such as:

- 0.1.0
- 0.1.1
- 0.2.0
- 0.2.1-health
- 0.3.0-ota

Avoid leaving outdated version strings after firmware modifications.

## Backend

Main files:

- `backend/server.js`
- `backend/database.js`
- `backend/public/index.html`

Expected API endpoints:

- `POST /weather`
- `GET /weather/latest`
- `GET /weather/history`
- `GET /health`

Database columns:

- `id`
- `temperature_celsius`
- `pressure_hpa`
- `light_level_lux`
- `wifi_rssi_dbm`
- `received_at`

API JSON format should use camelCase:

- `temperatureCelsius`
- `pressureHpa`
- `lightLevelLux`
- `wifiRssiDbm`
- `receivedAt`

## Docker / deployment

- Docker image must build in GitHub Actions.
- The backend image uses Node 20.
- `sqlite3` must be built from source in Docker to avoid glibc binary issues.
- SQLite data must remain persistent via `/app/data`.
- Do not copy local `node_modules` into Docker images.

## Runtime environment

The backend is deployed on a Synology NAS using Docker Compose.

Important:
- Local development and deployed runtime may differ.
- A working local API does NOT guarantee the Synology deployment uses the latest image.
- When debugging deployment issues, always verify:
  - running container image tag
  - active container recreation
  - mounted SQLite volume
  - exposed ports
  - reverse proxy configuration

## Current debugging priorities

When fixing dashboard/API issues:

1. Verify `/weather/latest` exists and reads latest row from SQLite.
2. Verify `/weather/history` returns camelCase JSON.
3. Verify dashboard calls the correct endpoints.
4. Verify Chart.js time scale uses real timestamps, not category labels.
5. Keep deploy path working through GitHub Actions and GHCR.

## Working style

Before editing:
- inspect the relevant files
- summarize the planned changes briefly

After editing:
- show the exact files changed
- summarize the behavior changed
- suggest the git commit message