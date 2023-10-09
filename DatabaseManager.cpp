#include "DatabaseManager.h"
#include <iostream>

DatabaseManager::DatabaseManager(const std::string& dbPath) {
    int rc = sqlite3_open(dbPath.c_str(), &db);
    if(rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        // Handle error
    }
}

DatabaseManager::~DatabaseManager() {
    sqlite3_close(db);
}

void DatabaseManager::createTable() {
    const char* sql = "CREATE TABLE IF NOT EXISTS recordings (date TEXT, time TEXT, unixtime INTEGER, talkgroup_id INTEGER, talkgroup_name TEXT, radio_id INTEGER, duration TEXT, filename TEXT, filepath TEXT, transcription TEXT, v2transcription TEXT)";
    sqlite3_exec(db, sql, 0, 0, 0);
}

void DatabaseManager::insertRecording(const std::string& date, const std::string& time, int unixtime, int talkgroupID, int radioID, const std::string& duration, const std::string& filename, const std::string& filepath, const std::string& transcription) {
    std::string insertSQL = "INSERT INTO recordings (date, time, unixtime, talkgroup_id, radio_id, duration, filename, filepath, transcription) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, insertSQL.c_str(), -1, &stmt, 0);

    // Bind the variables to the SQL statement
    sqlite3_bind_text(stmt, 1, date.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, time.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, unixtime);
    sqlite3_bind_int(stmt, 4, talkgroupID);
    sqlite3_bind_int(stmt, 5, radioID);
    sqlite3_bind_text(stmt, 6, duration.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 7, filename.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 8, filepath.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 9, transcription.c_str(), -1, SQLITE_STATIC);

    // Execute the statement
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}