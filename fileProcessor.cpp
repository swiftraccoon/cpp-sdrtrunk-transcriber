#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <fstream>
#include <iostream>
#include <chrono>
#include <ctime>
#include <sstream>
#include <cstdio>
#include <memory>
#include <string>
#include <array>
#include <stdexcept>
#include <vector>
#include <algorithm>
#include "fileProcessor.h"
#include "curlHelper.h"
#include "FileData.h"

void find_and_move_mp3_without_txt(const std::string &directoryToMonitor) {
    std::vector<std::string> mp3_files;
    std::vector<std::string> txt_files;

    for (const auto& file : std::filesystem::directory_iterator(directoryToMonitor)) {
        if (file.path().extension() == ".mp3") {
            mp3_files.push_back(file.path().filename().string());
        }
        if (file.path().extension() == ".txt") {
            txt_files.push_back(file.path().stem().string());
        }
    }

    for (const auto& mp3 : mp3_files) {
        std::string mp3_base = mp3.substr(0, mp3.size() - 4);  // Remove ".mp3" extension
        if (std::find(txt_files.begin(), txt_files.end(), mp3_base) == txt_files.end()) {
            std::filesystem::path src_path = std::filesystem::path(directoryToMonitor) / mp3;
            std::filesystem::path dest_path = std::filesystem::path(directoryToMonitor) / mp3;
            std::filesystem::rename(src_path, dest_path);  // Move the file
        }
    }
}

double getMP3Duration(const std::string& file_path) {
    // Open the file
    AVFormatContext *format_ctx = nullptr;
    if (avformat_open_input(&format_ctx, file_path.c_str(), nullptr, nullptr) != 0) {
        std::cerr << "Could not open file: " << file_path << std::endl;
        return -1.0;
    }

    // Retrieve stream information
    if (avformat_find_stream_info(format_ctx, nullptr) < 0) {
        std::cerr << "Could not find stream information." << std::endl;
        return -1.0;
    }

    // Get the duration
    int64_t duration = format_ctx->duration;
    double duration_in_seconds = static_cast<double>(duration) / AV_TIME_BASE;

    // Close the file
    avformat_close_input(&format_ctx);

    return duration_in_seconds;
}

int generateUnixTimestamp(const std::string& date, const std::string& time) {
    std::tm tm = {};
    std::string dateTime = date + time;  // Combine date and time
    std::istringstream ss(dateTime);
    ss >> std::get_time(&tm, "%Y%m%d%H%M%S");  // Parse the combined string
    auto tp = std::chrono::system_clock::from_time_t(std::mktime(&tm));
    return std::chrono::duration_cast<std::chrono::seconds>(tp.time_since_epoch()).count();
}

FileData processFile(const std::filesystem::path& path, const std::string& directoryToMonitor, const std::string& OPENAI_API_KEY) {
    FileData fileData;
    std::string file_path = path.string();  // Changed from filePath to path
    std::string filename = path.filename().string();  // Changed from filePath to path

    try
    {
        std::string transcription = curl_transcribe_audio(file_path, OPENAI_API_KEY);

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
        std::filesystem::path subDir = std::filesystem::path(directoryToMonitor) / talkgroupID;
        if (!std::filesystem::exists(subDir))
        {
            std::filesystem::create_directory(subDir);
        }

        // Move MP3 and TXT files to subdirectory
        std::filesystem::rename(file_path, subDir / filename);
        std::filesystem::rename(txt_filename, subDir / txt_filename);

        // Extract variables from filename
        std::string date = filename.substr(0, 8);
        std::string time = filename.substr(9, 6);
        size_t startRadioID = filename.find("_FROM_") + 6;
        size_t endRadioID = filename.find(".mp3");
        std::string radioID = filename.substr(startRadioID, endRadioID - startRadioID);
        
        fileData.date = date;
        fileData.time = time;
        fileData.unixtime = generateUnixTimestamp(fileData.date, fileData.time);
        fileData.talkgroupID = std::stoi(talkgroupID);
        fileData.radioID = std::stoi(radioID);
        fileData.duration = getMP3Duration(file_path);
        fileData.filename = filename;
        fileData.filepath = file_path;
        fileData.transcription = transcription;

        return fileData;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return FileData();
    }
}