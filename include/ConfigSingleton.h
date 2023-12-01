#ifndef CONFIG_SINGLETON_H
#define CONFIG_SINGLETON_H

// Standard Library Headers
#include <string>

// Third-Party Library Headers
#include <yaml-cpp/yaml.h>

#include "transcriptionProcessor.h"

class ConfigSingleton
{
public:
    static ConfigSingleton &getInstance();
    void initialize(const YAML::Node &config);

    std::string getOpenAIAPIKey() const;
    const std::unordered_map<int, TalkgroupFiles>& getTalkgroupFiles() const;
    std::string getDatabasePath() const;
    std::string getDirectoryToMonitor() const;
    int getloopWaitMilliseconds() const;
    int getMaxRetries() const;
    int getMaxRequestsPerMinute() const;
    int getErrorWindowSeconds() const;
    int getRateLimitWindowSeconds() const;
    int getMinDurationSeconds() const;
    int getMaxProcessFileThreads() const;
    int getMaxTranscribeThreads() const;
    bool isDebugCurlHelper() const;
    bool isDebugDatabaseManager() const;
    bool isDebugFileProcessor() const;
    bool isDebugMain() const;
    bool isDebugTranscriptionProcessor() const;

private:
    ConfigSingleton() = default;
    ~ConfigSingleton() = default;
    ConfigSingleton(const ConfigSingleton &) = delete;
    ConfigSingleton &operator=(const ConfigSingleton &) = delete;

    std::string openaiAPIKey;
    std::string databasePath;
    std::string directoryToMonitor;
    std::unordered_map<int, TalkgroupFiles> talkgroupFiles;
    int loopWaitMilliseconds;
    int maxRetries;
    int maxRequestsPerMinute;
    int errorWindowSeconds;
    int rateLimitWindowSeconds;
    int minDurationSeconds;
    int maxProcessFileThreads;
    int maxTranscribeThreads;
    bool debugCurlHelper;
    bool debugDatabaseManager;
    bool debugFileProcessor;
    bool debugMain;
    bool debugTranscriptionProcessor;
};

#endif // CONFIG_SINGLETON_H
