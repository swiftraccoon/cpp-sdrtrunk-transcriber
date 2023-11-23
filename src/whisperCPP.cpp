#include <iostream>
#include <string>
#include <array>
#include <memory>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <mpg123.h>
#include <sndfile.h>
#include <vector>
std::string convertMp3ToWav(const std::string& mp3FilePath); // Ensure this is declared

std::string local_transcribe_audio(const std::string& mp3FilePath) {
    std::string cmd;
    // whisper.cpp requires a WAV file, so we convert the MP3 file to WAV
    std::string wavFilePath = convertMp3ToWav(mp3FilePath); // Declare and initialize wavFilePath;
    #ifdef _WIN32
    // Windows command
    cmd = "..\\external\\whisper-bin-x64\\main.exe -oj -m models/ggml-medium.en.bin -f " + wavFilePath;
    #else
    // Linux command
    cmd = "../external/whisper.cpp/build/bin/main -oj -m ../external/whisper.cpp/build/bin/models/ggml-medium.en.bin -f " + wavFilePath;
    #endif

    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

std::string convertMp3ToWav(const std::string& mp3FilePath) {
    mpg123_handle *mp3;
    SNDFILE *wav;
    SF_INFO info;
    int err;

    // Generate WAV file path
    std::string wavFilePath = mp3FilePath.substr(0, mp3FilePath.find_last_of('.')) + ".wav";

    // Initialize libmpg123
    if (mpg123_init() != MPG123_OK) {
        std::cerr << "Failed to initialize mpg123" << std::endl;
        return "";
    }

    // Open MP3 file
    mp3 = mpg123_new(nullptr, &err);
    if (mpg123_open(mp3, mp3FilePath.c_str()) != MPG123_OK) {
        std::cerr << "Failed to open MP3 file" << std::endl;
        mpg123_close(mp3);
        mpg123_delete(mp3);
        mpg123_exit();
        return "";
    }

    // Get MP3 file info
    long rate;
    int channels, encoding;
    mpg123_getformat(mp3, &rate, &channels, &encoding);

    // Set up libsndfile info
    info.samplerate = rate;
    info.channels = channels;
    info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

    // Open WAV file for writing
    wav = sf_open(wavFilePath.c_str(), SFM_WRITE, &info);
    if (!wav) {
        std::cerr << "Failed to open WAV file for writing" << std::endl;
        mpg123_close(mp3);
        mpg123_delete(mp3);
        mpg123_exit();
        return "";
    }

    // Buffer for audio data
    std::vector<short> buffer(8192);

    // Read from MP3 and write to WAV
    size_t done;
    int samples;
    while ((samples = mpg123_read(mp3, reinterpret_cast<unsigned char*>(buffer.data()), buffer.size() * sizeof(short), &done)) == MPG123_OK) {
        sf_write_short(wav, buffer.data(), done / sizeof(short));
    }

    // Close files and clean up libmpg123
    sf_close(wav);
    mpg123_close(mp3);
    mpg123_delete(mp3);
    mpg123_exit();

    // Temporary WAV file path
    std::string tempWavFilePath = wavFilePath + ".temp";

    // Rename original WAV file to temporary file
    std::rename(wavFilePath.c_str(), tempWavFilePath.c_str());

    // Use FFmpeg to resample the WAV file to 16 kHz
    std::string ffmpegCmd = "ffmpeg -i " + tempWavFilePath + " -ar 16000 " + wavFilePath;
    if (std::system(ffmpegCmd.c_str()) != 0) {
        std::cerr << "FFmpeg failed to resample WAV file" << std::endl;
        return "";
    }

    // Remove temporary WAV file
    std::remove(tempWavFilePath.c_str());

    return wavFilePath;
}

