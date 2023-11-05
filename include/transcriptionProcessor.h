#ifndef TRANSCRIPTION_PROCESSOR_H
#define TRANSCRIPTION_PROCESSOR_H

// Standard Library Headers
#include <string>
#include <unordered_map>
#include <functional>

// Third-Party Library Headers
#include <yaml-cpp/yaml.h>

// Function to read a mapping file and return an unordered_map
std::unordered_map<std::string, std::string> readMappingFile(const std::string &filePath);

std::string extractActualTranscription(const std::string &transcription);

std::string getAppropriateFile(int talkgroupID, std::function<std::string()> getNCSHPFile, std::function<std::string()> getDefaultFile);

void insertMappings(std::stringstream &orderedJsonStr, const std::string &actualTranscription, const std::unordered_map<std::string, std::string> &mappings);

std::string generateV2Transcription(const std::string &transcription, int talkgroupID, int radioID);

#endif // TRANSCRIPTION_PROCESSOR_H