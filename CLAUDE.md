# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

```bash
# Configure and build (Debug with tests)
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
cmake --build build

# Configure and build (Release, no tests)
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

# Run tests
cd build && ctest --output-on-failure
```

## Running the Application

```bash
# Run with default config (./config.yaml)
./sdrTrunkTranscriber

# Run with custom config
./sdrTrunkTranscriber -c /path/to/config.yaml

# Run with local transcription (faster-whisper via pybind11)
./sdrTrunkTranscriber --local

# Run with parallel file processing (uses MAX_THREADS from config)
./sdrTrunkTranscriber --parallel

# Display help
./sdrTrunkTranscriber -h
```

## Project Architecture

### Language & Standard
- **C++23** (required, enforced by CMake)
- Uses `std::expected`, `std::ranges`, structured bindings, concepts

### Core Components

**Main Application Flow** (`src/main.cpp`):
- Monitors a directory for new MP3 files from SDRTrunk P25 recordings
- Uses a polling loop with configurable wait time (LoopWaitSeconds in config)
- Processes files through transcription pipeline (OpenAI API or local faster-whisper)
- Stores results in SQLite database

**Configuration System** (`src/ConfigSingleton.cpp`, `include/ConfigSingleton.h`):
- Singleton pattern for global configuration access
- Loads YAML configuration via in-house parser (`src/yamlParser.cpp`)
- Manages API keys, paths, debug flags, and talkgroup-specific glossary mappings

**File Processing** (`src/fileProcessor.cpp`, `include/fileProcessor.h`):
- Parses SDRTrunk MP3 filenames to extract metadata (date, time, talkgroup ID/name, radio ID)
- Validates file duration via libmpg123 (`src/MP3Duration.cpp`)
- Manages file movement and organization based on talkgroup

**Transcription Processing** (`src/transcriptionProcessor.cpp`, `include/transcriptionProcessor.h`):
- Handles OpenAI API transcription via whisper model
- Integrates with faster-whisper for local transcription (via pybind11 Python embedding)
- Searches transcriptions for tencodes/signals/callsigns and appends translations

**Database Management** (`src/DatabaseManager.cpp`, `include/DatabaseManager.h`):
- SQLite3 database operations for storing file metadata and transcriptions
- WAL journal mode with `busy_timeout=5000` for concurrent access
- Thread-safe writes via `std::mutex`
- Schema with `id INTEGER PRIMARY KEY AUTOINCREMENT`, `filename UNIQUE`, `duration REAL`
- Indexes on `talkgroup_id`, `unixtime`, `filename`
- `INSERT OR IGNORE` to skip duplicate filenames
- Automatic schema migration from old to new format

**API Communication** (`src/curlHelper.cpp`, `include/curlHelper.h`):
- CURL wrapper for OpenAI API requests
- Rate limiting (MAX_REQUESTS_PER_MINUTE)
- Error handling with retry logic (MAX_RETRIES)
- Error threshold monitoring (25% errors in ERROR_WINDOW_SECONDS triggers exit)
- Optional per-talkgroup prompt passed to Whisper API

**Thread Pool** (`include/ThreadPool.h`):
- Header-only thread pool using `std::jthread`, `std::condition_variable`, task queue
- Used for parallel file processing when `-p`/`--parallel` flag is set
- Pool size controlled by `MAX_THREADS` config option

**Local Transcription** (`src/fasterWhisper.cpp`, `include/fasterWhisper.h`):
- Integration with faster-whisper via pybind11 Python embedding
- Falls back to subprocess execution if pybind11 unavailable
- Used when --local flag is set

### In-House Parsers (replaced yaml-cpp, nlohmann/json, CLI11)

- **YAML Parser** (`src/yamlParser.cpp`, `include/yamlParser.h`): Config file loading
- **JSON Parser** (`src/jsonParser.cpp`, `include/jsonParser.h`): Glossary/API response parsing; supports both flat format and new multi-key `GLOSSARY` array format
- **Command Line Parser** (`src/commandLineParser.cpp`, `include/commandLineParser.h`): CLI argument handling

### Type Safety

**Strong Types** (`include/DomainTypes.h`):
- `StrongType<T, Tag>` template with `.get()` accessor
- Domain aliases: `TalkgroupId`, `RadioId`, `Duration`, `FilePath`, `Transcription`
- Prevents type-confusion bugs (e.g., passing filename where filepath expected)

**Error Handling** (`include/Result.h`):
- `Result<T>` = `std::expected<T, Error>` with error codes and context
- Monadic helpers: `map()`, `andThen()`, `orElse()`, `onError()`
- `TRY` / `TRY_ASSIGN` macros for early-return on error

### Data Structure

**FileData** (`include/FileData.h`):
- Core data structure with strongly-typed members:
  - `TalkgroupId talkgroupID`, `RadioId radioID`
  - `Duration duration`, `FilePath filename`, `FilePath filepath`
  - `Transcription transcription`, `Transcription v2transcription`
  - `std::string date`, `std::string time`, `std::string talkgroupName`

### Configuration Schema

Key configuration parameters in `config.yaml`:
- `OPENAI_API_KEY`: API key for OpenAI transcription
- `DirectoryToMonitor`: Path to SDRTrunk recordings
- `DATABASE_PATH`: SQLite database location
- `TALKGROUP_FILES`: Glossary mappings and optional prompt per talkgroup
- `MIN_DURATION_SECONDS`: Minimum file duration to process
- `LoopWaitSeconds`: Polling frequency (milliseconds)
- `MAX_THREADS`: Thread pool size for parallel processing (default 1)
- Debug flags for each component

#### Talkgroup Configuration Format
```yaml
TALKGROUP_FILES:
  52198-52250:
    GLOSSARY: ["ncshp.json"]
    PROMPT: "Police radio dispatch, North Carolina State Highway Patrol."
  28513,41003:
    GLOSSARY: ["fire.json"]
```

#### Glossary File Formats

**Old flat format** (still supported):
```json
{"10-4": "AFFIRMATIVE", "officer": "police officer"}
```

**New multi-key format**:
```json
{
    "GLOSSARY": [
        {"keys": ["10-4", "104"], "value": "AFFIRMATIVE"},
        {"keys": ["officer", "ofc"], "value": "police officer"}
    ]
}
```

Keys containing hyphens automatically generate stripped variants (e.g., `10-4` also matches `104`).

### External Dependencies

- **SQLite3**: Database operations
- **CURL**: HTTP requests to OpenAI API
- **libmpg123**: Native MP3 duration extraction (replaced ffprobe subprocess)
- **pybind11** (optional, fetched via CMake FetchContent): Python embedding for faster-whisper
- **Google Test**: Unit testing framework

### Debugging

Enable debug output for specific components in config.yaml:
- `DEBUG_CURL_HELPER`
- `DEBUG_DATABASE_MANAGER`
- `DEBUG_FILE_PROCESSOR`
- `DEBUG_MAIN`
- `DEBUG_TRANSCRIPTION_PROCESSOR`

## Important Notes

- The project expects SDRTrunk MP3 files with specific naming convention containing metadata
- Filename format: `{date}_{time}{talkgroupName}__TO_{tgID}_FROM_{radioID}.mp3`
- Glossary files support both flat JSON format and new multi-key `GLOSSARY` array format
- Tests are enabled by default (`BUILD_TESTS=ON`), 138 tests covering unit, integration, edge cases, and performance
- The system service installation script (`install-systemd-service.sh`) is untested
- Windows support is experimental and not fully tested
