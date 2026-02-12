// Standard Library Headers
#include <iostream>

// Project-Specific Headers
#include "../include/DatabaseManager.h"
#include "../include/debugUtils.h"

DatabaseManager::DatabaseManager(const std::string &dbPath)
{
    int rc = sqlite3_open(dbPath.c_str(), &db);
    if (rc)
    {
        std::cerr << "[" << getCurrentTime() << "] "
                  << "DatabaseManager.cpp DatabaseManager Can't open database: " << sqlite3_errmsg(db) << std::endl;
        throw std::runtime_error("[" + getCurrentTime() + "]" + "DatabaseManager.cpp DatabaseManager Failed to open database");
    }

    // Enable WAL mode for concurrent reads during writes
    sqlite3_exec(db, "PRAGMA journal_mode=WAL;", 0, 0, 0);
    sqlite3_exec(db, "PRAGMA synchronous=NORMAL;", 0, 0, 0);
    sqlite3_exec(db, "PRAGMA busy_timeout=5000;", 0, 0, 0);
}

DatabaseManager::~DatabaseManager()
{
    sqlite3_close(db);
}

void DatabaseManager::migrateSchema()
{
    // Check if the 'id' column exists (new schema has it, old schema does not)
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, "PRAGMA table_info(recordings);", -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return;

    bool hasIdColumn = false;
    bool hasDurationAsReal = false;
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        std::string colName = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
        std::string colType = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
        if (colName == "id")
            hasIdColumn = true;
        if (colName == "duration" && colType == "REAL")
            hasDurationAsReal = true;
    }
    sqlite3_finalize(stmt);

    if (hasIdColumn && hasDurationAsReal)
        return; // Already on new schema

    // Migrate: rename old table, create new, copy data, drop old
    char *errMsg = 0;
    const char *migrationSQL = R"(
        BEGIN TRANSACTION;
        ALTER TABLE recordings RENAME TO recordings_old;
        CREATE TABLE IF NOT EXISTS recordings (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            date TEXT NOT NULL,
            time TEXT NOT NULL,
            unixtime INTEGER NOT NULL,
            talkgroup_id INTEGER NOT NULL,
            talkgroup_name TEXT NOT NULL DEFAULT '',
            radio_id INTEGER NOT NULL,
            duration REAL NOT NULL DEFAULT 0.0,
            filename TEXT NOT NULL UNIQUE,
            filepath TEXT NOT NULL,
            transcription TEXT NOT NULL DEFAULT '',
            v2transcription TEXT NOT NULL DEFAULT ''
        );
        INSERT OR IGNORE INTO recordings (date, time, unixtime, talkgroup_id, talkgroup_name, radio_id, duration, filename, filepath, transcription, v2transcription)
            SELECT date, time, unixtime, talkgroup_id, talkgroup_name, radio_id,
                   CAST(duration AS REAL), filename, filepath, transcription, v2transcription
            FROM recordings_old;
        DROP TABLE recordings_old;
        COMMIT;
    )";
    rc = sqlite3_exec(db, migrationSQL, 0, 0, &errMsg);
    if (rc != SQLITE_OK)
    {
        std::cerr << "[" << getCurrentTime() << "] "
                  << "DatabaseManager.cpp migrateSchema Migration error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        // Attempt rollback in case of partial failure
        sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
    }
}

void DatabaseManager::createTable()
{
    const char *sql = R"(
        CREATE TABLE IF NOT EXISTS recordings (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            date TEXT NOT NULL,
            time TEXT NOT NULL,
            unixtime INTEGER NOT NULL,
            talkgroup_id INTEGER NOT NULL,
            talkgroup_name TEXT NOT NULL DEFAULT '',
            radio_id INTEGER NOT NULL,
            duration REAL NOT NULL DEFAULT 0.0,
            filename TEXT NOT NULL UNIQUE,
            filepath TEXT NOT NULL,
            transcription TEXT NOT NULL DEFAULT '',
            v2transcription TEXT NOT NULL DEFAULT ''
        )
    )";
    char *errMsg = 0;
    int rc = sqlite3_exec(db, sql, 0, 0, &errMsg);
    if (rc != SQLITE_OK)
    {
        std::cerr << "[" << getCurrentTime() << "] "
                  << "DatabaseManager.cpp createTable SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return;
    }

    // Create indexes for common queries
    sqlite3_exec(db, "CREATE INDEX IF NOT EXISTS idx_recordings_talkgroup_id ON recordings(talkgroup_id);", 0, 0, 0);
    sqlite3_exec(db, "CREATE INDEX IF NOT EXISTS idx_recordings_unixtime ON recordings(unixtime);", 0, 0, 0);
    sqlite3_exec(db, "CREATE INDEX IF NOT EXISTS idx_recordings_filename ON recordings(filename);", 0, 0, 0);

    // Migrate old schema if needed
    migrateSchema();
}

void DatabaseManager::insertRecording(const std::string &date, const std::string &time, int64_t unixtime, int talkgroupID, const std::string &talkgroupName, int radioID, double duration, const std::string &filename, const std::string &filepath, const std::string &transcription, const std::string &v2transcription)
{
    std::lock_guard<std::mutex> lock(writeMutex_);

    std::string insertSQL = "INSERT OR IGNORE INTO recordings (date, time, unixtime, talkgroup_id, talkgroup_name, radio_id, duration, filename, filepath, transcription, v2transcription) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, insertSQL.c_str(), -1, &stmt, 0);
    if (rc != SQLITE_OK)
    {
        std::cerr << "[" << getCurrentTime() << "] "
                  << "DatabaseManager.cpp insertRecording Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return;
    }

    // Bind the variables to the SQL statement
    sqlite3_bind_text(stmt, 1, date.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, time.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 3, unixtime);
    sqlite3_bind_int(stmt, 4, talkgroupID);
    sqlite3_bind_text(stmt, 5, talkgroupName.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 6, radioID);
    sqlite3_bind_double(stmt, 7, duration);
    sqlite3_bind_text(stmt, 8, filename.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 9, filepath.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 10, transcription.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 11, v2transcription.c_str(), -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE)
    {
        std::cerr << "[" << getCurrentTime() << "] "
                  << "DatabaseManager.cpp insertRecording Execution failed: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(stmt);
        return;
    }

    sqlite3_finalize(stmt);
}
