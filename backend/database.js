const sqlite3 = require("sqlite3").verbose();

const db = new sqlite3.Database("data/weather.db", (error) => {
  if (error) {
    console.error("Database connection failed:", error.message);
    return;
  }

  console.log("Connected to SQLite database.");
});

db.serialize(() => {
  db.run(`
    CREATE TABLE IF NOT EXISTS weather_measurements (
      id INTEGER PRIMARY KEY AUTOINCREMENT,
      temperature_celsius REAL NOT NULL,
      pressure_hpa REAL NOT NULL,
      light_level_lux REAL NOT NULL,
      wifi_rssi_dbm INTEGER NOT NULL,
      received_at TEXT NOT NULL
    )
  `);
});

module.exports = db;