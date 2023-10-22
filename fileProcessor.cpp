// Standard Library Headers
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

// Project-Specific Headers
#include "ConfigSingleton.h"
#include "curlHelper.h"
#include "FileData.h"
#include "fileProcessor.h"
#include "transcriptionProcessor.h"


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

std::string getMP3Duration(const std::string& mp3FilePath) {
    std::stringstream cmd;
    cmd << "ffprobe -v error -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 " << mp3FilePath;

    // Execute the ffprobe command and capture the output
    FILE* pipe = popen(cmd.str().c_str(), "r");
    if (!pipe) {
        std::cerr << "fileProcessor.cpp Error executing ffprobe." << std::endl;
        return "fileProcessor.cpp Unknown"; // Return -1 to indicate an error
    }

    char buffer[128];
    std::string durationStr;
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        durationStr += buffer;
    }

    pclose(pipe);

    // Remove any trailing newline characters
    durationStr.erase(durationStr.find_last_not_of("\n\r") + 1);

    return durationStr;
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
    
    std::string durationStr = getMP3Duration(file_path);
    try {
        float duration = std::stof(durationStr);
        if (duration < 9.0) {
            std::cerr << "fileProcessor.cpp File duration is less than 9 seconds. Deleting..." << std::endl;
            std::filesystem::remove(file_path);  // Delete the file
            throw std::runtime_error("fileProcessor.cpp File duration less than 9 seconds");  // Throw an exception
        }
        } catch (const std::invalid_argument& e) {
            std::cerr << "fileProcessor.cpp Invalid argument: " << e.what() << std::endl;
            // Handle the error as appropriate for your application
        } catch (const std::out_of_range& e) {
            std::cerr << "fileProcessor.cpp Out of range: " << e.what() << std::endl;
            // Handle the error as appropriate for your application
        }

    try
    {
        std::string transcription = curl_transcribe_audio(file_path, OPENAI_API_KEY);
        std::cout << "fileProcessor.cpp transcription: " << transcription << std::endl;
        // Extract talkgroup ID
        size_t start = filename.find("TO_") + 3;
        size_t end = filename.find("_FROM_");
        std::string talkgroupID = filename.substr(start, end - start);
        std::cout << "fileProcessor.cpp talkgroupID: " << talkgroupID << std::endl;
        // Extract variables from filename
        std::string date = filename.substr(0, 8);
        std::string time = filename.substr(9, 6);
        size_t startRadioID = filename.find("_FROM_") + 6;
        size_t endRadioID = filename.find(".mp3");
        std::string radioID = filename.substr(startRadioID, endRadioID - startRadioID);
        std::cout << "fileProcessor.cpp radioID: " << radioID << std::endl;
        fileData.talkgroupID = std::stoi(talkgroupID);
        fileData.radioID = std::stoi(radioID);
        fileData.v2transcription = generateV2Transcription(transcription, fileData.talkgroupID, fileData.radioID);

        // Save transcription to TXT file
        std::string txt_filename = filename.substr(0, filename.size() - 4) + ".txt";
        std::ofstream txt_file(txt_filename);
        txt_file << fileData.v2transcription;
        txt_file.close();

        // Create subdirectory if it doesn't exist
        std::filesystem::path subDir = std::filesystem::path(directoryToMonitor) / talkgroupID;
        if (!std::filesystem::exists(subDir))
        {
            std::filesystem::create_directory(subDir);
        }
        fileData.duration = getMP3Duration(file_path);
        
        // Move MP3 and TXT files to subdirectory
        std::filesystem::rename(file_path, subDir / filename);
        std::filesystem::rename(txt_filename, subDir / txt_filename);
        
        fileData.date = date;
        fileData.time = time;
        fileData.unixtime = generateUnixTimestamp(fileData.date, fileData.time);
        fileData.filename = filename;
        fileData.filepath = file_path;
        fileData.transcription = transcription;
        
        return fileData;
    }
    catch (const std::exception &e)
    {
        std::cerr << "fileProcessor.cpp Error: " << e.what() << std::endl;
        return FileData();
    }
}