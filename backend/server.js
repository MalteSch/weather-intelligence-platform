const express = require("express");

const app = express();
const PORT = 3000;

let latestWeatherData = null;

app.use(express.json());

app.post("/weather", (req, res) => {
  latestWeatherData = {
    ...req.body,
    receivedAt: new Date().toISOString(),
  };

  console.log("Received weather data:");
  console.log(latestWeatherData);

  res.status(200).json({
    status: "ok",
    message: "Weather data received",
  });
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

app.get("/health", (req, res) => {
  res.status(200).json({
    status: "ok",
  });
});

app.listen(PORT, "0.0.0.0", () => {
  console.log(`Weather backend listening on port ${PORT}`);
});