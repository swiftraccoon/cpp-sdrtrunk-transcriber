#pragma once

#include <cstdint>
#include <string>
#include <chrono>
#include <filesystem>

#include "DomainTypes.h"

using sdrtrunk::domain::TalkgroupId;
using sdrtrunk::domain::RadioId;
using sdrtrunk::domain::Duration;
using sdrtrunk::domain::FilePath;
using sdrtrunk::domain::Transcription;

struct FileData
{
    std::string date;
    std::string time;
    std::chrono::system_clock::time_point timestamp;
    TalkgroupId talkgroupID;
    std::string talkgroupName;
    RadioId radioID;
    Duration duration;
    FilePath filename;
    FilePath filepath;
    Transcription transcription;
    Transcription v2transcription;

    // Constructor with defaults
    FileData() : talkgroupID(0), radioID(0) {}

    // Helper to get unix timestamp for compatibility
    int64_t unixtime() const {
        return std::chrono::duration_cast<std::chrono::seconds>(
            timestamp.time_since_epoch()).count();
    }
};
