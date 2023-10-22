#pragma once

// Standard Library Headers
#include <string>

struct FileData {
    std::string date;
    std::string time;
    int unixtime;
    int talkgroupID;
    std::string talkgroupName;
    int radioID;
    std::string duration;
    std::string filename;
    std::string filepath;
    std::string transcription;
    std::string v2transcription;
};