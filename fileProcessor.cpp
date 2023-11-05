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
#include <thread>
#include <unistd.h>
#include <vector>
#include <sys/types.h>
#include <sys/wait.h>

// Project-Specific Headers
#include "ConfigSingleton.h"
#include "curlHelper.h"
#include "debugUtils.h"
#include "FileData.h"
#include "fileProcessor.h"
#include "transcriptionProcessor.h"

bool isFileBeingWrittenTo(const std::string &filePath)
{
    std::filesystem::path path(filePath);
    auto size1 = std::filesystem::file_size(path);
    std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Wait for 500 ms
    auto size2 = std::filesystem::file_size(path);
    return size1 != size2;
}

bool isFileLocked(const std::string &filePath)
{
    return std::filesystem::exists(filePath + ".lock");
}

void find_and_move_mp3_without_txt(const std::string &directoryToMonitor)
{
    std::vector<std::string> mp3_files;
    std::vector<std::string> txt_files;

    for (const auto &file : std::filesystem::directory_iterator(directoryToMonitor))
    {
        if (file.path().extension() == ".mp3")
        {
            mp3_files.push_back(file.path().filename().string());
        }
        if (file.path().extension() == ".txt")
        {
            txt_files.push_back(file.path().stem().string());
        }
    }

    for (const auto &mp3 : mp3_files)
    {
        std::string mp3_base = mp3.substr(0, mp3.size() - 4); // Remove ".mp3" extension
        if (std::find(txt_files.begin(), txt_files.end(), mp3_base) == txt_files.end())
        {
            std::filesystem::path src_path = std::filesystem::path(directoryToMonitor) / mp3;
            std::filesystem::path dest_path = std::filesystem::path(directoryToMonitor) / mp3;
            std::filesystem::rename(src_path, dest_path); // Move the file
        }
    }
}

std::string getMP3Duration(const std::string &mp3FilePath)
{
    int pipefd[2];
    if (pipe(pipefd) == -1)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid == -1)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0)
    {                                   // Child process
        close(pipefd[0]);               // Close read end
        dup2(pipefd[1], STDOUT_FILENO); // Redirect stdout to write end of pipe
        close(pipefd[1]);               // Close write end (now that it's duplicated)

        execlp("ffprobe", "ffprobe", "-v", "error", "-show_entries", "format=duration", "-of", "default=noprint_wrappers=1:nokey=1", mp3FilePath.c_str(), NULL);
        perror("execlp");
        exit(EXIT_FAILURE);
    }
    else
    {                     // Parent process
        close(pipefd[1]); // Close write end

        char buffer[128];
        std::string durationStr;
        ssize_t bytesRead;
        while ((bytesRead = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0)
        {
            buffer[bytesRead] = '\0';
            durationStr += buffer;
        }

        close(pipefd[0]); // Close read end

        int status;
        waitpid(pid, &status, 0); // Wait for child to exit

        // Remove any trailing newline characters
        durationStr.erase(durationStr.find_last_not_of("\n\r") + 1);

        return durationStr;
    }
}

int generateUnixTimestamp(const std::string &date, const std::string &time)
{
    std::tm tm = {};
    std::string dateTime = date + time;
    std::istringstream ss(dateTime);
    ss >> std::get_time(&tm, "%Y%m%d%H%M%S"); // Parse the combined string
    auto tp = std::chrono::system_clock::from_time_t(std::mktime(&tm));
    return std::chrono::duration_cast<std::chrono::seconds>(tp.time_since_epoch()).count();
}

// Checks if the file should be skipped
bool skipFile(const std::string &file_path)
{
    return isFileBeingWrittenTo(file_path) || isFileLocked(file_path);
}

// Validates the duration of the MP3 file
float validateDuration(const std::string &file_path, FileData &fileData)
{
    std::string durationStr = getMP3Duration(file_path);
    float duration = std::stof(durationStr);
    fileData.duration = durationStr; // Set the duration in FileData
    if (duration < 9.0)
    {
        std::filesystem::remove(file_path);
        return 0.0;
    }
    return duration;
}

// Transcribes the audio file
std::string transcribeAudio(const std::string &file_path, const std::string &OPENAI_API_KEY)
{
    return curl_transcribe_audio(file_path, OPENAI_API_KEY);
}

// Extracts information from the filename and transcription
void extractFileInfo(FileData &fileData, const std::string &filename, const std::string &transcription)
{
    //  Extract talkgroup ID
    size_t start = filename.find("TO_") + 3;
    size_t end = filename.find("_FROM_");

    // Find the last underscore before "_FROM_" to correctly set the end position
    size_t lastUnderscoreBeforeFrom = filename.rfind('_', end - 1);
    if (lastUnderscoreBeforeFrom != std::string::npos && lastUnderscoreBeforeFrom > start)
    {
        end = lastUnderscoreBeforeFrom;
    }

    std::string talkgroupID = filename.substr(start, end - start);

    // Check if talkgroupID starts with 'P_'
    if (talkgroupID.substr(0, 2) == "P_")
    {
        talkgroupID = talkgroupID.substr(2); // Remove the 'P_' prefix
    }

    // Check if talkgroupID contains square brackets and extract the ID before it
    size_t bracketPos = talkgroupID.find("[");
    if (bracketPos != std::string::npos)
    {
        talkgroupID = talkgroupID.substr(0, bracketPos);
    }
    // Extract variables from filename
    std::string date = filename.substr(0, 8);
    std::string time = filename.substr(9, 6);
    size_t startRadioID = filename.find("_FROM_") + 6;
    size_t endRadioID = filename.find(".mp3");
    std::string radioID = filename.substr(startRadioID, endRadioID - startRadioID);
    fileData.radioID = std::stoi(radioID);
    fileData.talkgroupID = std::stoi(talkgroupID);
    fileData.date = date;
    fileData.time = time;
    fileData.unixtime = generateUnixTimestamp(fileData.date, fileData.time);
    fileData.filename = filename;
    fileData.transcription = transcription;
    fileData.v2transcription = generateV2Transcription(transcription, fileData.talkgroupID, fileData.radioID);
}

// Saves the transcription to a TXT file
void saveTranscription(const FileData &fileData)
{
    std::string txt_filename = fileData.filename.substr(0, fileData.filename.size() - 4) + ".txt";
    std::ofstream txt_file(txt_filename);
    txt_file << fileData.v2transcription;
    txt_file.close();
}

// Moves the MP3 and TXT files to the appropriate subdirectory
void moveFiles(const FileData &fileData, const std::string &directoryToMonitor)
{
    std::filesystem::path subDir = std::filesystem::path(directoryToMonitor) / std::to_string(fileData.talkgroupID);
    if (!std::filesystem::exists(subDir))
    {
        std::filesystem::create_directory(subDir);
    }

    // 22 std::cout << "[" << getCurrentTime() << "] " << "Moving file from: " << fileData.filepath << " to: " << (subDir / fileData.filename) << std::endl;  // Debug statement

    if (std::filesystem::exists(fileData.filepath))
    {
        std::filesystem::rename(fileData.filepath, subDir / fileData.filename);
    }

    std::string txt_filename = fileData.filename.substr(0, fileData.filename.size() - 4) + ".txt";

    // 22 std::cout << "[" << getCurrentTime() << "] " << "Moving txt from: " << txt_filename << " to: " << (subDir / txt_filename) << std::endl;  // Debug statement

    if (std::filesystem::exists(txt_filename))
    {
        std::filesystem::rename(txt_filename, subDir / txt_filename);
    }
}

// The refactored processFile function
FileData processFile(const std::filesystem::path &path, const std::string &directoryToMonitor, const std::string &OPENAI_API_KEY)
{
    try
    {
        FileData fileData;
        std::string file_path = path.string();
        // 22 std::cout << "[" << getCurrentTime() << "] " << "Processing file: " << file_path << std::endl;  // Debug statement
        bool shouldSkip = skipFile(file_path);
        float duration = validateDuration(file_path, fileData);
        // 22 std::cout << "[" << getCurrentTime() << "] " << "Should skip: " << shouldSkip << std::endl;  // Debug statement
        shouldSkip = shouldSkip || (duration == 0.0); // Update this line
        if (shouldSkip)
        {
            return FileData(); // Skip further processing
        }
        fileData.filepath = file_path;
        std::string transcription = transcribeAudio(file_path, OPENAI_API_KEY);
        extractFileInfo(fileData, path.filename().string(), transcription);

        saveTranscription(fileData);
        moveFiles(fileData, directoryToMonitor);

        return fileData;
    }
    catch (const std::exception &e)
    {
        std::cerr << "[" << getCurrentTime() << "] "
                  << "fileProcessor.cpp processFile Error: " << e.what() << std::endl;
        return FileData();
    }
}
