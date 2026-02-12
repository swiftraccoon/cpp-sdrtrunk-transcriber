#ifndef TRANSCRIPTION_PROCESSOR_H
#define TRANSCRIPTION_PROCESSOR_H

// Standard Library Headers
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <vector>

struct TalkgroupFiles
{
    std::vector<std::string> glossaryFiles;
    std::string prompt;
};

// Function to read a mapping file and return an unordered_map
std::unordered_map<std::string, std::string> readMappingFile(const std::string &filePath);

std::string extractActualTranscription(const std::string &transcription);

std::string getAppropriateFile(int talkgroupID, std::function<std::string()> getNCSHPFile, std::function<std::string()> getDefaultFile);

void insertMappings(std::stringstream &orderedJsonStr, const std::string &actualTranscription, const std::unordered_map<std::string, std::string> &mappings);

std::unordered_set<int> parseTalkgroupIDs(const std::string &idString);

std::unordered_map<int, TalkgroupFiles> readTalkgroupFileMappings(const std::string &configFilePath);

std::string generateV2Transcription(
    const std::string &transcription,
    int talkgroupID,
    int radioID,
    const std::unordered_map<int, TalkgroupFiles> &talkgroupFiles);

#endif // TRANSCRIPTION_PROCESSOR_H