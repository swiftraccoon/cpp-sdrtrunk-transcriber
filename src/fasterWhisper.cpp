#include <iostream>
#include <string>
#include <stdio.h>
#include <memory>
#include <array>

std::string trim(const std::string& str) {
    const char* whitespace = " \t\n\r\f\v";

    size_t start = str.find_first_not_of(whitespace);
    size_t end = str.find_last_not_of(whitespace);

    return (start == std::string::npos || end == std::string::npos) ? "" : str.substr(start, end - start + 1);
}

std::string local_transcribe_audio(const std::string& mp3FilePath) {
    // Command to execute the Python script
    std::string command = "python fasterWhisper.py " + mp3FilePath;

    // Create a pipe to read the output of the executed command
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }

    // Read the output a line at a time
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }

    // Find the start of the JSON object and return everything from this point
    size_t jsonStartPos = result.find('{');
    if (jsonStartPos != std::string::npos) {
        std::string jsonResult = result.substr(jsonStartPos);
        // Trim whitespace and newline characters
        return trim(jsonResult);
    }

    return "";
}