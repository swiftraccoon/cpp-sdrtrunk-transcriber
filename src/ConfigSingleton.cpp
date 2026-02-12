#include <iostream> // Include iostream for debug output
#include <unordered_set>

// Project-Specific Headers
#include "../include/ConfigSingleton.h"
#include "../include/debugUtils.h"
#include "../include/transcriptionProcessor.h"

ConfigSingleton &ConfigSingleton::getInstance()
{
    static ConfigSingleton instance;
    return instance;
}

void ConfigSingleton::initialize(const YamlNode &config)
{
    // Parsing TALKGROUP_FILES with debug output
    const YamlNode &tgFilesNode = config["TALKGROUP_FILES"];
    auto tgKeys = tgFilesNode.getKeys();
    for (const auto &tgKey : tgKeys) {
        std::cout << "[" << getCurrentTime() << "] " << "ConfigSingleton.cpp Processing Talkgroup: " << tgKey << std::endl; // Debugging output

        std::unordered_set<int> tgIDs = parseTalkgroupIDs(tgKey);
        const YamlNode &tgNode = tgFilesNode[tgKey];
        const YamlNode &glossaryNode = tgNode["GLOSSARY"];
        
        std::vector<std::string> glossaryFiles;
        // Handle GLOSSARY as a vector of strings
        if (std::holds_alternative<std::vector<std::string>>(glossaryNode.getValue())) {
            glossaryFiles = std::get<std::vector<std::string>>(glossaryNode.getValue());
            for (const auto &file : glossaryFiles) {
                std::cout << "[" << getCurrentTime() << "] " << "ConfigSingleton.cpp Added Glossary File: " << file << std::endl; // Debugging output
            }
        }

        TalkgroupFiles tgFiles{glossaryFiles, {}};
        try {
            if (tgNode.hasKey("PROMPT")) {
                tgFiles.prompt = tgNode["PROMPT"].as<std::string>();
                std::cout << "[" << getCurrentTime() << "] " << "ConfigSingleton.cpp Added Prompt for talkgroup: " << tgKey << std::endl;
            }
        } catch (...) {}
        for (int id : tgIDs) {
            talkgroupFiles[id] = tgFiles;
        }
    }
    databasePath = config["DATABASE_PATH"].as<std::string>();
    directoryToMonitor = config["DirectoryToMonitor"].as<std::string>();
    loopWaitSeconds = config["LoopWaitSeconds"].as<int>();
    openaiAPIKey = config["OPENAI_API_KEY"].as<std::string>();
    maxRetries = config["MAX_RETRIES"].as<int>();
    maxRequestsPerMinute = config["MAX_REQUESTS_PER_MINUTE"].as<int>();
    errorWindowSeconds = config["ERROR_WINDOW_SECONDS"].as<int>();
    rateLimitWindowSeconds = config["RATE_LIMIT_WINDOW_SECONDS"].as<int>();
    minDurationSeconds = config["MIN_DURATION_SECONDS"].as<int>();
    try {
        maxThreads = config["MAX_THREADS"].as<int>();
    } catch (...) {
        maxThreads = 1;
    }
    // Handle optional debug flags with defaults
    try {
        debugCurlHelper = config["DEBUG_CURL_HELPER"].as<bool>();
    } catch (...) {
        debugCurlHelper = false;
    }
    try {
        debugDatabaseManager = config["DEBUG_DATABASE_MANAGER"].as<bool>();
    } catch (...) {
        debugDatabaseManager = false;
    }
    try {
        debugFileProcessor = config["DEBUG_FILE_PROCESSOR"].as<bool>();
    } catch (...) {
        debugFileProcessor = false;
    }
    try {
        debugMain = config["DEBUG_MAIN"].as<bool>();
    } catch (...) {
        debugMain = false;
    }
    try {
        debugTranscriptionProcessor = config["DEBUG_TRANSCRIPTION_PROCESSOR"].as<bool>();
    } catch (...) {
        debugTranscriptionProcessor = false;
    }
}

const std::unordered_map<int, TalkgroupFiles>& ConfigSingleton::getTalkgroupFiles() const {
        return talkgroupFiles;
}
std::string ConfigSingleton::getDatabasePath() const { return databasePath; }
std::string ConfigSingleton::getDirectoryToMonitor() const { return directoryToMonitor; }
std::string ConfigSingleton::getOpenAIAPIKey() const { return openaiAPIKey; }
int ConfigSingleton::getLoopWaitSeconds() const { return loopWaitSeconds; }
int ConfigSingleton::getMinDurationSeconds() const { return minDurationSeconds; }
int ConfigSingleton::getMaxThreads() const { return maxThreads; }
int ConfigSingleton::getMaxRetries() const { return maxRetries; }
int ConfigSingleton::getMaxRequestsPerMinute() const { return maxRequestsPerMinute; }
int ConfigSingleton::getErrorWindowSeconds() const { return errorWindowSeconds; }
int ConfigSingleton::getRateLimitWindowSeconds() const { return rateLimitWindowSeconds; }
bool ConfigSingleton::isDebugCurlHelper() const { return debugCurlHelper; }
bool ConfigSingleton::isDebugDatabaseManager() const { return debugDatabaseManager; }
bool ConfigSingleton::isDebugFileProcessor() const { return debugFileProcessor; }
bool ConfigSingleton::isDebugMain() const { return debugMain; }
bool ConfigSingleton::isDebugTranscriptionProcessor() const { return debugTranscriptionProcessor; }
