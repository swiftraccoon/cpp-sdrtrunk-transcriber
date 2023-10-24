// Standard Library Headers
#include <iostream>

// Project-Specific Headers
#include "DatabaseManager.h"

DatabaseManager::DatabaseManager(const std::string &dbPath)
{
    int rc = sqlite3_open(dbPath.c_str(), &db);
    if (rc)
    {
        std::cerr << "DatabaseManager.cpp Can't open database: " << sqlite3_errmsg(db) << std::endl;
        throw std::runtime_error("DatabaseManager.cpp Failed to open database");
    }
}

DatabaseManager::~DatabaseManager()
{
    sqlite3_close(db);
}

void DatabaseManager::createTable()
{
    const char *sql = "CREATE TABLE IF NOT EXISTS recordings (date TEXT, time TEXT, unixtime INTEGER, talkgroup_id INTEGER, talkgroup_name TEXT, radio_id INTEGER, duration TEXT, filename TEXT, filepath TEXT, transcription TEXT, v2transcription TEXT)";
    char *errMsg = 0;
    int rc = sqlite3_exec(db, sql, 0, 0, &errMsg);
    if (rc != SQLITE_OK)
    {
        std::cerr << "DatabaseManager.cpp SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
    }
}

void DatabaseManager::insertRecording(const std::string &date, const std::string &time, int unixtime, int talkgroupID, const std::string &talkgroupName, int radioID, const std::string &duration, const std::string &filename, const std::string &filepath, const std::string &transcription, const std::string &v2transcription)
{
    std::string insertSQL = "INSERT INTO recordings (date, time, unixtime, talkgroup_id, talkgroup_name, radio_id, duration, filename, filepath, transcription, v2transcription) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, insertSQL.c_str(), -1, &stmt, 0);
    if (rc != SQLITE_OK)
    {
        std::cerr << "DatabaseManager.cpp Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return;
    }

    // Bind the variables to the SQL statement
    sqlite3_bind_text(stmt, 1, date.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, time.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, unixtime);
    sqlite3_bind_int(stmt, 4, talkgroupID);
    sqlite3_bind_text(stmt, 5, talkgroupName.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 6, radioID);
    sqlite3_bind_text(stmt, 7, duration.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 8, filename.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 9, filepath.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 10, transcription.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 11, v2transcription.c_str(), -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE)
    {
        std::cerr << "DatabaseManager.cpp Execution failed: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(stmt);
        return;
    }

    sqlite3_finalize(stmt);
}