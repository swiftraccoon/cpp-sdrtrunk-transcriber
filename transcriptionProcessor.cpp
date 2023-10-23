// Standard Library Headers
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <deque>
#include <algorithm>
#include <cctype>
#include <unordered_set>

// Third-Party Library Headers
#include "json/single_include/nlohmann/json.hpp"
#include <yaml-cpp/yaml.h>

// Project-Specific Headers
#include "ConfigSingleton.h"

constexpr int TALKGROUP_1 = 52198;
constexpr int TALKGROUP_2 = 52199;
constexpr int TALKGROUP_3 = 52201;

std::unordered_map<std::string, std::string> readMappingFile(const std::string& filePath);
std::string extractActualTranscription(const std::string& transcription);
void insertMappings(std::stringstream& orderedJsonStr, const std::string& actualTranscription, const std::unordered_map<std::string, std::string>& mappings);

std::string generateV2Transcription(const std::string& transcription, int talkgroupID, int radioID) {
    ConfigSingleton& config = ConfigSingleton::getInstance();
    const auto tensignFile = talkgroupID == TALKGROUP_1 || talkgroupID == TALKGROUP_2 || talkgroupID == TALKGROUP_3 ? config.getNCSHP_TensignFile() : config.getTensignFile();
    const auto callsignFile = talkgroupID == TALKGROUP_1 || talkgroupID == TALKGROUP_2 || talkgroupID == TALKGROUP_3 ? config.getNCSHP_CallsignFile() : config.getCallsignFile();
    const auto signalsFile = talkgroupID == TALKGROUP_1 || talkgroupID == TALKGROUP_2 || talkgroupID == TALKGROUP_3 ? config.getNCSHP_SignalFile() : config.getSignalFile();

    const auto tensigns = readMappingFile(tensignFile);
    const auto callsigns = readMappingFile(callsignFile);
    const auto signals = readMappingFile(signalsFile);

    const auto actualTranscription = extractActualTranscription(transcription);
    if (actualTranscription.empty()) {
        std::cerr << "transcriptionProcessor.cpp Could not extract actual transcription from JSON-like string." << std::endl;
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

std::unordered_map<std::string, std::string> readMappingFile(const std::string& filePath) {
    std::unordered_map<std::string, std::string> mapping;
    std::ifstream file(filePath);
    if (!file.is_open()) {
        //22 std::cerr << "transcriptionProcessor.cpp Could not open file: " << filePath << std::endl;
        return mapping;
    }

    try {
        nlohmann::json j;
        file >> j;
        std::cerr << "transcriptionProcessor.cpp JSON: " << j << std::endl;
        for (const auto& [key, value] : j.items()) {
            mapping[key] = value.is_string() ? value.get<std::string>() : std::to_string(value.get<int>());
        }
    } catch (const nlohmann::json::exception& e) {
        std::cerr << "transcriptionProcessor.cpp JSON Error: " << e.what() << std::endl;
    }
    return mapping;
}

std::string extractActualTranscription(const std::string& transcription) {
    static const std::regex text_regex("\"text\":\"([^\"]+)\"");
    std::smatch match;
    return std::regex_search(transcription, match, text_regex) && match.size() > 1 ? match.str(1) : "";
}

void insertMappings(std::stringstream& orderedJsonStr, const std::string& actualTranscription, const std::unordered_map<std::string, std::string>& mappings) {
    std::unordered_set<std::string> insertedKeys;

    for (const auto& [key, value] : mappings) {
        std::regex keyRegex("\\b" + key + "\\b", std::regex::icase); // Use word boundaries and case-insensitive matching
        std::smatch match;
        if (std::regex_search(actualTranscription, match, keyRegex) && insertedKeys.find(key) == insertedKeys.end()) {
            orderedJsonStr << ",\"" << key << "\":\"" << value << "\"";
            insertedKeys.insert(key);
        }
    }
}