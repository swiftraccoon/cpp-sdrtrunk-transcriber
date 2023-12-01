// Standard Library Headers
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <semaphore>
#include <stdexcept>
#include <thread>

// Third-Party Library Headers
#include <yaml-cpp/yaml.h>
#include <CLI/CLI.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/asio/post.hpp>

// Project-Specific Headers
#include "../include/ConfigSingleton.h"
#include "../include/curlHelper.h"
#include "../include/DatabaseManager.h"
#include "../include/debugUtils.h"
#include "../include/FileData.h"
#include "../include/fileProcessor.h"
#include "../include/globalFlags.h"
#include "../include/inotifyWatcher.h"

constexpr const char *DEFAULT_CONFIG_PATH = "./config.yaml";
constexpr const char *MP3_EXTENSION = ".mp3";
bool gLocalFlag = false;

std::optional<YAML::Node> loadConfig(const std::string &configPath);


std::optional<YAML::Node> loadConfig(const std::string &configPath)
{
    if (!std::filesystem::exists(configPath))
    {
        std::cerr << "[" << getCurrentTime() << "] "
                  << "main.cpp loadConfig Configuration file not found.\n";
        return std::nullopt;
    }
    // TODO: add more error handling here?
    return YAML::LoadFile(configPath);
}

void processExistingFiles(const std::string& directoryToMonitor, boost::asio::thread_pool& processFilePool,
                          boost::asio::thread_pool& transcribePool, int maxTranscribeThreads, DatabaseManager& dbManager, std::counting_semaphore<>& semaphore) {
    for (const auto& entry : std::filesystem::directory_iterator(directoryToMonitor)) {
        if (entry.path().extension() == MP3_EXTENSION) {
            std::string filePath = entry.path().string();
            std::cerr << "[" << getCurrentTime() << "] "
                      << "main.cpp processExistingFiles Processing existing file: " << filePath << std::endl;
            boost::asio::post(processFilePool, [&semaphore, &transcribePool, filePath, directoryToMonitor, maxTranscribeThreads, &dbManager]() {
                semaphore.acquire(); // Wait and acquire the semaphore
                try {
                    FileData fileData = processFile(filePath, directoryToMonitor, transcribePool);
                    if (!fileData.filename.empty()) {
                        fileData.talkgroupName = "TODO"; // Replace with actual logic to determine talkgroup name
                        dbManager.insertRecording(fileData.date, fileData.time, fileData.unixtime, fileData.talkgroupID, fileData.talkgroupName, fileData.radioID, fileData.duration, fileData.filename, fileData.filepath, fileData.transcription, fileData.v2transcription);
                    }
                } catch (const std::exception &e) {
                    std::cerr << "[" << getCurrentTime() << "] "
                              << "main.cpp processExistingFiles Error processing file: " << e.what() << std::endl;
                }
                semaphore.release(); // Release the semaphore
            });
        }
    }
}

void handleInotifyEvent(const struct inotify_event* event, const std::string& directoryToMonitor, boost::asio::thread_pool& processFilePool, boost::asio::thread_pool& transcribePool,const YAML::Node& config, DatabaseManager& dbManager, int maxTranscribeThreads) {
    if (event->mask & IN_CREATE) {
        std::string filePath = directoryToMonitor + "/" + std::string(event->name);
        std::cerr << "[" << getCurrentTime() << "] "
                      << "main.cpp handleInotifyEvent filePath: " << filePath << std::endl;
        if (filePath.substr(filePath.length() - 4) == MP3_EXTENSION) {
            std::cerr << "[" << getCurrentTime() << "] "
                      << "main.cpp handleInotifyEvent found mp3 file, entering IF statement.\n";
            boost::asio::post(processFilePool, [&transcribePool, filePath, directoryToMonitor, maxTranscribeThreads, &dbManager]() {
                try {
                    std::cerr << "[" << getCurrentTime() << "] "
                      << "main.cpp handleInotifyEvent entered TRY within boost::asio::post.\n";
                    FileData fileData = processFile(filePath, directoryToMonitor, transcribePool);
                    if (!fileData.filename.empty()) {
                        fileData.talkgroupName = "TODO"; // Replace with actual logic to determine talkgroup name
                        dbManager.insertRecording(fileData.date, fileData.time, fileData.unixtime, fileData.talkgroupID, fileData.talkgroupName, fileData.radioID, fileData.duration, fileData.filename, fileData.filepath, fileData.transcription, fileData.v2transcription);
                    }
                } catch (const std::exception &e) {
                    std::cerr << "[" << getCurrentTime() << "] "
                              << "main.cpp handleInotifyEvent Error processing file: " << e.what() << std::endl;
                }
            });
        }
    }
}

int main(int argc, char *argv[]) {
    std::cout << "[" << getCurrentTime() << "] "
              << "main.cpp started." << std::endl;

    CLI::App app{"transcribe and process SDRTrunk mp3 recordings"};
    std::string configPath = DEFAULT_CONFIG_PATH;
    app.add_option("-c,--config", configPath, "Configuration path (Optional, default is './config.yaml')");
    app.add_flag("-l,--local", gLocalFlag, "Set this to enable local transcription via faster-whisper");
    CLI11_PARSE(app, argc, argv);

    auto configOpt = loadConfig(configPath);
    if (!configOpt.has_value()) {
        return 1;
    }
    YAML::Node config = configOpt.value();

    ConfigSingleton::getInstance().initialize(config);

    std::string directoryToMonitor = config["DIRECTORY_TO_MONITOR"].as<std::string>();
    std::string databasePath = config["DATABASE_PATH"].as<std::string>();
    int maxProcessFileThreads = config["MAX_PROCESS_FILE_THREADS"].as<int>();
    int maxTranscribeThreads = config["MAX_TRANSCRIBE_THREADS"].as<int>();
    std::counting_semaphore<> semaphore(maxProcessFileThreads);

    DatabaseManager dbManager(databasePath);
    dbManager.createTable();

    int inotifyFd = inotify_init();
    if (inotifyFd == -1) {
        std::cerr << "[" << getCurrentTime() << "] "
                  << "main.cpp Error initializing inotify.\n";
        return 1;
    }

    int wd = inotify_add_watch(inotifyFd, directoryToMonitor.c_str(), IN_CREATE);
    if (wd == -1) {
        std::cerr << "[" << getCurrentTime() << "] "
                  << "main.cpp Error adding watch to inotify.\n";
        return 1;
    }

    // Create the two separate thread pools
    boost::asio::thread_pool fileProcessPool(maxProcessFileThreads);
    boost::asio::thread_pool transcribePool(maxTranscribeThreads);
    // Process existing files in the directory
    processExistingFiles(directoryToMonitor, fileProcessPool, transcribePool, maxTranscribeThreads, dbManager, semaphore);

    char buffer[1024];
    while (true) {
        int length = read(inotifyFd, buffer, 1024);
        if (length < 0) {
            std::cerr << "[" << getCurrentTime() << "] "
                      << "main.cpp Error reading inotify event.\n";
            break;
        }

        int i = 0;
        while (i < length) {
            struct inotify_event *event = (struct inotify_event *)&buffer[i];
            if (event->len) {
                handleInotifyEvent(event, directoryToMonitor, fileProcessPool, transcribePool, config, dbManager, maxTranscribeThreads);
            }
            i += sizeof(struct inotify_event) + event->len;
        }
    }

    inotify_rm_watch(inotifyFd, wd);
    close(inotifyFd);
    fileProcessPool.join();
    transcribePool.join();
    return 0;
}