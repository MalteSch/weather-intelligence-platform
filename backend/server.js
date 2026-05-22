const express = require("express");

const db = require("./database");

const app = express();
const PORT = 3000;

app.use(express.static("public"));

let latestWeatherData = null;

app.use(express.json());

app.post("/weather", (req, res) => {
  const measurement = {
    ...req.body,
    receivedAt: new Date().toISOString(),
  };

  const query = `
    INSERT INTO weather_measurements (
      temperature_celsius,
      pressure_hpa,
      light_level_lux,
      wifi_rssi_dbm,
      received_at
    )
    VALUES (?, ?, ?, ?, ?)
  `;

  db.run(
    query,
    [
      measurement.temperatureCelsius,
      measurement.pressureHpa,
      measurement.lightLevelLux,
      measurement.wifiRssiDbm,
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
  if (!latestWeatherData) {
    return res.status(404).json({
      status: "error",
      message: "No weather data received yet",
    });
  }

  res.status(200).json(latestWeatherData);
});

app.get("/weather/history", (req, res) => {
  const query = `
    SELECT
      id,
      temperature_celsius,
      pressure_hpa,
      light_level_lux,
      wifi_rssi_dbm,
      received_at
    FROM weather_measurements
    ORDER BY received_at DESC
    LIMIT 100
  `;

  db.all(query, [], (error, rows) => {
    if (error) {
      console.error("Failed to load weather history:", error.message);

      return res.status(500).json({
        status: "error",
        message: "Failed to load weather history",
      });
    }

    const history = rows.map((row) => ({
      id: row.id,
      temperatureCelsius: row.temperature_celsius,
      pressureHpa: row.pressure_hpa,
      lightLevelLux: row.light_level_lux,
      wifiRssiDbm: row.wifi_rssi_dbm,
      receivedAt: row.received_at,
    }));

    res.status(200).json(history);
  });
});

app.get("/health", (req, res) => {
  res.status(200).json({
    status: "ok",
  });
});

app.listen(PORT, "0.0.0.0", () => {
  console.log(`Weather backend listening on port ${PORT}`);
});