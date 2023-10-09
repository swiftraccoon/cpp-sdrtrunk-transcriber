#include <iostream>
#include <cstdlib>
#include <stdexcept>
#include <filesystem>
#include <thread>
#include <chrono>
#include "curlHelper.h"
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
    std::string openAI_API_KEY = config["OpenAI_API_KEY"].as<std::string>();

    std::cout << "Directory to Monitor: " << directoryToMonitor << std::endl;
    std::cout << "Loop Wait Seconds: " << loopWaitSeconds << std::endl;
    std::cout << "OpenAI API Key: " << openAI_API_KEY << std::endl;

    // Infinite loop to keep monitoring the directory
    while (true) {
        for (const auto &entry : std::filesystem::directory_iterator(directoryToMonitor)) {
            if (entry.path().extension() == ".mp3") {
                std::string file_path = entry.path().string();
                try {
                    std::cout << curl_transcribe_audio(file_path) << std::endl;
                } catch (const std::exception &e) {
                    std::cerr << "Error: " << e.what() << std::endl;
                }
            }
        }
        // Wait for specified duration before next iteration
        std::this_thread::sleep_for(std::chrono::seconds(loopWaitSeconds)); 
    }

    return 0;
}