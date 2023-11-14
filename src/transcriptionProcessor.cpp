// Standard Library Headers
#include <algorithm>
#include <cctype>
#include <deque>
#include <fstream>
#include <functional>
#include <iostream>
#include <regex>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// Third-Party Library Headers
#include <nlohmann/json.hpp>
#include <yaml-cpp/yaml.h>

// Project-Specific Headers
#include "../include/ConfigSingleton.h"
#include "../include/debugUtils.h"

std::unordered_map<std::string, std::string> readMappingFile(const std::string &filePath);
std::string extractActualTranscription(const std::string &transcription);
void insertMappings(std::stringstream &orderedJsonStr, const std::string &actualTranscription, const std::unordered_map<std::string, std::string> &mappings);

std::unordered_set<int> specialTalkgroupIDs = {52197, 52198, 52199, 52200, 52201};

std::string getAppropriateFile(int talkgroupID, std::function<std::string()> getNCSHPFile, std::function<std::string()> getDefaultFile)
{
    if (specialTalkgroupIDs.find(talkgroupID) != specialTalkgroupIDs.end())
    {
        return getNCSHPFile();
    }
    return getDefaultFile();
}

std::string generateV2Transcription(const std::string &transcription, int talkgroupID, int radioID)
{
    ConfigSingleton &config = ConfigSingleton::getInstance();

    const auto tensignFile = getAppropriateFile(
        talkgroupID, [&]()
        { return config.getNCSHP_TensignFile(); },
        [&]()
        { return config.getTensignFile(); });
    const auto callsignFile = getAppropriateFile(
        talkgroupID, [&]()
        { return config.getNCSHP_CallsignFile(); },
        [&]()
        { return config.getCallsignFile(); });
    const auto signalsFile = getAppropriateFile(
        talkgroupID, [&]()
        { return config.getNCSHP_SignalFile(); },
        [&]()
        { return config.getSignalFile(); });

    const auto tensigns = readMappingFile(tensignFile);
    const auto callsigns = readMappingFile(callsignFile);
    const auto signals = readMappingFile(signalsFile);

    const auto actualTranscription = extractActualTranscription(transcription);
    if (actualTranscription.empty())
    {
        std::cerr << "[" << getCurrentTime() << "] "
                  << "transcriptionProcessor.cpp generateV2Transcription Could not extract actual transcription from JSON-like string. Received transcription string: '"
                  << transcription << "'" << std::endl;
        return {};
    }

    std::stringstream orderedJsonStr;
    orderedJsonStr << "{";
    orderedJsonStr << "\"" << std::to_string(radioID) << "\":\"" << actualTranscription << "\"";

    insertMappings(orderedJsonStr, actualTranscription, tensigns);
    insertMappings(orderedJsonStr, actualTranscription, signals);
    insertMappings(orderedJsonStr, actualTranscription, callsigns);

    orderedJsonStr << "}";

    return orderedJsonStr.str();
}

std::unordered_map<std::string, std::string> readMappingFile(const std::string &filePath)
{
    std::unordered_map<std::string, std::string> mapping;
    std::ifstream file(filePath);
    if (!file.is_open())
    {
        if (ConfigSingleton::getInstance().isDebugTranscriptionProcessor())
        {
            std::cerr << "[" << getCurrentTime() << "] "
                      << "transcriptionProcessor.cpp readMappingFile Could not open file: " << filePath << std::endl;
        }
        return mapping;
    }

    try
    {
        nlohmann::json j;
        file >> j;
        // if (ConfigSingleton::getInstance().isDebugTranscriptionProcessor())
        // {
        //     std::cerr << "[" << getCurrentTime() << "] "
        //               << "transcriptionProcessor.cpp readMappingFile JSON: " << j << std::endl;
        // }
        for (const auto &[key, value] : j.items())
        {
            if (value.is_string())
            {
                mapping[key] = value.get<std::string>();
            }
            else if (value.is_number())
            {
                mapping[key] = std::to_string(value.get<int>());
            }
            else if (value.is_array())
            {
                std::cerr << "[" << getCurrentTime() << "] "
                          << "transcriptionProcessor.cpp readMappingFile Array type detected for key: " << key << "in " << filePath << std::endl;
            }
            else if (value.is_object())
            {
                // std::stringstream ss;
                // ss << value;
                // mapping[key] = ss.str();
                std::cerr << "[" << getCurrentTime() << "] "
                          << "transcriptionProcessor.cpp readMappingFile Object type detected for key: " << key << "in " << filePath << std::endl;
            }
            else if (value.is_boolean())
            {
                std::cerr << "[" << getCurrentTime() << "] "
                          << "transcriptionProcessor.cpp readMappingFile Boolean type detected for key: " << key << "in " << filePath << std::endl;
            }
            else if (value.is_null())
            {
                std::cerr << "[" << getCurrentTime() << "] "
                          << "transcriptionProcessor.cpp readMappingFile Null type detected for key: " << key << "in " << filePath << std::endl;
            }
            else
            {
                std::cerr << "[" << getCurrentTime() << "] "
                          << "transcriptionProcessor.cpp readMappingFile Unexpected type for key: " << key << "in " << filePath << std::endl;
            }
        }
    }
    catch (const nlohmann::json::exception &e)
    {
        std::cerr << "[" << getCurrentTime() << "] "
                  << "transcriptionProcessor.cpp readMappingFile JSON Error: " << e.what() << std::endl;
    }
    return mapping;
}

std::string extractActualTranscription(const std::string &transcription)
{
    if (ConfigSingleton::getInstance().isDebugTranscriptionProcessor())
    {
        std::cerr << "[" << getCurrentTime() << "] "
                  << "transcriptionProcessor.cpp extractActualTranscription Received transcription string: " << transcription << std::endl;
    }
    static const std::regex text_regex("\"text\":\"([^\"]+)\"");
    std::smatch match;
    return std::regex_search(transcription, match, text_regex) && match.size() > 1 ? match.str(1) : "";
}

void insertMappings(std::stringstream &orderedJsonStr, const std::string &actualTranscription, const std::unordered_map<std::string, std::string> &mappings)
{
    std::unordered_set<std::string> insertedKeys;

    for (const auto &[key, value] : mappings)
    {
        std::regex keyRegex("\\b" + key + "\\b", std::regex::icase); // Use word boundaries and case-insensitive matching
        std::smatch match;
        if (std::regex_search(actualTranscription, match, keyRegex) && insertedKeys.find(key) == insertedKeys.end())
        {
            orderedJsonStr << ", \"" << key << "\":\"" << value << "\"";
            insertedKeys.insert(key);
        }
    }
}
