const sqlite3 = require("sqlite3").verbose();

const db = new sqlite3.Database("data/weather.db", (error) => {
  if (error) {
    console.error("Database connection failed:", error.message);
    return;
  }

  console.log("Connected to SQLite database.");
});

db.ready = new Promise((resolve, reject) => {
  db.serialize(() => {
    db.run(
      `
        CREATE TABLE IF NOT EXISTS weather_measurements (
          id INTEGER PRIMARY KEY AUTOINCREMENT,
          temperature_celsius REAL NOT NULL,
          pressure_hpa REAL NOT NULL,
          light_level_lux REAL NOT NULL,
          wifi_rssi_dbm INTEGER NOT NULL,
          firmware_version TEXT,
          received_at TEXT NOT NULL
        )
      `,
      (error) => {
        if (error) {
          reject(error);
          return;
        }

        db.all("PRAGMA table_info(weather_measurements)", [], (error, columns) => {
          if (error) {
            reject(error);
            return;
          }

          const hasFirmwareVersion = columns.some(
            (column) => column.name === "firmware_version"
          );

          if (hasFirmwareVersion) {
            resolve();
            return;
          }

          db.run(
            "ALTER TABLE weather_measurements ADD COLUMN firmware_version TEXT",
            (error) => {
              if (error) {
                reject(error);
                return;
              }

              console.log("Added firmware_version column to weather measurements.");
              resolve();
            }
          );
        });
      }
    );
  });
});

module.exports = db;
