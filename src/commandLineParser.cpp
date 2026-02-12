#include "../include/commandLineParser.h"
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <map>
#include <functional>

void printHelp() {
    std::cout << "transcribe and process SDRTrunk mp3 recordings\n";
    std::cout << "Usage: sdrTrunkTranscriber [OPTIONS]\n\n";
    std::cout << "Options:\n";
    std::cout << "  -c, --config <path>  Configuration path (Optional, default is './config.yaml')\n";
    std::cout << "  -l, --local          Set this to enable local transcription via faster-whisper\n";
    std::cout << "  -p, --parallel       Enable parallel file processing (uses MAX_THREADS from config)\n";
    std::cout << "  -h, --help           Display this help message\n";
}

CommandLineArgs parseCommandLine(int argc, char* argv[]) {
    CommandLineArgs args;
    args.configPath = "./config.yaml";  // Default value
    args.localFlag = false;
    args.parallelFlag = false;
    args.helpFlag = false;

    // Map of option handlers - options that take no arguments
    std::map<std::string, std::function<void()>> simpleHandlers = {
        {"-h", [&](){ args.helpFlag = true; }},
        {"--help", [&](){ args.helpFlag = true; }},
        {"-l", [&](){ args.localFlag = true; }},
        {"--local", [&](){ args.localFlag = true; }},
        {"-p", [&](){ args.parallelFlag = true; }},
        {"--parallel", [&](){ args.parallelFlag = true; }}
    };

    // Map of option handlers - options that require an argument
    std::map<std::string, std::function<void(const std::string&)>> argHandlers = {
        {"-c", [&](const std::string& path){ args.configPath = path; }},
        {"--config", [&](const std::string& path){ args.configPath = path; }}
    };

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        // Check for simple options (no arguments)
        if (simpleHandlers.find(arg) != simpleHandlers.end()) {
            simpleHandlers[arg]();
            if (args.helpFlag) {
                return args;  // Exit early if help was requested
            }
        }
        // Check for options that require arguments
        else if (argHandlers.find(arg) != argHandlers.end()) {
            if (i + 1 < argc) {
                argHandlers[arg](argv[++i]);
            } else {
                std::cerr << "Error: " << arg << " requires an argument\n";
                printHelp();
                exit(1);
            }
        }
        // Unknown option
        else {
            std::cerr << "Unknown option: " << arg << "\n";
            printHelp();
            exit(1);
        }
    }
    
    return args;
}