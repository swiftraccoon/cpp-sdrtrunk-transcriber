#ifndef CONFIG_SINGLETON_H
#define CONFIG_SINGLETON_H

// Standard Library Headers
#include <string>

// Third-Party Library Headers
#include <yaml-cpp/yaml.h>

class ConfigSingleton
{
public:
    static ConfigSingleton &getInstance();
    void initialize(const YAML::Node &config);

    std::string getOpenAIAPIKey() const;
    std::string getTensignFile() const;
    std::string getCallsignFile() const;
    std::string getSignalFile() const;
    std::string getNCSHP_TensignFile() const;
    std::string getNCSHP_CallsignFile() const;
    std::string getNCSHP_SignalFile() const;
    std::string getDatabasePath() const;
    std::string getDirectoryToMonitor() const;
    int getLoopWaitSeconds() const;
    int getMaxRetries() const;
    int getMaxRequestsPerMinute() const;
    int getErrorWindowSeconds() const;
    int getRateLimitWindowSeconds() const;
    int getMinDurationSeconds() const;
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
    std::string tensignFile;
    std::string callsignFile;
    std::string signalFile;
    std::string NCSHP_tensignFile;
    std::string NCSHP_callsignFile;
    std::string NCSHP_signalFile;
    std::string databasePath;
    std::string directoryToMonitor;
    int loopWaitSeconds;
    int maxRetries;
    int maxRequestsPerMinute;
    int errorWindowSeconds;
    int rateLimitWindowSeconds;
    int minDurationSeconds;
    bool debugCurlHelper;
    bool debugDatabaseManager;
    bool debugFileProcessor;
    bool debugMain;
    bool debugTranscriptionProcessor;
};

#endif // CONFIG_SINGLETON_H
