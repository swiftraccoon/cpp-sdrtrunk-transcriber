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
#include <curlHelper.h>
#include <yaml-cpp/yaml.h>

// Project-Specific Headers
#include "DatabaseManager.h"
#include "FileData.h"
#include "fileProcessor.h"


constexpr const char* DEFAULT_CONFIG_PATH = "./config.yaml";
constexpr const char* MP3_EXTENSION = ".mp3";


std::optional<YAML::Node> loadConfig(const std::string& configPath);


void processDirectory(const std::string& directoryToMonitor, const YAML::Node& config, DatabaseManager& dbManager);


void displayHelp()
{
    std::cout << "Usage: ./sdrTrunkTranscriber [options]\n";
    std::cout << "Options:\n";
    std::cout << "  -c <config_path>  Path to the configuration file. (Optional, default is './config.yaml')\n";
    std::cout << "  -h, --help    Display this help message.\n";
    std::cout << "\n";
    std::cout << "\n";
}


int main(int argc, char *argv[])
{
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
    if (!configOpt.has_value()) {
        return 1;
    }
    YAML::Node config = configOpt.value();

    DatabaseManager dbManager(config["DATABASE_PATH"].as<std::string>());
    dbManager.createTable();

    // Cache frequently accessed config values
    std::string directoryToMonitor = config["DirectoryToMonitor"].as<std::string>();
    int loopWaitSeconds = config["LoopWaitSeconds"].as<int>();

    while (true)
    {
        processDirectory(directoryToMonitor, config, dbManager);
        std::this_thread::sleep_for(std::chrono::seconds(loopWaitSeconds));
    }
    return 0;
}

std::optional<YAML::Node> loadConfig(const std::string& configPath) {
    if (!std::filesystem::exists(configPath)) {
        std::cerr << "Configuration file not found.\n";
        return std::nullopt;
    }
    // TODO: add more error handling here?
    return YAML::LoadFile(configPath);
}

void processDirectory(const std::string& directoryToMonitor, const YAML::Node& config, DatabaseManager& dbManager) {
    FileData fileData;
    std::string OPENAI_API_KEY = config["OPENAI_API_KEY"].as<std::string>();

    find_and_move_mp3_without_txt(directoryToMonitor);

    for (const auto &entry : std::filesystem::directory_iterator(directoryToMonitor))
    {
        if (entry.path().parent_path() == std::filesystem::path(directoryToMonitor))
        {
            if (entry.path().extension() == MP3_EXTENSION)
            {
                fileData = processFile(entry.path(), directoryToMonitor, OPENAI_API_KEY);
                
                // TODO: Consider batch database operations?
                dbManager.insertRecording(fileData.date, fileData.time, fileData.unixtime, fileData.talkgroupID, fileData.radioID, fileData.duration, fileData.filename, fileData.filepath, fileData.transcription);
            }
        }
    }
}  