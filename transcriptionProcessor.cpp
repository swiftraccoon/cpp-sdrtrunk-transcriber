// Standard Library Headers
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <unordered_map>
#include <vector>

// Third-Party Library Headers
#include "json/single_include/nlohmann/json.hpp"
#include <yaml-cpp/yaml.h>

// Project-Specific Headers
#include "ConfigSingleton.h"

std::unordered_map<std::string, std::string> readMappingFile(const std::string& filePath) {
    std::unordered_map<std::string, std::string> mapping;
    std::cout << "transcriptionProcessor.cpp Trying to open file: " << filePath << std::endl;
    std::ifstream file(filePath);
    if (file.is_open()) {
        std::string firstLine;
        std::getline(file, firstLine);
        std::cout << "transcriptionProcessor.cpp First line of the file: " << firstLine << std::endl;  // Debugging line
        file.seekg(0, std::ios::beg);  // Reset file pointer to the beginning

        try {
            nlohmann::json j;
            file >> j;
            for (auto &[key, value] : j.items())
            {
                if (value.is_string())
                {
                    mapping[key] = value.get<std::string>();
                }
                else if (value.is_number())
                {
                    mapping[key] = std::to_string(value.get<int>()); // or get<double>()
                }
                else
                {
                    // Handle other types or throw an exception
                }
            }
        } catch (nlohmann::json::parse_error& e) {
            std::cerr << "transcriptionProcessor.cpp JSON Parse Error: " << e.what() << std::endl;
        } catch (nlohmann::json::type_error& e) {
            std::cerr << "transcriptionProcessor.cpp JSON Type Error: " << e.what() << std::endl;
        }
    } else {
        std::cerr << "transcriptionProcessor.cpp Could not open file: " << filePath << std::endl;
    }
    return mapping;
}

std::string generateV2Transcription(const std::string &transcription, int talkgroupID, int radioID)
{
    std::cout << "transcriptionProcessor.cpp transcription: " << transcription << std::endl;
    std::cout << "transcriptionProcessor.cpp talkgroupID: " << talkgroupID << std::endl;
    std::cout << "transcriptionProcessor.cpp radioID: " << radioID << std::endl;
    std::string tensignFile = ConfigSingleton::getInstance().getTensignFile();
    std::string callsignFile = ConfigSingleton::getInstance().getCallsignFile();
    std::string signalFile = ConfigSingleton::getInstance().getSignalFile();

    if (talkgroupID == 52198 || talkgroupID == 52199 || talkgroupID == 52201)
    {
        tensignFile = ConfigSingleton::getInstance().getNCSHP_TensignFile();
        callsignFile = ConfigSingleton::getInstance().getNCSHP_CallsignFile();
        signalFile = ConfigSingleton::getInstance().getNCSHP_SignalFile();
    }

    std::unordered_map<std::string, std::string> tensigns = readMappingFile(tensignFile);
    std::unordered_map<std::string, std::string> callsigns = readMappingFile(callsignFile);
    std::unordered_map<std::string, std::string> signals = readMappingFile(signalFile);

    // Initialize v2transcription
    std::unordered_map<std::string, std::string> v2transcription;

    // Extract the actual transcription text from the JSON-like string
    std::regex text_regex("\"text\":\"([^\"]+)\"");
    std::smatch match;
    std::string actualTranscription;
    if (std::regex_search(transcription, match, text_regex) && match.size() > 1)
    {
        actualTranscription = match.str(1);
    }
    else
    {
        std::cerr << "transcriptionProcessor.cpp Could not extract actual transcription from JSON-like string." << std::endl;
        return "{}"; // Return an empty JSON object
    }

    // Populate the v2transcription map with the radioID as the key and actual transcription as the value
    std::string radioIDStr = std::to_string(radioID);
    v2transcription[radioIDStr] = actualTranscription;

    // Use actualTranscription for the word search
    std::regex word_regex("(\\S+)");
    auto words_begin = std::sregex_iterator(actualTranscription.begin(), actualTranscription.end(), word_regex);
    auto words_end = std::sregex_iterator();

    for (std::sregex_iterator i = words_begin; i != words_end; ++i)
    {
        std::string word = (*i).str();
        std::string upperWord = std::string(1, static_cast<char>(std::toupper(word[0]))) + word.substr(1);
        std::cout << "Looking for key: " << upperWord << std::endl;
        if (tensigns.find(upperWord) != tensigns.end())
        {
            v2transcription[upperWord] = tensigns[upperWord];
        }
        if (callsigns.find(upperWord) != callsigns.end())
        {
            v2transcription[upperWord] = callsigns[upperWord];
        }
        if (signals.find(upperWord) != signals.end())
        {
            v2transcription[upperWord] = signals[upperWord];
        }
    }

    // Convert the unordered_map to a JSON-like string
    std::string v2transcriptionStr = "{";
    for (const auto &pair : v2transcription)
    {
        for (const auto& pair : tensigns) {
            std::cout << pair.first << ": " << pair.second << std::endl;
        }
        v2transcriptionStr += "\"" + pair.first + "\": \"" + pair.second + "\", ";
    }
    v2transcriptionStr = v2transcriptionStr.substr(0, v2transcriptionStr.length() - 2) + "}";

    return v2transcriptionStr;
}