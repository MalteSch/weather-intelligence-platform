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
          uptime_seconds INTEGER,
          free_heap_bytes INTEGER,
          wifi_reconnect_count INTEGER,
          upload_failure_count INTEGER,
          last_successful_upload_seconds_ago INTEGER,
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

          const existingColumnNames = new Set(columns.map((column) => column.name));
          const optionalColumns = [
            ["firmware_version", "TEXT"],
            ["uptime_seconds", "INTEGER"],
            ["free_heap_bytes", "INTEGER"],
            ["wifi_reconnect_count", "INTEGER"],
            ["upload_failure_count", "INTEGER"],
            ["last_successful_upload_seconds_ago", "INTEGER"],
          ];
          const missingColumns = optionalColumns.filter(
            ([columnName]) => !existingColumnNames.has(columnName)
          );

          if (missingColumns.length === 0) {
            resolve();
            return;
          }

          let migratedColumnCount = 0;

          for (const [columnName, columnType] of missingColumns) {
            db.run(
              `ALTER TABLE weather_measurements ADD COLUMN ${columnName} ${columnType}`,
              (error) => {
                if (error) {
                  reject(error);
                  return;
                }

                console.log(`Added ${columnName} column to weather measurements.`);
                migratedColumnCount++;

                if (migratedColumnCount === missingColumns.length) {
                  resolve();
                }
              }
            );
          }
        });
      }
    );
  });
});

module.exports = db;
