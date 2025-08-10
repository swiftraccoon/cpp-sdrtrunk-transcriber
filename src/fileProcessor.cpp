// Standard Library Headers
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <iostream>
#include <memory>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
#include <sys/types.h>
#include "security.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#endif

// Project-Specific Headers
#include "../include/ConfigSingleton.h"
#include "../include/curlHelper.h"
#include "../include/debugUtils.h"
#include "../include/FileData.h"
#include "../include/fileProcessor.h"
#include "../include/globalFlags.h"
#include "../include/transcriptionProcessor.h"
#include "../include/fasterWhisper.h"

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
#ifdef _WIN32
    // Windows-specific implementation
    // Securely escape the file path to prevent command injection
    std::string escapedPath = Security::escapeShellArg(mp3FilePath);
    std::string command = "ffprobe -v error -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 " + escapedPath;

    // Create a pipe for the child process's STDOUT
    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    HANDLE g_hChildStd_OUT_Rd = NULL;
    HANDLE g_hChildStd_OUT_Wr = NULL;

    // Create a pipe for the child process's STDOUT
    if (!CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0))
    {
        std::cerr << "StdoutRd CreatePipe failed\n";
        return "";
    }

    // Ensure the read handle to the pipe for STDOUT is not inherited
    if (!SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0))
    {
        std::cerr << "Stdout SetHandleInformation failed\n";
        return "";
    }

    // Create the child process
    PROCESS_INFORMATION piProcInfo;
    STARTUPINFO siStartInfo;
    BOOL bSuccess = FALSE;

    ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
    ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
    siStartInfo.cb = sizeof(STARTUPINFO);
    siStartInfo.hStdError = g_hChildStd_OUT_Wr;
    siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    bSuccess = CreateProcess(NULL,
                             const_cast<char *>(command.c_str()), // command line
                             NULL,                                // process security attributes
                             NULL,                                // primary thread security attributes
                             TRUE,                                // handles are inherited
                             0,                                   // creation flags
                             NULL,                                // use parent's environment
                             NULL,                                // use parent's current directory
                             &siStartInfo,                        // STARTUPINFO pointer
                             &piProcInfo);                        // receives PROCESS_INFORMATION

    // Close handles to the stdin and stdout pipes no longer needed by the child process
    // If they are not explicitly closed, there is no way to recognize that the child process has ended
    CloseHandle(g_hChildStd_OUT_Wr);

    if (!bSuccess)
    {
        std::cerr << "CreateProcess failed\n";
        return "";
    }
    else
    {
        // Read output from the child process's pipe for STDOUT
        // Stop when there is no more data
        DWORD dwRead;
        CHAR chBuf[4096];
        std::string outStr;
        bSuccess = FALSE;
        HANDLE hParentStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

        for (;;)
        {
            bSuccess = ReadFile(g_hChildStd_OUT_Rd, chBuf, 4096, &dwRead, NULL);
            if (!bSuccess || dwRead == 0)
                break;

            std::string str(chBuf, dwRead);
            outStr += str;
        }

        // The remaining cleanup is the same for both processes
        CloseHandle(piProcInfo.hProcess);
        CloseHandle(piProcInfo.hThread);

        // Trim the output string
        outStr.erase(std::remove(outStr.begin(), outStr.end(), '\n'), outStr.end());
        return outStr;
    }
#else
    // Original POSIX-specific implementation
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
        std::string result;
        ssize_t count;
        while ((count = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0)
        {
            buffer[count] = '\0';
            result += buffer;
        }

        close(pipefd[0]); // Close read end

        int status;
        waitpid(pid, &status, 0); // Wait for child process to finish

        if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
        {
            std::cerr << "ffprobe process failed\n";
            return "";
        }

        result.erase(std::remove(result.begin(), result.end(), '\n'), result.end());
        return result;
    }
#endif
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
    if (ConfigSingleton::getInstance().isDebugFileProcessor())
    {
        std::cout << "[" << getCurrentTime() << "] "
                  << "fileProcessor.cpp validateDuration var durationStr: " << durationStr << std::endl;
    }
    float duration = std::stof(durationStr);
    fileData.duration = durationStr; // Set the duration in FileData

    float minDuration = ConfigSingleton::getInstance().getMinDurationSeconds();

    if (ConfigSingleton::getInstance().isDebugFileProcessor())
    {
        std::cout << "[" << getCurrentTime() << "] "
                  << "fileProcessor.cpp validateDuration var duration: " << duration << std::endl;
    }

    if (duration < minDuration)
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

// Transcribe the audio file locally with whisper.cpp
std::string transcribeAudioLocal(const std::string &file_path)
{
    return local_transcribe_audio(file_path);
}

// Extracts information from the filename and transcription
void extractFileInfo(FileData &fileData, const std::string &filename, const std::string &transcription)
{
    std::smatch match;
    std::string talkgroupID, radioID;
    int defaultID = 1234567; // Default integer value for NBFM usage

    // Regular expressions for talkgroupID with and without 'P_' prefix
    std::regex rgx_talkgroup_P("TO_P_(\\d+)");
    std::regex rgx_talkgroup("TO_(\\d+)");
    // Regular expression for radioID
    std::regex rgx_radio("_FROM_(\\d+)");

    // Determine which regex to use based on whether the filename contains 'P_'
    if (std::regex_search(filename, match, filename.find("TO_P_") != std::string::npos ? rgx_talkgroup_P : rgx_talkgroup) && match.size() > 1)
    {
        talkgroupID = match[1].str();
    }
    else
    {
        talkgroupID = std::to_string(defaultID); // Set to default value
    }

    // Use a single regex search for radioID
    if (std::regex_search(filename, match, rgx_radio) && match.size() > 1)
    {
        radioID = match[1].str();
    }
    else
    {
        radioID = std::to_string(defaultID); // Set to default value
    }
    // Extract variables from filename
    std::string date = filename.substr(0, 8);
    std::string time = filename.substr(9, 6);
    std::cout << "[" << getCurrentTime() << "] "
              << "fileProcessor.cpp extractFileInfo RID: " << radioID << std::endl;
    std::cout << "[" << getCurrentTime() << "] "
              << "fileProcessor.cpp extractFileInfo TGID: " << talkgroupID << std::endl;
    fileData.radioID = std::stoi(radioID);
    fileData.talkgroupID = std::stoi(talkgroupID);
    fileData.date = date;
    fileData.time = time;
    fileData.unixtime = generateUnixTimestamp(fileData.date, fileData.time);
    fileData.filename = filename;
    fileData.transcription = transcription;

    // Retrieve the talkgroupFiles map from ConfigSingleton
    const auto &talkgroupFiles = ConfigSingleton::getInstance().getTalkgroupFiles();
    fileData.v2transcription = generateV2Transcription(
        transcription,
        fileData.talkgroupID,
        fileData.radioID,
        talkgroupFiles);
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
    if (ConfigSingleton::getInstance().isDebugFileProcessor())
    {
        std::cout << "[" << getCurrentTime() << "] "
                  << "fileProcessor.cpp moveFiles Moving file from: " << fileData.filepath << " to: " << (subDir / fileData.filename) << std::endl;
    }

    if (std::filesystem::exists(fileData.filepath))
    {
        std::filesystem::rename(fileData.filepath, subDir / fileData.filename);
    }

    std::string txt_filename = fileData.filename.substr(0, fileData.filename.size() - 4) + ".txt";
    if (ConfigSingleton::getInstance().isDebugFileProcessor())
    {
        std::cout << "[" << getCurrentTime() << "] "
                  << "fileProcessor.cpp moveFiles Moving txt from: " << txt_filename << " to: " << (subDir / txt_filename) << std::endl;
    }

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
        // Validate that the file path is within the allowed directory (prevents path traversal)
        if (!Security::isPathSafe(path, directoryToMonitor)) {
            std::cerr << "[" << getCurrentTime() << "] "
                      << "Security: Rejected file outside allowed directory: " << path << std::endl;
            FileData emptyData;
            return emptyData;
        }
        
        FileData fileData;
        std::string file_path = path.string();
        if (ConfigSingleton::getInstance().isDebugFileProcessor())
        {
            std::cout << "[" << getCurrentTime() << "] "
                      << "fileProcessor.cpp processFile Processing file: " << file_path << std::endl;
        }
        bool shouldSkip = skipFile(file_path);
        float duration = validateDuration(file_path, fileData);
        if (ConfigSingleton::getInstance().isDebugFileProcessor())
        {
            std::cout << "[" << getCurrentTime() << "] "
                      << "fileProcessor.cpp processFile shouldSkip isFileBeingWrittenTo || isFileLocked: " << shouldSkip << std::endl;
        }
        shouldSkip = shouldSkip || (duration == 0.0);
        if (shouldSkip)
        {
            if (ConfigSingleton::getInstance().isDebugFileProcessor())
            {
                std::cout << "[" << getCurrentTime() << "] "
                          << "fileProcessor.cpp processFile shouldSkip || duration check: " << shouldSkip << std::endl;
                std::cout << "[" << getCurrentTime() << "] "
                          << "fileProcessor.cpp processFile Skipping: " << file_path << std::endl;
            }
            return FileData(); // Skip further processing
        }
        fileData.filepath = file_path;
        std::string transcription;
        if (gLocalFlag)
        {
            std::cout << "[" << getCurrentTime() << "] "
                      << "fileProcessor.cpp processFile gLocalFlag " << gLocalFlag << std::endl;
            transcription = transcribeAudioLocal(file_path);
        }
        else
        {
            std::cout << "[" << getCurrentTime() << "] "
                      << "fileProcessor.cpp processFile gLocalFlag " << gLocalFlag << std::endl;
            transcription = transcribeAudio(file_path, OPENAI_API_KEY);
        }
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
