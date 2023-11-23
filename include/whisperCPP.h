#ifndef WHISPERCPP_H
#define WHISPERCPP_H

#include <string>

// Function declaration
std::string local_transcribe_audio(const std::string& mp3FilePath);
std::string convertMp3ToWav(const std::string& mp3FilePath);

#endif // WHISPERCPP_H
