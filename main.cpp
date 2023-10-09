#include <iostream>
#include <cstdlib>
#include <stdexcept>
#include <filesystem>
#include <thread>
#include <chrono>
#include <fstream>
#include "curlHelper.h"
#include "DatabaseManager.h"
#include "fileProcessor.h"
#include <yaml-cpp/yaml.h>

void displayHelp() {
    std::cout << "Usage: ./sdrTrunkTranscriber [options]\n";
    std::cout << "Options:\n";
    std::cout << "  -c <config_path>  Path to the configuration file. (Optional, default is './config.yaml')\n";
    std::cout << "  -h, --help    Display this help message.\n";
    std::cout << "\n";
    std::cout << "\n";
}

int main(int argc, char *argv[]) {
    std::string configPath = "./config.yaml";  // Default config file path
    
    // Parse command-line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-c" && i + 1 < argc) {
            configPath = argv[++i];  // Increment i to skip the next argument
        } else if (arg == "--help" || arg == "-h") {
            displayHelp();
            return 0;
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            displayHelp();
            return 1;
        }
    }

    // Check if config file exists
    if (!std::filesystem::exists(configPath)) {
        std::cerr << "Configuration file not found. Please specify a valid path using -c or place 'config.yaml' in the same directory as the binary.\n";
        return 1;
    }

    // Load config file
    YAML::Node config = YAML::LoadFile(configPath);

    std::string directoryToMonitor = config["DirectoryToMonitor"].as<std::string>();
    int loopWaitSeconds = config["LoopWaitSeconds"].as<int>();
    std::string OPENAI_API_KEY = config["OPENAI_API_KEY"].as<std::string>();
    std::string DATABASE_PATH = config["DATABASE_PATH"].as<std::string>();

    // Initialize DatabaseManager and create table
    DatabaseManager dbManager(DATABASE_PATH);
    dbManager.createTable();

    // Infinite loop to keep monitoring the directory
    while (true) {
        for (const auto &entry : std::filesystem::directory_iterator(directoryToMonitor)) {
            if (entry.path().extension() == ".mp3") {
                // Process the file and get needed data
                FileData fileData = processFile(entry.path(), directoryToMonitor);

                // Insert into database
                dbManager.insertRecording(fileData.date, fileData.time, fileData.unixtime, fileData.talkgroupID, fileData.radioID, fileData.duration, fileData.filename, fileData.filepath, fileData.transcription);
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(loopWaitSeconds)); 
    }
    return 0;
}