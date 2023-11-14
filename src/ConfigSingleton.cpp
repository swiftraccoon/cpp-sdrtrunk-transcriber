// Project-Specific Headers
#include "../include/ConfigSingleton.h"

ConfigSingleton &ConfigSingleton::getInstance()
{
    static ConfigSingleton instance;
    return instance;
}

void ConfigSingleton::initialize(const YAML::Node &config)
{
    tensignFile = config["TENSIGN_FILE"].as<std::string>();
    callsignFile = config["CALLSIGNS_FILE"].as<std::string>();
    signalFile = config["SIGNALS_FILE"].as<std::string>();
    NCSHP_tensignFile = config["NCSHP_TENSIGN_FILE"].as<std::string>();
    NCSHP_callsignFile = config["NCSHP_CALLSIGNS_FILE"].as<std::string>();
    NCSHP_signalFile = config["NCSHP_SIGNALS_FILE"].as<std::string>();
    databasePath = config["DATABASE_PATH"].as<std::string>();
    directoryToMonitor = config["DirectoryToMonitor"].as<std::string>();
    loopWaitSeconds = config["LoopWaitSeconds"].as<int>();
    openaiAPIKey = config["OPENAI_API_KEY"].as<std::string>();
    maxRetries = config["MAX_RETRIES"].as<int>();
    maxRequestsPerMinute = config["MAX_REQUESTS_PER_MINUTE"].as<int>();
    errorWindowSeconds = config["ERROR_WINDOW_SECONDS"].as<int>();
    rateLimitWindowSeconds = config["RATE_LIMIT_WINDOW_SECONDS"].as<int>();
}

std::string ConfigSingleton::getTensignFile() const
{
    return tensignFile;
}

std::string ConfigSingleton::getCallsignFile() const
{
    return callsignFile;
}

std::string ConfigSingleton::getSignalFile() const
{
    return signalFile;
}

std::string ConfigSingleton::getNCSHP_TensignFile() const
{
    return NCSHP_tensignFile;
}

std::string ConfigSingleton::getNCSHP_CallsignFile() const
{
    return NCSHP_callsignFile;
}

std::string ConfigSingleton::getNCSHP_SignalFile() const
{
    return NCSHP_signalFile;
}

std::string ConfigSingleton::getDatabasePath() const
{
    return databasePath;
}

std::string ConfigSingleton::getDirectoryToMonitor() const
{
    return directoryToMonitor;
}

std::string ConfigSingleton::getOpenAIAPIKey() const
{
    return openaiAPIKey;
}

int ConfigSingleton::getLoopWaitSeconds() const
{
    return loopWaitSeconds;
}

int ConfigSingleton::getMaxRetries() const { return maxRetries; }
int ConfigSingleton::getMaxRequestsPerMinute() const { return maxRequestsPerMinute; }
int ConfigSingleton::getErrorWindowSeconds() const { return errorWindowSeconds; }
int ConfigSingleton::getRateLimitWindowSeconds() const { return rateLimitWindowSeconds; }