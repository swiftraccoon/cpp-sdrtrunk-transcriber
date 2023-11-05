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
#include <CLI/CLI.hpp>

// Project-Specific Headers
#include "ConfigSingleton.h"
#include "curlHelper.h"
#include "DatabaseManager.h"
#include "debugUtils.h"
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
        std::cerr << "[" << getCurrentTime() << "] "
                  << "main.cpp Configuration file not found.\n";
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
                // 22 std::cout << "[" << getCurrentTime() << "] " << "main.cpp Processing directory: " << directoryToMonitor << std::endl;
                // 22 std::cout << "[" << getCurrentTime() << "] " << "main.cpp Checking file: " << entry.path() << std::endl;
                try
                {
                    fileData = processFile(entry.path(), directoryToMonitor, OPENAI_API_KEY);
                }
                catch (const std::runtime_error &e)
                {
                    // 22 std::cerr << "[" << getCurrentTime() << "] " << "main.cpp Skipping file: " << e.what() << std::endl;
                    continue; // Move on to the next file
                }

                if (!fileData.filename.empty()) // You can add more conditions based on other fields
                {
                    try
                    {
                        fileData.talkgroupName = "TODO";
                        dbManager.insertRecording(fileData.date, fileData.time, fileData.unixtime, fileData.talkgroupID, fileData.talkgroupName, fileData.radioID, fileData.duration, fileData.filename, fileData.filepath, fileData.transcription, fileData.v2transcription);
                    }
                    catch (const std::exception &e)
                    {
                        std::cerr << "[\" << getCurrentTime() << \"] "
                                  << "main.cpp Database insertion failed: " << e.what() << std::endl;
                    }
                }
            }
        }
    }
}

int main(int argc, char *argv[])
{
    std::cout << "[" << getCurrentTime() << "] "
              << "main.cpp started." << std::endl;
    FileData fileData{};

    CLI::App app{"transcribe and process SDRTrunk mp3 recordings"};
    std::string configPath = DEFAULT_CONFIG_PATH;
    app.add_option("-c,--config", configPath, "Configuration path (Optional, default is './config.yaml')");
    CLI11_PARSE(app, argc, argv);

    auto configOpt = loadConfig(configPath);
    if (!configOpt.has_value())
    {
        return 1;
    }
    YAML::Node config = configOpt.value();

    // Debugging: Print out all config.yaml variables
    std::cout << "[" << getCurrentTime() << "] "
              << "=======================================" << std::endl;
    std::cout << "[" << getCurrentTime() << "] "
              << "Config variables:" << std::endl;
    for (YAML::const_iterator it = config.begin(); it != config.end(); ++it)
    {
        std::cout << it->first.as<std::string>() << ": " << it->second.as<std::string>() << std::endl;
    }
    std::cout << "[" << getCurrentTime() << "] "
              << "=======================================" << std::endl;

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
    return 0;
}