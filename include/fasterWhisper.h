#ifndef FASTERWHISPER_H
#define FASTERWHISPER_H

#include <string>
#include <expected>

// Returns transcription on success, error message on failure
std::expected<std::string, std::string> local_transcribe_audio(const std::string& mp3FilePath);

void cleanup_python();  // Clean up Python interpreter on exit

#endif // FASTERWHISPER_H
