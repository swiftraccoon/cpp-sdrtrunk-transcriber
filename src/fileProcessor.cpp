// Standard Library Headers
#include <algorithm>
#include <ranges>
#include <array>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <iostream>
#include <iomanip>
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
#include "../include/MP3Duration.h"
#include "../include/Result.h"
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
        if (std::ranges::find(txt_files, mp3_base) == txt_files.end())
        {
            std::filesystem::path src_path = std::filesystem::path(directoryToMonitor) / mp3;
            std::filesystem::path dest_path = std::filesystem::path(directoryToMonitor) / mp3;
            std::filesystem::rename(src_path, dest_path); // Move the file
        }
    }
}

// Using libmpg123 for accurate MP3 duration extraction
std::string getMP3Duration(const std::string &mp3FilePath)
{
    // Use libmpg123 for sample-accurate duration with gapless support
    auto result = sdrtrunk::getMP3Duration(mp3FilePath);

    if (result.has_value()) {
        // Return duration as string with 6 decimal places to match ffprobe format
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%.6f", result.value());
        return std::string(buffer);
    }

    // Log error if debug enabled
    if (ConfigSingleton::getInstance().isDebugFileProcessor()) {
        std::cerr << "[" << getCurrentTime() << "] "
                  << "MP3 duration error: " << result.error().toString() << std::endl;
    }
    return "";  // Return empty string on error to maintain compatibility
}

// Legacy version using ffprobe subprocess - kept for reference but deprecated
std::string getMP3DurationLegacy(const std::string &mp3FilePath)
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
        auto [first, last] = std::ranges::remove(outStr, '\n');
        outStr.erase(first, outStr.end());
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

        auto [first, last] = std::ranges::remove(result, '\n');
        result.erase(first, result.end());
        return result;
    }
#endif
    return "";  // Should never reach here, but ensures all paths return
}

int64_t generateUnixTimestamp(const std::string &date, const std::string &time)
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
    
    // Handle empty or invalid duration strings
    if (durationStr.empty()) {
        if (ConfigSingleton::getInstance().isDebugFileProcessor()) {
            std::cout << "[" << getCurrentTime() << "] "
                      << "fileProcessor.cpp validateDuration: Empty duration string, setting to 0.0" << std::endl;
        }
        fileData.duration = Duration(std::chrono::seconds(0));
        return 0.0f;
    }

    float duration = 0.0f;
    try {
        duration = std::stof(durationStr);
        fileData.duration = Duration(std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::duration<float>(duration)));
    } catch (const std::invalid_argument& e) {
        if (ConfigSingleton::getInstance().isDebugFileProcessor()) {
            std::cout << "[" << getCurrentTime() << "] "
                      << "fileProcessor.cpp validateDuration: Invalid duration format '" << durationStr
                      << "', setting to 0.0" << std::endl;
        }
        fileData.duration = Duration(std::chrono::seconds(0));
        return 0.0f;
    } catch (const std::out_of_range& e) {
        if (ConfigSingleton::getInstance().isDebugFileProcessor()) {
            std::cout << "[" << getCurrentTime() << "] "
                      << "fileProcessor.cpp validateDuration: Duration out of range '" << durationStr
                      << "', setting to 0.0" << std::endl;
        }
        fileData.duration = Duration(std::chrono::seconds(0));
        return 0.0f;
    }

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

// Extract talkgroup ID from filename without full extractFileInfo processing
// Handles all SDRTrunk __TO_ formats:
//   Standard:     __TO_52324_FROM_...                        -> 52324
//   With version: __TO_41001_V2_FROM_...                     -> 41001
//   NBFM:         __TO_9969                                  -> 9969
//   P-group alone:      __TO_P52197                          -> 52197
//   P-group bracket:    __TO_P52197-[52198]_FROM_...         -> 52198
//   P-group bracket _:  __TO_P52197_[52198]_FROM_...         -> 52198
//   P-group multi:      __TO_P52197-[52198--51426--56881]... -> 52198
//   P-group multi _:    __TO_P52197_[52198__52199]...        -> 52198
int extractTalkgroupIdFromFilename(const std::string &filename)
{
    std::smatch match;

    if (filename.find("TO_P") != std::string::npos)
    {
        // P-group with brackets: extract first number inside [ ]
        // Matches [52198] and [52198--51426] and [52198__52199] alike
        std::regex rgx_bracket("\\[(\\d+)");
        if (std::regex_search(filename, match, rgx_bracket) && match.size() > 1)
        {
            return std::stoi(match[1].str());
        }

        // P-group without brackets: TO_P52197 or legacy TO_P_52198
        std::regex rgx_talkgroup_P("TO_P_?(\\d+)");
        if (std::regex_search(filename, match, rgx_talkgroup_P) && match.size() > 1)
        {
            return std::stoi(match[1].str());
        }
    }

    // Standard: TO_digits (also handles _V2 suffix since \d+ stops at _)
    std::regex rgx_talkgroup("TO_(\\d+)");
    if (std::regex_search(filename, match, rgx_talkgroup) && match.size() > 1)
    {
        return std::stoi(match[1].str());
    }

    return 0;
}

// Transcribes the audio file
std::string transcribeAudio(const std::string &file_path, const std::string &OPENAI_API_KEY, const std::string &prompt = "")
{
    return curl_transcribe_audio(file_path, OPENAI_API_KEY, prompt);
}

// Transcribe the audio file locally with whisper.cpp
std::string transcribeAudioLocal(const std::string &file_path)
{
    auto result = local_transcribe_audio(file_path);
    if (result.has_value()) {
        return result.value();
    } else {
        std::cerr << "[ERROR] Failed to transcribe " << file_path << ": " << result.error() << std::endl;
        return "";  // Return empty string on error
    }
}

// Extracts information from the filename and transcription
void extractFileInfo(FileData &fileData, const std::string &filename, const std::string &transcription)
{
    std::smatch match;
    std::string talkgroupID, radioID;
    int defaultID = 1234567; // Default integer value for NBFM usage

    // Regular expression for radioID
    std::regex rgx_radio("_FROM_(\\d+)");

    // Extract talkgroup ID — handle all SDRTrunk P-group variants
    bool tgFound = false;
    if (filename.find("TO_P") != std::string::npos)
    {
        // P-group with brackets: first number inside [ ] is the primary talkgroup
        // Handles [52198], [52198--51426--56881], [52198__52199]
        std::regex rgx_bracket("\\[(\\d+)");
        if (std::regex_search(filename, match, rgx_bracket) && match.size() > 1)
        {
            talkgroupID = match[1].str();
            tgFound = true;
        }
        else
        {
            // P-group without brackets: TO_P52197 or legacy TO_P_52198
            std::regex rgx_talkgroup_P("TO_P_?(\\d+)");
            if (std::regex_search(filename, match, rgx_talkgroup_P) && match.size() > 1)
            {
                talkgroupID = match[1].str();
                tgFound = true;
            }
        }
    }
    if (!tgFound)
    {
        // Standard: TO_digits (handles _V2 suffix since \d+ stops at _)
        std::regex rgx_talkgroup("TO_(\\d+)");
        if (std::regex_search(filename, match, rgx_talkgroup) && match.size() > 1)
        {
            talkgroupID = match[1].str();
        }
        else
        {
            talkgroupID = std::to_string(defaultID);
        }
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

    // Extract talkgroup name: text between timestamp (pos 15) and "__TO_"
    // New SDRTrunk format has underscore separator: 20260208_121343_Name
    // Old format runs name directly after time:      20250715_112349Name
    std::string talkgroupName;
    auto toPos = filename.find("__TO_");
    if (toPos != std::string::npos && toPos > 15) {
        size_t nameStart = 15;
        // Skip leading underscore separator if present
        if (nameStart < filename.size() && filename[nameStart] == '_') {
            nameStart++;
        }
        if (nameStart < toPos) {
            talkgroupName = filename.substr(nameStart, toPos - nameStart);
        }
    }

    std::cout << "[" << getCurrentTime() << "] "
              << "fileProcessor.cpp extractFileInfo RID: " << radioID << std::endl;
    std::cout << "[" << getCurrentTime() << "] "
              << "fileProcessor.cpp extractFileInfo TGID: " << talkgroupID << std::endl;
    fileData.radioID = RadioId(std::stoi(radioID));
    fileData.talkgroupID = TalkgroupId(std::stoi(talkgroupID));
    fileData.talkgroupName = talkgroupName;
    fileData.date = date;
    fileData.time = time;
    // Parse timestamp properly
    std::tm tm = {};
    std::istringstream ss(date + time);
    ss >> std::get_time(&tm, "%Y%m%d%H%M%S");
    fileData.timestamp = std::chrono::system_clock::from_time_t(std::mktime(&tm));
    fileData.filename = FilePath(std::filesystem::path(filename));
    fileData.transcription = Transcription(transcription);

    // Retrieve the talkgroupFiles map from ConfigSingleton
    const auto &talkgroupFiles = ConfigSingleton::getInstance().getTalkgroupFiles();
    fileData.v2transcription = Transcription(generateV2Transcription(
        transcription,
        fileData.talkgroupID.get(),
        fileData.radioID.get(),
        talkgroupFiles));
}

// Saves the transcription to a TXT file alongside the MP3
void saveTranscription(const FileData &fileData)
{
    std::filesystem::path txt_path = fileData.filepath.get();
    txt_path.replace_extension(".txt");
    std::ofstream txt_file(txt_path);
    txt_file << fileData.v2transcription.get();
    txt_file.close();
}

// Moves the MP3 and TXT files to the appropriate subdirectory
void moveFiles(const FileData &fileData, const std::string &directoryToMonitor)
{
    std::filesystem::path subDir = std::filesystem::path(directoryToMonitor) / std::to_string(fileData.talkgroupID.get());
    if (!std::filesystem::exists(subDir))
    {
        std::filesystem::create_directory(subDir);
    }
    if (ConfigSingleton::getInstance().isDebugFileProcessor())
    {
        std::cout << "[" << getCurrentTime() << "] "
                  << "fileProcessor.cpp moveFiles Moving file from: " << fileData.filepath.get() << " to: " << (subDir / fileData.filename.get()) << std::endl;
    }

    if (std::filesystem::exists(fileData.filepath.get()))
    {
        std::filesystem::rename(fileData.filepath.get(), subDir / fileData.filename.get());
    }

    std::filesystem::path txt_path = fileData.filepath.get();
    txt_path.replace_extension(".txt");
    if (ConfigSingleton::getInstance().isDebugFileProcessor())
    {
        std::cout << "[" << getCurrentTime() << "] "
                  << "fileProcessor.cpp moveFiles Moving txt from: " << txt_path << " to: " << (subDir / txt_path.filename()) << std::endl;
    }

    if (std::filesystem::exists(txt_path))
    {
        std::filesystem::rename(txt_path, subDir / txt_path.filename());
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
        fileData.filepath = FilePath(std::filesystem::path(file_path));

        // Look up per-talkgroup prompt before transcription
        std::string prompt;
        int tgId = extractTalkgroupIdFromFilename(path.filename().string());
        if (tgId > 0) {
            auto it = ConfigSingleton::getInstance().getTalkgroupFiles().find(tgId);
            if (it != ConfigSingleton::getInstance().getTalkgroupFiles().end()) {
                prompt = it->second.prompt;
            }
        }

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
            transcription = transcribeAudio(file_path, OPENAI_API_KEY, prompt);
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
