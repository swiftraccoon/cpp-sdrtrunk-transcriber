#pragma once
#include <string>
#include <filesystem>
#include "FileData.h"

FileData processFile(const std::filesystem::path& path, const std::string& directoryToMonitor, const std::string& OPENAI_API_KEY);
void find_and_move_mp3_without_txt(const std::string &directoryToMonitor);