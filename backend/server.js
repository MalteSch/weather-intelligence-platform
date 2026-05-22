const express = require("express");

const app = express();
const PORT = 3000;

app.use(express.json());

app.post("/weather", (req, res) => {
  console.log("Received weather data:");
  console.log(req.body);

  res.status(200).json({
    status: "ok",
    message: "Weather data received",
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