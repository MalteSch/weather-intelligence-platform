const express = require("express");

const db = require("./database");

const app = express();
const PORT = 3000;

app.use(express.static("public"));

let latestWeatherData = null;

app.use(express.json());

function validateWeatherMeasurement(payload) {
  const errors = [];
  const optionalNonNegativeNumbers = [
    "uptimeSeconds",
    "freeHeapBytes",
    "wifiReconnectCount",
    "uploadFailureCount",
    "lastSuccessfulUploadSecondsAgo",
  ];

  if (typeof payload.temperatureCelsius !== "number") {
    errors.push("temperatureCelsius must be a number");
  }

  if (typeof payload.pressureHpa !== "number") {
    errors.push("pressureHpa must be a number");
  }

  if (
    payload.humidityPercent !== undefined &&
    payload.humidityPercent !== null &&
    (typeof payload.humidityPercent !== "number" ||
      !Number.isFinite(payload.humidityPercent) ||
      payload.humidityPercent < 0 ||
      payload.humidityPercent > 100)
  ) {
    errors.push("humidityPercent must be a number between 0 and 100");
  }

  if (typeof payload.lightLevelLux !== "number") {
    errors.push("lightLevelLux must be a number");
  }

  if (typeof payload.wifiRssiDbm !== "number") {
    errors.push("wifiRssiDbm must be a number");
  }

  if (
    payload.firmwareVersion !== undefined &&
    payload.firmwareVersion !== null &&
    typeof payload.firmwareVersion !== "string"
  ) {
    errors.push("firmwareVersion must be a string");
  }

  for (const fieldName of optionalNonNegativeNumbers) {
    const value = payload[fieldName];

    if (
      value !== undefined &&
      value !== null &&
      (typeof value !== "number" || !Number.isFinite(value) || value < 0)
    ) {
      errors.push(`${fieldName} must be a non-negative number`);
    }
  }

  return errors;
}

function mapWeatherRow(row) {
  return {
    id: row.id,
    temperatureCelsius: row.temperature_celsius,
    pressureHpa: row.pressure_hpa,
    humidityPercent: row.humidity_percent,
    lightLevelLux: row.light_level_lux,
    wifiRssiDbm: row.wifi_rssi_dbm,
    firmwareVersion: row.firmware_version,
    uptimeSeconds: row.uptime_seconds,
    freeHeapBytes: row.free_heap_bytes,
    wifiReconnectCount: row.wifi_reconnect_count,
    uploadFailureCount: row.upload_failure_count,
    lastSuccessfulUploadSecondsAgo: row.last_successful_upload_seconds_ago,
    receivedAt: row.received_at,
  };
}

const DEVICE_LOG_LEVELS = new Set(["info", "warn", "error"]);

function validateDeviceLog(payload) {
  const errors = [];

  if (!payload || typeof payload !== "object" || Array.isArray(payload)) {
    return ["request body must be a JSON object"];
  }

  if (!DEVICE_LOG_LEVELS.has(payload.level)) {
    errors.push('level must be one of "info", "warn", or "error"');
  }

  if (
    typeof payload.event !== "string" ||
    payload.event.trim().length === 0 ||
    payload.event.length > 80
  ) {
    errors.push("event must be a non-empty string of at most 80 characters");
  }

  if (
    payload.message !== undefined &&
    payload.message !== null &&
    (typeof payload.message !== "string" || payload.message.length > 500)
  ) {
    errors.push("message must be a string of at most 500 characters");
  }

  if (
    payload.firmwareVersion !== undefined &&
    payload.firmwareVersion !== null &&
    (typeof payload.firmwareVersion !== "string" ||
      payload.firmwareVersion.length > 80)
  ) {
    errors.push("firmwareVersion must be a string of at most 80 characters");
  }

  if (
    payload.uptimeSeconds !== undefined &&
    payload.uptimeSeconds !== null &&
    (typeof payload.uptimeSeconds !== "number" ||
      !Number.isFinite(payload.uptimeSeconds) ||
      payload.uptimeSeconds < 0)
  ) {
    errors.push("uptimeSeconds must be a non-negative number");
  }

  return errors;
}

function mapDeviceLogRow(row) {
  return {
    id: row.id,
    level: row.level,
    event: row.event,
    message: row.message,
    firmwareVersion: row.firmware_version,
    uptimeSeconds: row.uptime_seconds,
    receivedAt: row.received_at,
  };
}

app.post("/weather", (req, res) => {
  const validationErrors = validateWeatherMeasurement(req.body);

  if (validationErrors.length > 0) {
    console.error("Invalid weather payload:", validationErrors);

    return res.status(400).json({
      status: "error",
      errors: validationErrors,
    });
  }

  const measurement = {
    ...req.body,
    receivedAt: new Date().toISOString(),
  };

  const query = `
    INSERT INTO weather_measurements (
      temperature_celsius,
      pressure_hpa,
      humidity_percent,
      light_level_lux,
      wifi_rssi_dbm,
      firmware_version,
      uptime_seconds,
      free_heap_bytes,
      wifi_reconnect_count,
      upload_failure_count,
      last_successful_upload_seconds_ago,
      received_at
    )
    VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
  `;

  db.run(
    query,
    [
      measurement.temperatureCelsius,
      measurement.pressureHpa,
      measurement.humidityPercent ?? null,
      measurement.lightLevelLux,
      measurement.wifiRssiDbm,
      measurement.firmwareVersion ?? null,
      measurement.uptimeSeconds ?? null,
      measurement.freeHeapBytes ?? null,
      measurement.wifiReconnectCount ?? null,
      measurement.uploadFailureCount ?? null,
      measurement.lastSuccessfulUploadSecondsAgo ?? null,
      measurement.receivedAt,
    ],
    function (error) {
      if (error) {
        console.error("Failed to store weather data:", error.message);

        return res.status(500).json({
          status: "error",
          message: "Failed to store weather data",
        });
      }

      latestWeatherData = {
        id: this.lastID,
        ...measurement,
      };

      console.log("Stored weather data:");
      console.log(latestWeatherData);

      res.status(201).json({
        status: "ok",
        message: "Weather data stored",
        id: this.lastID,
      });
    }
  );
});

app.get("/weather/latest", (req, res) => {
  const query = `
    SELECT
      id,
      temperature_celsius,
      pressure_hpa,
      humidity_percent,
      light_level_lux,
      wifi_rssi_dbm,
      firmware_version,
      uptime_seconds,
      free_heap_bytes,
      wifi_reconnect_count,
      upload_failure_count,
      last_successful_upload_seconds_ago,
      received_at
    FROM weather_measurements
    ORDER BY received_at DESC
    LIMIT 1
  `;

  db.get(query, [], (error, row) => {
    if (error) {
      console.error("Failed to load latest weather data:", error.message);

      return res.status(500).json({
        status: "error",
        message: "Failed to load latest weather data",
      });
    }

    if (!row) {
      return res.status(404).json({
        status: "error",
        message: "No weather data received yet",
      });
    }

    res.status(200).json(mapWeatherRow(row));
  });
});

app.get("/weather/history", (req, res) => {
  const from = req.query.from;
  const to = req.query.to;

  let query = `
    SELECT
      id,
      temperature_celsius,
      pressure_hpa,
      humidity_percent,
      light_level_lux,
      wifi_rssi_dbm,
      firmware_version,
      uptime_seconds,
      free_heap_bytes,
      wifi_reconnect_count,
      upload_failure_count,
      last_successful_upload_seconds_ago,
      received_at
    FROM weather_measurements
  `;

  const queryParams = [];

  if (from && to) {
    query += `
      WHERE received_at BETWEEN ? AND ?
      ORDER BY received_at ASC
    `;

    queryParams.push(from, to);
  } else {
    query = `
      SELECT
        id,
        temperature_celsius,
        pressure_hpa,
        humidity_percent,
        light_level_lux,
        wifi_rssi_dbm,
        firmware_version,
        uptime_seconds,
        free_heap_bytes,
        wifi_reconnect_count,
        upload_failure_count,
        last_successful_upload_seconds_ago,
        received_at
      FROM (
        SELECT
          id,
          temperature_celsius,
          pressure_hpa,
          humidity_percent,
          light_level_lux,
          wifi_rssi_dbm,
          firmware_version,
          uptime_seconds,
          free_heap_bytes,
          wifi_reconnect_count,
          upload_failure_count,
          last_successful_upload_seconds_ago,
          received_at
        FROM weather_measurements
        ORDER BY received_at DESC
        LIMIT 2000
      )
      ORDER BY received_at ASC
    `;
  }

  db.all(query, queryParams, (error, rows) => {
    if (error) {
      console.error("Failed to load weather history:", error.message);

      return res.status(500).json({
        status: "error",
        message: "Failed to load weather history",
      });
    }

    const history = rows.map(mapWeatherRow);

    res.status(200).json(history);
  });
});

app.post("/device/logs", (req, res) => {
  const validationErrors = validateDeviceLog(req.body);

  if (validationErrors.length > 0) {
    console.error("Invalid device log payload:", validationErrors);

    return res.status(400).json({
      status: "error",
      errors: validationErrors,
    });
  }

  const deviceLog = {
    level: req.body.level,
    event: req.body.event.trim(),
    message: req.body.message ?? null,
    firmwareVersion: req.body.firmwareVersion ?? null,
    uptimeSeconds: req.body.uptimeSeconds ?? null,
    receivedAt: new Date().toISOString(),
  };

  const query = `
    INSERT INTO device_logs (
      level,
      event,
      message,
      firmware_version,
      uptime_seconds,
      received_at
    )
    VALUES (?, ?, ?, ?, ?, ?)
  `;

  db.run(
    query,
    [
      deviceLog.level,
      deviceLog.event,
      deviceLog.message,
      deviceLog.firmwareVersion,
      deviceLog.uptimeSeconds,
      deviceLog.receivedAt,
    ],
    function (error) {
      if (error) {
        console.error("Failed to store device log:", error.message);

        return res.status(500).json({
          status: "error",
          message: "Failed to store device log",
        });
      }

      res.status(201).json({
        status: "ok",
        message: "Device log stored",
        id: this.lastID,
      });
    }
  );
});

app.get("/device/logs", (req, res) => {
  const requestedLimit = Number.parseInt(req.query.limit, 10);
  const limit = Number.isInteger(requestedLimit) && requestedLimit > 0
    ? Math.min(requestedLimit, 100)
    : 10;
  const query = `
    SELECT
      id,
      level,
      event,
      message,
      firmware_version,
      uptime_seconds,
      received_at
    FROM device_logs
    ORDER BY received_at DESC, id DESC
    LIMIT ?
  `;

  db.all(query, [limit], (error, rows) => {
    if (error) {
      console.error("Failed to load device logs:", error.message);

      return res.status(500).json({
        status: "error",
        message: "Failed to load device logs",
      });
    }

    res.status(200).json(rows.map(mapDeviceLogRow));
  });
});

app.get("/health", (req, res) => {
  res.status(200).json({
    status: "ok",
  });
});

db.ready
  .then(() => {
    app.listen(PORT, "0.0.0.0", () => {
      console.log(`Weather backend listening on port ${PORT}`);
    });
  })
  .catch((error) => {
    console.error("Failed to initialize SQLite database:", error.message);
    process.exitCode = 1;
  });
