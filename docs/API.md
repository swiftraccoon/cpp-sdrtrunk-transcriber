# API Documentation

This document provides API documentation for the C++ SDRTrunk Transcriber project, including class interfaces, function signatures, and integration points.

## Table of Contents

- [Core Classes](#core-classes)
  - [ConfigSingleton](#configsingleton)
  - [DatabaseManager](#databasemanager)
  - [FileData](#filedata)
- [Processing Components](#processing-components)
  - [File Processor](#file-processor)
  - [Transcription Processor](#transcription-processor)
  - [CURL Helper](#curl-helper)
  - [Faster Whisper Integration](#faster-whisper-integration)
- [Utility Functions](#utility-functions)
  - [Debug Utils](#debug-utils)
  - [Global Flags](#global-flags)
- [Configuration Schema](#configuration-schema)
- [Integration Points](#integration-points)
- [Error Handling](#error-handling)

## Core Classes

### ConfigSingleton

**File**: `include/ConfigSingleton.h`, `src/ConfigSingleton.cpp`

Singleton class providing centralized configuration management throughout the application.

#### Public Methods

```cpp
static ConfigSingleton& getInstance();
void initialize(const YamlNode& config);
```

**Configuration Getters:**
```cpp
std::string getOpenAIAPIKey() const;
const std::unordered_map<int, TalkgroupFiles>& getTalkgroupFiles() const;
std::string getDatabasePath() const;
std::string getDirectoryToMonitor() const;
int getLoopWaitSeconds() const;
int getMaxRetries() const;
int getMaxRequestsPerMinute() const;
int getErrorWindowSeconds() const;
int getRateLimitWindowSeconds() const;
int getMinDurationSeconds() const;
int getMaxThreads() const;
```

**Debug Flag Getters:**
```cpp
bool isDebugCurlHelper() const;
bool isDebugDatabaseManager() const;
bool isDebugFileProcessor() const;
bool isDebugMain() const;
bool isDebugTranscriptionProcessor() const;
```

#### Usage Example

```cpp
#include "ConfigSingleton.h"
#include "yamlParser.h"

// Load and initialize configuration
YamlNode config = YamlNode::fromFile("config.yaml");
ConfigSingleton::getInstance().initialize(config);

// Access configuration values
auto& cfg = ConfigSingleton::getInstance();
std::string apiKey = cfg.getOpenAIAPIKey();
int waitSeconds = cfg.getLoopWaitSeconds();
```

#### Thread Safety

The ConfigSingleton is **not thread-safe**. Initialize once during application startup before creating multiple threads.

### DatabaseManager

**File**: `include/DatabaseManager.h`, `src/DatabaseManager.cpp`

Manages SQLite3 database operations for storing file metadata and transcriptions.

#### Constructor/Destructor

```cpp
DatabaseManager(const std::string& dbPath);
~DatabaseManager();
```

#### Public Methods

```cpp
void createTable();
void insertRecording(const std::string& date,
                    const std::string& time,
                    int64_t unixtime,
                    int talkgroupID,
                    const std::string& talkgroupName,
                    int radioID,
                    double duration,
                    const std::string& filename,
                    const std::string& filepath,
                    const std::string& transcription,
                    const std::string& v2transcription);
```

#### Private Methods

```cpp
void migrateSchema();  // Automatic old-to-new schema migration
```

#### Database Schema

```sql
CREATE TABLE IF NOT EXISTS recordings (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    date TEXT NOT NULL,
    time TEXT NOT NULL,
    unixtime INTEGER NOT NULL,
    talkgroup_id INTEGER NOT NULL,
    talkgroup_name TEXT NOT NULL DEFAULT '',
    radio_id INTEGER NOT NULL,
    duration REAL NOT NULL DEFAULT 0.0,
    filename TEXT NOT NULL UNIQUE,
    filepath TEXT NOT NULL,
    transcription TEXT NOT NULL DEFAULT '',
    v2transcription TEXT NOT NULL DEFAULT ''
);

-- Indexes
CREATE INDEX IF NOT EXISTS idx_recordings_talkgroup_id ON recordings(talkgroup_id);
CREATE INDEX IF NOT EXISTS idx_recordings_unixtime ON recordings(unixtime);
CREATE INDEX IF NOT EXISTS idx_recordings_filename ON recordings(filename);
```

#### Database Configuration

On connection, the following pragmas are set:
- `PRAGMA journal_mode=WAL` — concurrent reads during writes
- `PRAGMA synchronous=NORMAL` — balanced durability/performance
- `PRAGMA busy_timeout=5000` — retry for 5s before SQLITE_BUSY

#### Usage Example

```cpp
#include "DatabaseManager.h"

DatabaseManager db("/path/to/recordings.db");
db.createTable();  // Also runs migrateSchema() for old databases

db.insertRecording("20240301", "143022", 1709298622, 52197,
                  "Police Dispatch", 123, 15.0,
                  "recording.mp3", "/path/to/recording.mp3",
                  "Unit 123 responding to call",
                  "Unit 123 responding to call [ENHANCED]");
// Duplicate filenames are silently ignored (INSERT OR IGNORE)
```

#### Thread Safety

- `insertRecording()` is protected by `std::mutex` for thread-safe writes
- WAL mode enables concurrent reads from other connections

#### Error Handling

- Throws `std::runtime_error` on database connection failures
- Logs errors to stderr on SQL execution failures

### FileData

**File**: `include/FileData.h`

Core data structure containing all metadata for processed audio files. Uses strong types from `DomainTypes.h` for type safety.

#### Structure Definition

```cpp
#include "DomainTypes.h"

using sdrtrunk::domain::TalkgroupId;
using sdrtrunk::domain::RadioId;

struct FileData {
    std::string date;                              // Date string (YYYYMMDD)
    std::string time;                              // Time string (HHMMSS)
    std::chrono::system_clock::time_point timestamp; // Parsed timestamp
    TalkgroupId talkgroupID;                       // P25 talkgroup (strong type, .get() for int)
    std::string talkgroupName;                     // Human-readable talkgroup name
    RadioId radioID;                               // P25 radio ID (strong type, .get() for int)
    std::chrono::seconds duration;                 // Audio duration
    std::filesystem::path filename;                // Original filename
    std::filesystem::path filepath;                // Full path to file
    std::string transcription;                     // Primary transcription text
    std::string v2transcription;                   // Enhanced transcription with translations

    FileData() : talkgroupID(0), radioID(0), duration(0) {}

    int64_t unixtime() const;  // Unix epoch seconds from timestamp
};
```

#### Strong Types

`TalkgroupId` and `RadioId` are `StrongType<int, Tag>` wrappers defined in `include/DomainTypes.h`. Access the underlying value with `.get()`.

```cpp
FileData data;
data.talkgroupID = TalkgroupId(52198);
int rawId = data.talkgroupID.get();  // 52198
```

#### SDRTrunk Filename Format

The application expects SDRTrunk filenames in the format:
```
YYYYMMDD_HHMMSSSystem_Name__TO_TALKGROUPID_FROM_RADIOID.mp3
```

Example: `20240115_143045North_Carolina_VIPER_Rutherford_T-SPDControl__TO_52324_FROM_2097268.mp3`

## Processing Components

### File Processor

**File**: `include/fileProcessor.h`, `src/fileProcessor.cpp`

Handles parsing and validation of SDRTrunk audio files. Uses libmpg123 for MP3 duration extraction.

#### Key Functions

```cpp
// Process a file through the full pipeline (validate, transcribe, extract info, save, move)
FileData processFile(const std::filesystem::path& path,
                    const std::string& directoryToMonitor,
                    const std::string& OPENAI_API_KEY);

// Find MP3 files without corresponding TXT and organize them
void find_and_move_mp3_without_txt(const std::string& directoryToMonitor);

// Check if a file is still being written to
bool isFileBeingWrittenTo(const std::string& filePath);

// Check if a file has a .lock companion
bool isFileLocked(const std::string& filePath);

// Extract metadata from filename and populate FileData
void extractFileInfo(FileData& fileData, const std::string& filename,
                    const std::string& transcription);
```

#### Internal Functions (not in header)

```cpp
// Get MP3 duration using libmpg123
std::string getMP3Duration(const std::string& mp3FilePath);

// Generate unix timestamp from date/time strings
int64_t generateUnixTimestamp(const std::string& date, const std::string& time);

// Validate file duration meets minimum threshold
float validateDuration(const std::string& file_path, FileData& fileData);
```

### Transcription Processor

**File**: `include/transcriptionProcessor.h`, `src/transcriptionProcessor.cpp`

Handles transcription processing and glossary mapping.

#### Core Structures

```cpp
struct TalkgroupFiles {
    std::vector<std::string> glossaryFiles;
    std::string prompt;  // Optional per-talkgroup Whisper API prompt
};

struct GlossaryEntry {  // Defined in jsonParser.h
    std::vector<std::string> keys;
    std::string value;
};
```

#### Key Functions

```cpp
// Read JSON glossary mapping file
std::unordered_map<std::string, std::string> readMappingFile(const std::string& filePath);

// Extract actual transcription text from API response
std::string extractActualTranscription(const std::string& transcription);

// Generate enhanced transcription with glossary mappings
std::string generateV2Transcription(const std::string& transcription,
                                   int talkgroupID,
                                   int radioID,
                                   const std::unordered_map<int, TalkgroupFiles>& talkgroupFiles);

// Parse talkgroup ID ranges and individual IDs (e.g. "52197-52201,28513")
std::unordered_set<int> parseTalkgroupIDs(const std::string& idString);
```

#### Glossary File Formats

**Old flat format** (still supported):
```json
{
  "10-4": "Acknowledgment",
  "10-20": "Location",
  "Code 3": "Emergency Response",
  "Signal 7": "Out of Service"
}
```

**New multi-key format** (allows aliases for each term):
```json
{
    "GLOSSARY": [
        {"keys": ["10-4", "104"], "value": "Acknowledgment"},
        {"keys": ["10-20", "1020"], "value": "Location"},
        {"keys": ["Code 3"], "value": "Emergency Response"}
    ]
}
```

Keys containing hyphens automatically generate stripped variants (e.g., `10-4` also matches `104`). This applies to both formats.

### CURL Helper

**File**: `include/curlHelper.h`, `src/curlHelper.cpp`

Manages HTTP requests to OpenAI API with rate limiting and error handling.

#### Key Functions

```cpp
// CURL response write callback
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);

// Setup CURL headers with API key
void setupCurlHeaders(CURL* curl, struct curl_slist*& headers, const std::string& OPENAI_API_KEY);

// Setup CURL multipart form for audio upload (prompt is optional per-talkgroup context)
void setupCurlPostFields(CURL* curl, curl_mime*& mime, const std::string& file_path, const std::string& prompt = "");

// Execute CURL request and return response
std::string makeCurlRequest(CURL* curl, curl_mime* mime);

// Transcribe audio via OpenAI Whisper API (prompt is optional per-talkgroup context)
std::string curl_transcribe_audio(const std::string& file_path, const std::string& OPENAI_API_KEY, const std::string& prompt = "");
```

#### Rate Limiting

- Tracks requests per minute based on configuration
- Implements sliding window rate limiting
- Monitors error rates and exits if threshold exceeded (25% errors in ERROR_WINDOW_SECONDS)

### Faster Whisper Integration

**File**: `include/fasterWhisper.h`, `src/fasterWhisper.cpp`

Local transcription using faster-whisper via pybind11 Python embedding.

#### Key Functions

```cpp
// Execute local transcription using faster-whisper (returns expected<string, string>)
std::expected<std::string, std::string> local_transcribe_audio(const std::string& mp3FilePath);

// Clean up Python interpreter on exit
void cleanup_python();
```

#### Requirements

- Built with `USE_LOCAL_TRANSCRIPTION=ON` (default)
- `fasterWhisper.py` script must be accessible
- Uses pybind11 to embed Python interpreter
- faster-whisper Python package must be installed

### Thread Pool

**File**: `include/ThreadPool.h`

Header-only thread pool for parallel file processing using `std::jthread`.

#### Public Methods

```cpp
explicit ThreadPool(size_t numThreads);
~ThreadPool();

template<typename F, typename... Args>
auto enqueue(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>>;
```

#### Usage Example

```cpp
#include "ThreadPool.h"

ThreadPool pool(4);  // 4 worker threads

auto future = pool.enqueue([](const std::string& file) {
    return processFile(file, monitorDir, apiKey);
}, filePath);

FileData result = future.get();
```

#### Thread Safety

- Task submission is thread-safe
- Worker threads automatically join on destruction via `std::jthread`

## Utility Functions

### Debug Utils

**File**: `include/debugUtils.h`, `src/debugUtils.cpp`

```cpp
// Get current time as formatted string for log output
std::string getCurrentTime();
```

### Global Flags

**File**: `include/globalFlags.h`

```cpp
extern bool gLocalFlag;     // When true, use local faster-whisper instead of OpenAI API
extern bool gParallelFlag;  // When true, use thread pool for parallel file processing
```

## Configuration Schema

Configuration uses an in-house YAML parser (`include/yamlParser.h`).

### Required Configuration

```yaml
DirectoryToMonitor: "/path/to/recordings"
DATABASE_PATH: "/path/to/database.db"
LoopWaitSeconds: 200
MIN_DURATION_SECONDS: 9

# OpenAI API settings (if not using --local mode)
OPENAI_API_KEY: "your_api_key"
MAX_REQUESTS_PER_MINUTE: 50
MAX_RETRIES: 3
```

### Optional Configuration

```yaml
# Rate limiting
ERROR_WINDOW_SECONDS: 300
RATE_LIMIT_WINDOW_SECONDS: 60

# Thread pool for parallel processing (used with -p flag)
MAX_THREADS: 4

# Talkgroup glossaries and prompts (ranges and individual IDs supported)
TALKGROUP_FILES:
  52197-52201:
    GLOSSARY: ["/path/to/glossary1.json"]
    PROMPT: "Police radio dispatch, North Carolina State Highway Patrol."
  28513,41003:
    GLOSSARY: ["/path/to/glossary2.json"]

# Debug flags
DEBUG_MAIN: false
DEBUG_FILE_PROCESSOR: false
DEBUG_DATABASE_MANAGER: false
DEBUG_CURL_HELPER: false
DEBUG_TRANSCRIPTION_PROCESSOR: false
```

## Integration Points

### External Systems

**SDRTrunk Integration:**
- Monitors SDRTrunk recording output directory
- Parses SDRTrunk filename format automatically
- Compatible with standard SDRTrunk P25 configurations

**OpenAI API Integration:**
- Uses Whisper model for transcription
- Implements proper error handling and rate limiting

**faster-whisper Integration:**
- Embeds Python interpreter via pybind11
- Uses faster-whisper with large-v3 model
- Configured for radio traffic (high noise threshold, specific VAD parameters)

### Web Interface Integration

**sdrtrunk-transcribed-web Compatibility:**
- Database schema compatible with web interface
- File organization matches expected structure

## Error Handling

### Exception Types

**Configuration Errors:**
- `std::runtime_error`: Invalid configuration file or missing required fields

**Database Errors:**
- `std::runtime_error`: Database connection failure
- SQL errors logged to stderr

**File Processing Errors:**
- Returns empty/default FileData for invalid files
- Skips files below minimum duration
- Logs warnings when debug mode enabled

**Network Errors:**
- Automatic retry with configurable MAX_RETRIES
- Rate limit handling with sliding window
- Error threshold monitoring (exits if >25% errors)

**Local Transcription Errors:**
- Returns `std::expected` with error string on failure
- Python interpreter initialized once, cleaned up on exit
