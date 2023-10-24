// Standard Library Headers
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <thread>

// Third-Party Library Headers
#include <yaml-cpp/yaml.h>

// Project-Specific Headers
#include "ConfigSingleton.h"
#include "curlHelper.h"
#include "DatabaseManager.h"
#include "FileData.h"
#include "fileProcessor.h"

constexpr const char *DEFAULT_CONFIG_PATH = "./config.yaml";
constexpr const char *MP3_EXTENSION = ".mp3";

std::optional<YAML::Node> loadConfig(const std::string &configPath);

void processDirectory(const std::string &directoryToMonitor, const YAML::Node &config, DatabaseManager &dbManager);

void displayHelp()
{
    std::cout << "Usage: ./sdrTrunkTranscriber [options]\n";
    std::cout << "Options:\n";
    std::cout << "  -c <config_path>  Path to the configuration file. (Optional, default is './config.yaml')\n";
    std::cout << "  -h, --help    Display this help message.\n";
    std::cout << "\n";
    std::cout << "\n";
}

std::optional<YAML::Node> loadConfig(const std::string &configPath)
{
    if (!std::filesystem::exists(configPath))
    {
        std::cerr << "main.cpp Configuration file not found.\n";
        return std::nullopt;
    }
    // TODO: add more error handling here?
    return YAML::LoadFile(configPath);
}

void processDirectory(const std::string &directoryToMonitor, const YAML::Node &config, DatabaseManager &dbManager)
{
    FileData fileData;
    std::string OPENAI_API_KEY = ConfigSingleton::getInstance().getOpenAIAPIKey();

    find_and_move_mp3_without_txt(directoryToMonitor);

    for (const auto &entry : std::filesystem::directory_iterator(directoryToMonitor))
    {
        if (entry.path().parent_path() == std::filesystem::path(directoryToMonitor))
        {
            if (entry.path().extension() == MP3_EXTENSION)
            {
                // 22 std::cout << "main.cpp Processing directory: " << directoryToMonitor << std::endl;
                // 22 std::cout << "main.cpp Checking file: " << entry.path() << std::endl;
                try
                {
                    fileData = processFile(entry.path(), directoryToMonitor, OPENAI_API_KEY);
                }
                catch (const std::runtime_error &e)
                {
                    // 22 std::cerr << "main.cpp Skipping file: " << e.what() << std::endl;
                    continue; // Move on to the next file
                }

                try
                {
                    fileData.talkgroupName = "TODO";
                    dbManager.insertRecording(fileData.date, fileData.time, fileData.unixtime, fileData.talkgroupID, fileData.talkgroupName, fileData.radioID, fileData.duration, fileData.filename, fileData.filepath, fileData.transcription, fileData.v2transcription);
                }
                catch (const std::exception &e)
                {
                    std::cerr << "main.cpp Database insertion failed: " << e.what() << std::endl;
                }
            }
        }
    }
}

int main(int argc, char *argv[])
{
    std::cout << "main.cpp started." << std::endl;
    FileData fileData{};
    std::string configPath = DEFAULT_CONFIG_PATH;

    // Parse command-line arguments
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "-c" && i + 1 < argc)
        {
            configPath = argv[++i]; // Increment i to skip the next argument
        }
        else if (arg == "--help" || arg == "-h")
        {
            displayHelp();
            return 0;
        }
        else
        {
            std::cerr << "Unknown option: " << arg << std::endl;
            displayHelp();
            return 1;
        }
    }

    auto configOpt = loadConfig(configPath);
    if (!configOpt.has_value())
    {
        return 1;
    }
    YAML::Node config = configOpt.value();

    ConfigSingleton::getInstance().initialize(config);

    std::string databasePath = ConfigSingleton::getInstance().getDatabasePath();
    DatabaseManager dbManager(databasePath);
    dbManager.createTable();

    // Cache frequently accessed config values
    std::string directoryToMonitor = config["DirectoryToMonitor"].as<std::string>();
    int loopWaitSeconds = config["LoopWaitSeconds"].as<int>();

    while (true)
    {
        processDirectory(directoryToMonitor, config, dbManager);
        std::this_thread::sleep_for(std::chrono::seconds(loopWaitSeconds));
    }
    std::cout << "main.cpp ending." << std::endl;
    return 0;
}