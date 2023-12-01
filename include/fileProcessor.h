#pragma once

// Standard Library Headers
#include <filesystem>
#include <string>

// Project-Specific Headers
#include "FileData.h"

FileData processFile(const std::filesystem::path &path, const std::string &directoryToMonitor, boost::asio::thread_pool& transcribePool);
void find_and_move_mp3_without_txt(const std::string &directoryToMonitor);
bool isFileBeingWrittenTo(const std::string &filePath);
bool isFileLocked(const std::string &filePath);