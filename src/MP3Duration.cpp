/**
 * @file MP3Duration.cpp
 * @brief MP3 duration extraction using libmpg123
 *
 * This implementation uses the robust libmpg123 library to accurately
 * determine MP3 file duration, including proper handling of:
 * - Variable bitrate (VBR) files
 * - Constant bitrate (CBR) files
 * - Xing/VBRI/LAME headers with frame count
 * - Gapless playback metadata (encoder delay and padding)
 * - Corrupted or truncated files
 */

#include "../include/MP3Duration.h"
#include <mpg123.h>
#include <memory>

namespace sdrtrunk {

// RAII wrapper for mpg123 cleanup
struct MPG123Deleter {
    void operator()(mpg123_handle* mh) const {
        if (mh) {
            mpg123_close(mh);
            mpg123_delete(mh);
        }
    }
};

// Initialize mpg123 library once
class MPG123Initializer {
public:
    MPG123Initializer() {
        if (mpg123_init() != MPG123_OK) {
            initialized = false;
        } else {
            initialized = true;
        }
    }

    ~MPG123Initializer() {
        if (initialized) {
            mpg123_exit();
        }
    }

    bool isInitialized() const { return initialized; }

private:
    bool initialized;
};

Result<double> getMP3Duration(const std::string& filepath) {
    // Initialize mpg123 library (thread-safe singleton pattern)
    static MPG123Initializer initializer;
    if (!initializer.isInitialized()) {
        return Err<double>(ErrorCode::SystemError, "Failed to initialize mpg123 library");
    }

    // Create new mpg123 handle
    int err = MPG123_OK;
    std::unique_ptr<mpg123_handle, MPG123Deleter> mh(mpg123_new(nullptr, &err));

    if (!mh || err != MPG123_OK) {
        return Err<double>(ErrorCode::SystemError,
                          "Failed to create mpg123 handle: " + std::string(mpg123_plain_strerror(err)));
    }

    // Enable gapless playback (uses LAME/Xing delay+padding when present)
    mpg123_param(mh.get(), MPG123_ADD_FLAGS, MPG123_GAPLESS, 0.0);

    // Open the MP3 file
    if (mpg123_open(mh.get(), filepath.c_str()) != MPG123_OK) {
        return Err<double>(ErrorCode::FileNotFound,
                          "Cannot open file: " + filepath + " - " + mpg123_strerror(mh.get()));
    }

    // Scan all frames to build index for accurate duration
    // This is fast and doesn't decode to PCM
    int scan_result = mpg123_scan(mh.get());
    if (scan_result != MPG123_OK && scan_result != MPG123_NEW_FORMAT) {
        // Some files might not scan perfectly but still have valid duration
        // Continue anyway unless it's a critical error
        if (scan_result != MPG123_DONE) {
            // Non-critical, continue
        }
    }

    // Get the format info (sample rate needed for duration calculation)
    long sample_rate = 0;
    int channels = 0;
    int encoding = 0;
    if (mpg123_getformat(mh.get(), &sample_rate, &channels, &encoding) != MPG123_OK) {
        return Err<double>(ErrorCode::InvalidFormat,
                          "Cannot determine MP3 format for: " + filepath);
    }

    if (sample_rate <= 0) {
        return Err<double>(ErrorCode::InvalidFormat,
                          "Invalid sample rate in: " + filepath);
    }

    // Get total number of samples (gapless-aware when metadata present)
    off_t total_samples = mpg123_length(mh.get());

    if (total_samples <= 0) {
        // If length() fails, we could fall back to frame counting,
        // but this is rare with properly scanned files
        return Err<double>(ErrorCode::InvalidFormat,
                          "Cannot determine duration for: " + filepath +
                          " (no frame data available)");
    }

    // Calculate duration in seconds
    double duration_seconds = static_cast<double>(total_samples) / static_cast<double>(sample_rate);

    // Sanity check - MP3 files shouldn't be days long
    if (duration_seconds < 0.0 || duration_seconds > 86400.0) {  // 24 hours
        return Err<double>(ErrorCode::InvalidFormat,
                          "Unrealistic duration calculated: " + std::to_string(duration_seconds) + " seconds");
    }

    return Ok(duration_seconds);
}

} // namespace sdrtrunk