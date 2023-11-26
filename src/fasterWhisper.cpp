#include <iostream>
#include <string>
#include <array>
#include <memory>
// Include necessary headers for Windows
#ifdef _WIN32
#include <stdio.h>
#else
// POSIX headers for other platforms
#include <cstdio>
#endif

std::string trim(const std::string &str)
{
    const char *whitespace = " \t\n\r\f\v";

    size_t start = str.find_first_not_of(whitespace);
    size_t end = str.find_last_not_of(whitespace);

    return (start == std::string::npos || end == std::string::npos) ? "" : str.substr(start, end - start + 1);
}

std::string local_transcribe_audio(const std::string &mp3FilePath)
{
    std::string command = "python fasterWhisper.py " + mp3FilePath;

    std::array<char, 128> buffer;
    std::string result;

// Use the appropriate popen and pclose functions based on the platform
#ifdef _WIN32
    std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(command.c_str(), "r"), _pclose);
#else
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
#endif

    if (!pipe)
    {
        throw std::runtime_error("popen() failed!");
    }

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
    {
        result += buffer.data();
    }

    size_t jsonStartPos = result.find('{');
    if (jsonStartPos != std::string::npos)
    {
        return trim(result.substr(jsonStartPos));
    }

    return "MUCH_BROKEN_very_wow";
}