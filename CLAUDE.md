# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

```bash
# Initial setup - clone external dependencies
git clone https://github.com/jbeder/yaml-cpp.git external/yaml-cpp
git clone https://github.com/CLIUtils/CLI11.git external/CLI11

# Generate Makefile
cmake .

# Build the project
make

# Build with specific build type
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

# Run tests (if enabled - currently commented out in CMakeLists.txt)
ctest -C Release
```

## Running the Application

```bash
# Run with default config (./config.yaml)
./sdrTrunkTranscriber

# Run with custom config
./sdrTrunkTranscriber -c /path/to/config.yaml

# Run with local transcription (faster-whisper)
./sdrTrunkTranscriber --local

# Display help
./sdrTrunkTranscriber -h
```

## Project Architecture

### Core Components

**Main Application Flow** (`src/main.cpp`):
- Monitors a directory for new MP3 files from SDRTrunk P25 recordings
- Uses a polling loop with configurable wait time (LoopWaitSeconds in config)
- Processes files through transcription pipeline (OpenAI API or local faster-whisper)
- Stores results in SQLite database

**Configuration System** (`src/ConfigSingleton.cpp`, `include/ConfigSingleton.h`):
- Singleton pattern for global configuration access
- Loads YAML configuration from config.yaml
- Manages API keys, paths, debug flags, and talkgroup-specific glossary mappings

**File Processing** (`src/fileProcessor.cpp`, `include/fileProcessor.h`):
- Parses SDRTrunk MP3 filenames to extract metadata (date, time, talkgroup ID, radio ID)
- Validates file duration against MIN_DURATION_SECONDS threshold
- Manages file movement and organization based on talkgroup

**Transcription Processing** (`src/transcriptionProcessor.cpp`, `include/transcriptionProcessor.h`):
- Handles OpenAI API transcription via whisper model
- Integrates with local faster-whisper Python script for offline transcription
- Searches transcriptions for tencodes/signals/callsigns and appends translations

**Database Management** (`src/DatabaseManager.cpp`, `include/DatabaseManager.h`):
- SQLite3 database operations for storing file metadata and transcriptions
- Schema initialization and management
- Query operations for processed files

**API Communication** (`src/curlHelper.cpp`, `include/curlHelper.h`):
- CURL wrapper for OpenAI API requests
- Rate limiting (MAX_REQUESTS_PER_MINUTE)
- Error handling with retry logic (MAX_RETRIES)
- Error threshold monitoring (25% errors in ERROR_WINDOW_SECONDS triggers exit)

**Local Transcription** (`src/fasterWhisper.cpp`, `include/fasterWhisper.h`):
- Integration with faster-whisper Python script
- Executes Python script as subprocess for local transcription
- Used when --local flag is set

### Data Structure

**FileData** (`include/FileData.h`):
- Core data structure containing:
  - Date, time, unix timestamp
  - Talkgroup ID and name
  - Radio ID
  - Duration
  - File path and name
  - Transcription text (v1 and v2)

### Configuration Schema

Key configuration parameters in `config.yaml`:
- `OPENAI_API_KEY`: API key for OpenAI transcription
- `DirectoryToMonitor`: Path to SDRTrunk recordings
- `DATABASE_PATH`: SQLite database location
- `TALKGROUP_FILES`: Glossary mappings per talkgroup
- `MIN_DURATION_SECONDS`: Minimum file duration to process
- `LoopWaitSeconds`: Polling frequency (milliseconds)
- Debug flags for each component

### External Dependencies

- **yaml-cpp**: YAML configuration parsing
- **CLI11**: Command-line argument parsing
- **nlohmann/json**: JSON parsing (in external/json/single_include)
- **SQLite3**: Database operations
- **CURL**: HTTP requests to OpenAI API
- **ffmpeg**: Audio file processing (system dependency)

### Python Integration

The `scripts/fasterWhisper.py` script:
- Must be in the same directory as the binary when using --local
- Uses faster-whisper with large-v3 model
- Configured for radio traffic transcription (high noise threshold, specific VAD parameters)
- Returns JSON-formatted transcription results

### Debugging

Enable debug output for specific components in config.yaml:
- `DEBUG_CURL_HELPER`
- `DEBUG_DATABASE_MANAGER`
- `DEBUG_FILE_PROCESSOR`
- `DEBUG_MAIN`
- `DEBUG_TRANSCRIPTION_PROCESSOR`

## Important Notes

- The project expects SDRTrunk MP3 files with specific naming convention containing metadata
- Glossary files for tencodes/signals must follow the format shown in `howToFormatYourJSON.json`
- Tests are currently disabled in CMakeLists.txt (lines 49-71 commented out)
- The system service installation script (`install-systemd-service.sh`) is untested
- Windows support is experimental and not fully tested