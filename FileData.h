#pragma once

#include <string>

struct FileData {
    std::string date;
    std::string time;
    int unixtime;
    int talkgroupID;
    int radioID;
    std::string duration;
    std::string filename;
    std::string filepath;
    std::string transcription;
};