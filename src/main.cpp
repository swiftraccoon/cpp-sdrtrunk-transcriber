// Standard Library Headers
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <thread>
#include <atomic>
#include <vector>

// Project-Specific Headers
#include "../include/commandLineParser.h"
#include "../include/ConfigSingleton.h"
#include "../include/curlHelper.h"
#include "../include/DatabaseManager.h"
#include "../include/debugUtils.h"
#include "../include/FileData.h"
#include "../include/fileProcessor.h"
#include "../include/fasterWhisper.h"
#include "../include/globalFlags.h"
#include "../include/ThreadPool.h"
#include "../include/yamlParser.h"

constexpr const char *DEFAULT_CONFIG_PATH = "./config.yaml";
constexpr const char *MP3_EXTENSION = ".mp3";
bool gLocalFlag = false;
bool gParallelFlag = false;
std::atomic<bool> g_shutdown_requested{false};

// Signal handler for graceful shutdown
void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\n[" << getCurrentTime() << "] Shutdown signal received. Cleaning up..." << std::endl;
        g_shutdown_requested = true;
    }
}

// Cleanup function to be called on exit
void cleanup() {
    std::cout << "[" << getCurrentTime() << "] Performing cleanup..." << std::endl;
    cleanup_python();  // Clean up Python interpreter if initialized
    std::cout << "[" << getCurrentTime() << "] Cleanup completed." << std::endl;
}

std::optional<YamlNode> loadConfig(const std::string &configPath);

void processDirectory(const std::string &directoryToMonitor, const YamlNode &config, DatabaseManager &dbManager);

std::optional<YamlNode> loadConfig(const std::string &configPath)
{
    if (!std::filesystem::exists(configPath))
    {
        std::cerr << "[" << getCurrentTime() << "] "
                  << "main.cpp loadConfig Configuration file not found.\n";
        return std::nullopt;
    }
    try {
        return YamlParser::loadFile(configPath);
    } catch (const std::exception& e) {
        std::cerr << "[" << getCurrentTime() << "] "
                  << "main.cpp loadConfig Error parsing YAML: " << e.what() << "\n";
        return std::nullopt;
    }
}

void insertFileData(DatabaseManager &dbManager, const FileData &fileData)
{
    if (!fileData.filename.get().empty())
    {
        try
        {
            auto unixtime = std::chrono::duration_cast<std::chrono::seconds>(
                fileData.timestamp.time_since_epoch()).count();
            dbManager.insertRecording(
                fileData.date,
                fileData.time,
                unixtime,
                fileData.talkgroupID.get(),
                fileData.talkgroupName,
                fileData.radioID.get(),
                static_cast<double>(fileData.duration.get().count()),
                fileData.filename.get().string(),
                fileData.filepath.get().string(),
                fileData.transcription.get(),
                fileData.v2transcription.get());
        }
        catch (const std::exception &e)
        {
            std::cerr << "[" << getCurrentTime() << "] "
                      << "main.cpp insertFileData Database insertion failed: " << e.what() << std::endl;
        }
    }
}

void processDirectory(const std::string &directoryToMonitor, const YamlNode &config, DatabaseManager &dbManager)
{
    std::string OPENAI_API_KEY = ConfigSingleton::getInstance().getOpenAIAPIKey();

    find_and_move_mp3_without_txt(directoryToMonitor);

    // Collect MP3 files to process
    std::vector<std::filesystem::path> mp3Files;
    for (const auto &entry : std::filesystem::directory_iterator(directoryToMonitor))
    {
        if (entry.path().parent_path() == std::filesystem::path(directoryToMonitor) &&
            entry.path().extension() == MP3_EXTENSION)
        {
            if (ConfigSingleton::getInstance().isDebugMain())
            {
                std::cout << "[" << getCurrentTime() << "] "
                          << "main.cpp processDirectory Processing directory: " << directoryToMonitor << std::endl;
                std::cout << "[" << getCurrentTime() << "] "
                          << "main.cpp processDirectory Checking file: " << entry.path() << std::endl;
            }
            mp3Files.push_back(entry.path());
        }
    }

    if (mp3Files.empty())
        return;

    int maxThreads = gParallelFlag ? ConfigSingleton::getInstance().getMaxThreads() : 1;

    if (maxThreads > 1 && mp3Files.size() > 1)
    {
        // Parallel processing with thread pool
        size_t poolSize = std::min(static_cast<size_t>(maxThreads), mp3Files.size());
        ThreadPool pool(poolSize);
        std::vector<std::future<FileData>> futures;

        for (const auto &path : mp3Files)
        {
            futures.push_back(pool.enqueue([path, &directoryToMonitor, &OPENAI_API_KEY]() {
                try
                {
                    return processFile(path, directoryToMonitor, OPENAI_API_KEY);
                }
                catch (const std::runtime_error &e)
                {
                    if (ConfigSingleton::getInstance().isDebugMain())
                    {
                        std::cerr << "[" << getCurrentTime() << "] "
                                  << "main.cpp processDirectory Skipping file: " << e.what() << std::endl;
                    }
                    return FileData();
                }
            }));
        }

        for (auto &future : futures)
        {
            FileData fileData = future.get();
            insertFileData(dbManager, fileData);
        }
    }
    else
    {
        // Sequential processing (default)
        for (const auto &path : mp3Files)
        {
            FileData fileData;
            try
            {
                fileData = processFile(path, directoryToMonitor, OPENAI_API_KEY);
            }
            catch (const std::runtime_error &e)
            {
                if (ConfigSingleton::getInstance().isDebugMain())
                {
                    std::cerr << "[" << getCurrentTime() << "] "
                              << "main.cpp processDirectory Skipping file: " << e.what() << std::endl;
                }
                continue;
            }
            insertFileData(dbManager, fileData);
        }
    }
}

int main(int argc, char *argv[])
{
    // Set up signal handlers for graceful shutdown
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    
    // Register cleanup function
    std::atexit(cleanup);
    
    std::cout << "[" << getCurrentTime() << "] "
              << "main.cpp started." << std::endl;
    FileData fileData{};

    CommandLineArgs args = parseCommandLine(argc, argv);
    
    if (args.helpFlag) {
        printHelp();
        return 0;
    }
    
    std::string configPath = args.configPath;
    gLocalFlag = args.localFlag;
    gParallelFlag = args.parallelFlag;

    auto configOpt = loadConfig(configPath);
    if (!configOpt.has_value())
    {
        return 1;
    }
    YamlNode config = configOpt.value();

    // Debugging: Print out all config.yaml variables
    std::cout << "[" << getCurrentTime() << "] "
              << "=======================================" << std::endl;
    std::cout << "[" << getCurrentTime() << "] "
              << "Config variables:" << std::endl;
    // below loop breaks our glossary implementation
    // for (YAML::const_iterator it = config.begin(); it != config.end(); ++it)
    // {
    //     std::cout << it->first.as<std::string>() << ": " << it->second.as<std::string>() << std::endl;
    // }
    std::cout << "[" << getCurrentTime() << "] "
              << "=======================================" << std::endl;

    ConfigSingleton::getInstance().initialize(config);

    std::string databasePath = ConfigSingleton::getInstance().getDatabasePath();
    DatabaseManager dbManager(databasePath);
    dbManager.createTable();

    // Cache frequently accessed config values
    std::string directoryToMonitor = config["DirectoryToMonitor"].as<std::string>();
    int loopWaitSeconds = config["LoopWaitSeconds"].as<int>();

    while (!g_shutdown_requested)
    {
        processDirectory(directoryToMonitor, config, dbManager);
        std::this_thread::sleep_for(std::chrono::milliseconds(loopWaitSeconds));
    }
    
    std::cout << "[" << getCurrentTime() << "] Shutdown requested. Exiting gracefully." << std::endl;
    return 0;
}