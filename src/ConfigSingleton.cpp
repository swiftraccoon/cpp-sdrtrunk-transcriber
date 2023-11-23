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

void ConfigSingleton::initialize(const YAML::Node &config)
{
    std::cout << "[" << getCurrentTime() << "] " << "ConfigSingleton.cpp Config YAML content:\n" << config << std::endl;
    // Parsing TALKGROUP_FILES with debug output
    const YAML::Node &tgFilesNode = config["TALKGROUP_FILES"];
    for (const auto &tg : tgFilesNode) {
        std::string tgKey = tg.first.as<std::string>();
        std::cout << "[" << getCurrentTime() << "] " << "ConfigSingleton.cpp Processing Talkgroup: " << tgKey << std::endl; // Debugging output

        std::unordered_set<int> tgIDs = parseTalkgroupIDs(tgKey);
        const YAML::Node &glossaryNode = tg.second["GLOSSARY"];
        std::vector<std::string> glossaryFiles;
        for (const auto &file : glossaryNode) {
            glossaryFiles.push_back(file.as<std::string>());
            std::cout << "[" << getCurrentTime() << "] " << "ConfigSingleton.cpp Added Glossary File: " << file.as<std::string>() << std::endl; // Debugging output
        }

        TalkgroupFiles tgFiles{glossaryFiles};
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
    debugCurlHelper = config["DEBUG_CURL_HELPER"].as<bool>(false);
    debugDatabaseManager = config["DEBUG_DATABASE_MANAGER"].as<bool>(false);
    debugFileProcessor = config["DEBUG_FILE_PROCESSOR"].as<bool>(false);
    debugMain = config["DEBUG_MAIN"].as<bool>(false);
    debugTranscriptionProcessor = config["DEBUG_TRANSCRIPTION_PROCESSOR"].as<bool>(false);
}

const std::unordered_map<int, TalkgroupFiles>& ConfigSingleton::getTalkgroupFiles() const {
        return talkgroupFiles;
}
std::string ConfigSingleton::getDatabasePath() const { return databasePath; }
std::string ConfigSingleton::getDirectoryToMonitor() const { return directoryToMonitor; }
std::string ConfigSingleton::getOpenAIAPIKey() const { return openaiAPIKey; }
int ConfigSingleton::getLoopWaitSeconds() const { return loopWaitSeconds; }
int ConfigSingleton::getMinDurationSeconds() const { return minDurationSeconds; }
int ConfigSingleton::getMaxRetries() const { return maxRetries; }
int ConfigSingleton::getMaxRequestsPerMinute() const { return maxRequestsPerMinute; }
int ConfigSingleton::getErrorWindowSeconds() const { return errorWindowSeconds; }
int ConfigSingleton::getRateLimitWindowSeconds() const { return rateLimitWindowSeconds; }
bool ConfigSingleton::isDebugCurlHelper() const { return debugCurlHelper; }
bool ConfigSingleton::isDebugDatabaseManager() const { return debugDatabaseManager; }
bool ConfigSingleton::isDebugFileProcessor() const { return debugFileProcessor; }
bool ConfigSingleton::isDebugMain() const { return debugMain; }
bool ConfigSingleton::isDebugTranscriptionProcessor() const { return debugTranscriptionProcessor; }
