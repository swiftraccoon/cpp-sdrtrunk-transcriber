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

// Function to parse a string representing a range or list of talkgroup IDs
std::unordered_set<int> parseTalkgroupIDs(const std::string &idString)
{
    std::unordered_set<int> ids;
    std::stringstream ss(idString);
    std::string token;
    while (std::getline(ss, token, ','))
    {
        size_t dash = token.find('-');
        if (dash != std::string::npos)
        {
            int start = std::stoi(token.substr(0, dash));
            int end = std::stoi(token.substr(dash + 1));
            for (int i = start; i <= end; ++i)
            {
                ids.insert(i);
            }
        }
        else
        {
            ids.insert(std::stoi(token));
        }
    }
    return ids;
}

// Function to read talkgroup-specific file mappings from config.yaml
std::unordered_map<int, TalkgroupFiles> readTalkgroupFileMappings(const std::string &configFilePath)
{
    YAML::Node config = YAML::LoadFile(configFilePath);
    std::unordered_map<int, TalkgroupFiles> mappings;

    if (config["talkgroupFiles"])
    {
        for (const auto &tg : config["talkgroupFiles"])
        {
            auto ids = parseTalkgroupIDs(tg.first.as<std::string>());
            TalkgroupFiles files;
            for (const auto &file : tg.second["glossary"])
            {
                files.glossaryFiles.push_back(file.as<std::string>());
            }
            for (int id : ids)
            {
                mappings[id] = files;
            }
        }
    }
    return mappings;
}

std::string generateV2Transcription(
    const std::string &transcription,
    int talkgroupID,
    int radioID,
    const std::unordered_map<int, TalkgroupFiles> &talkgroupFiles)
{
    // Extract the actual transcription
    const auto actualTranscription = extractActualTranscription(transcription);
    if (actualTranscription.empty())
    {
        std::cerr << "[" << getCurrentTime() << "] "
                  << "transcriptionProcessor.cpp generateV2Transcription Could not extract actual transcription from JSON-like string. Received transcription string: '"
                  << transcription << "'" << std::endl;
        return {};
    }

    std::unordered_map<std::string, std::string> mappings;

    // Check if there are glossary files for the talkgroup
    auto it = talkgroupFiles.find(talkgroupID);
    if (it != talkgroupFiles.end())
    {
        // Process each glossary file
        for (const auto &file : it->second.glossaryFiles)
        {
            // Read the mappings from each glossary file
            auto fileMappings = readMappingFile(file);
            // Merge the mappings from this file into the main mappings map
            mappings.insert(fileMappings.begin(), fileMappings.end());
        }
    }

    // Build the ordered JSON string
    std::stringstream orderedJsonStr;
    orderedJsonStr << "{";
    orderedJsonStr << "\"" << std::to_string(radioID) << "\":\"" << actualTranscription << "\"";

    // Insert the mappings into the JSON string
    insertMappings(orderedJsonStr, actualTranscription, mappings);

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
