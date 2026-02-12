#pragma once

// Standard Library Headers
#include <cstdint>
#include <mutex>
#include <string>

// Project-Specific Headers
#include <sqlite3.h>

class DatabaseManager
{
public:
    DatabaseManager(const std::string &dbPath);
    ~DatabaseManager();
    void createTable();
    void insertRecording(const std::string &date, const std::string &time, int64_t unixtime, int talkgroupID, const std::string &talkgroupName, int radioID, double duration, const std::string &filename, const std::string &filepath, const std::string &transcription, const std::string &v2transcription);

private:
    void migrateSchema();
    sqlite3 *db;
    std::mutex writeMutex_;
};
