#include "fileProcessor.h"
#include "curlHelper.h"
#include <fstream>
#include <iostream>
#include <chrono>
#include <ctime>
#include <sstream>
#include <taglib/fileref.h>
#include <taglib/mpegfile.h>

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

std::string getMP3Duration(const std::string& filePath) {
    TagLib::FileRef file(filePath.c_str());
    if (!file.isNull() && file.audioProperties()) {
        int seconds = file.audioProperties()->length() % 60;
        int minutes = (file.audioProperties()->length() - seconds) / 60;
        return std::to_string(minutes) + ":" + std::to_string(seconds);
    }
    return "Unknown";
}

int generateUnixTimestamp(const std::string& date, const std::string& time) {
    std::tm tm = {};
    std::string dateTime = date + time;  // Combine date and time
    std::istringstream ss(dateTime);
    ss >> std::get_time(&tm, "%Y%m%d%H%M%S");  // Parse the combined string
    auto tp = std::chrono::system_clock::from_time_t(std::mktime(&tm));
    return std::chrono::duration_cast<std::chrono::seconds>(tp.time_since_epoch()).count();
}

void processFile(const std::filesystem::path &filePath, const std::string &directoryToMonitor)
{
    std::string file_path = filePath.string();
    std::string filename = filePath.filename().string();

    try
    {
        std::string transcription = curl_transcribe_audio(file_path);

        // Save transcription to TXT file
        std::string txt_filename = filename.substr(0, filename.size() - 4) + ".txt";
        std::ofstream txt_file(txt_filename);
        txt_file << transcription;
        txt_file.close();

        // Extract talkgroup ID
        size_t start = filename.find("TO_") + 3;
        size_t end = filename.find("_FROM_");
        std::string talkgroupID = filename.substr(start, end - start);

        // Create subdirectory if it doesn't exist
        std::filesystem::path subDir = directoryToMonitor / talkgroupID;
        if (!std::filesystem::exists(subDir))
        {
            std::filesystem::create_directory(subDir);
        }

        // Move MP3 and TXT files to subdirectory
        std::filesystem::rename(file_path, subDir / filename);
        std::filesystem::rename(txt_filename, subDir / txt_filename);

        FileData fileData;

        // Extract variables from filename
        std::string date = filename.substr(0, 8);
        std::string time = filename.substr(9, 6);
        size_t startTalkgroup = filename.find("TO_") + 3;
        size_t endTalkgroup = filename.find("_FROM_");
        std::string talkgroupID = filename.substr(startTalkgroup, endTalkgroup - startTalkgroup);
        std::string radioID = filename.substr(endTalkgroup + 6, filename.find(".mp3") - (endTalkgroup + 6));
        
        fileData.date = date;
        fileData.time = time;
        fileData.unixtime = generateUnixTimestamp(fileData.date, fileData.time);
        fileData.talkgroupID = std::stoi(talkgroupID);
        fileData.radioID = std::stoi(radioID);
        fileData.duration = getMP3Duration(filePath.string());
        fileData.filename = filename;
        fileData.filepath = file_path;
        fileData.transcription = transcription;

        return fileData;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}