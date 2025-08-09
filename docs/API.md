# API Documentation

This document provides comprehensive API documentation for the C++ SDRTrunk Transcriber project, including class interfaces, function signatures, and integration points.

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
void initialize(const YAML::Node& config);
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
#include <yaml-cpp/yaml.h>

// Initialize configuration
YAML::Node config = YAML::LoadFile("config.yaml");
ConfigSingleton::getInstance().initialize(config);

// Access configuration values
auto& config = ConfigSingleton::getInstance();
std::string apiKey = config.getOpenAIAPIKey();
int waitSeconds = config.getLoopWaitSeconds();
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
                    int unixtime, 
                    int talkgroupID, 
                    const std::string& talkgroupName, 
                    int radioID, 
                    const std::string& duration, 
                    const std::string& filename, 
                    const std::string& filepath, 
                    const std::string& transcription, 
                    const std::string& v2transcription);
```

#### Database Schema

```sql
CREATE TABLE IF NOT EXISTS recordings (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    date TEXT NOT NULL,
    time TEXT NOT NULL,
    unixtime INTEGER NOT NULL,
    talkgroupID INTEGER NOT NULL,
    talkgroupName TEXT,
    radioID INTEGER NOT NULL,
    duration TEXT NOT NULL,
    filename TEXT NOT NULL,
    filepath TEXT NOT NULL,
    transcription TEXT,
    v2transcription TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
```

#### Usage Example

```cpp
#include "DatabaseManager.h"

// Initialize database
DatabaseManager db("/path/to/recordings.db");
db.createTable();

// Insert recording
db.insertRecording("2024-03-01", "14:30:22", 1709298622, 52197, 
                  "Police Dispatch", 123, "00:15", 
                  "recording.mp3", "/path/to/recording.mp3", 
                  "Unit 123 responding to call", 
                  "Unit 123 responding to call [ENHANCED]");
```

#### Error Handling

- Throws `std::runtime_error` on database connection failures
- Throws `std::runtime_error` on SQL execution errors
- Automatic database creation if file doesn't exist

### FileData

**File**: `include/FileData.h`

Core data structure containing all metadata for processed audio files.

#### Structure Definition

```cpp
struct FileData {
    std::string date;           // Date in YYYY-MM-DD format
    std::string time;           // Time in HH:MM:SS format
    int unixtime;              // Unix timestamp
    int talkgroupID;           // P25 talkgroup identifier
    std::string talkgroupName; // Human-readable talkgroup name
    int radioID;               // P25 radio identifier
    std::string duration;      // Audio duration (MM:SS format)
    std::string filename;      // Original filename
    std::string filepath;      // Full path to file
    std::string transcription; // Primary transcription text
    std::string v2transcription; // Enhanced transcription with translations
};
```

#### SDRTrunk Filename Format

The application expects SDRTrunk filenames in the format:
```
YYYYMMDD_HHMMSS_TALKGROUPID_RADIOID.mp3
```

Example: `20240301_143022_52197_123.mp3`
- Date: 2024-03-01
- Time: 14:30:22
- Talkgroup: 52197
- Radio: 123

## Processing Components

### File Processor

**File**: `include/fileProcessor.h`, `src/fileProcessor.cpp`

Handles parsing and validation of SDRTrunk audio files.

#### Key Functions

```cpp
// Parse SDRTrunk filename into FileData structure
FileData parseSDRTrunkFilename(const std::string& filename);

// Validate file meets minimum duration requirements
bool validateFileDuration(const std::string& filepath, int minDurationSeconds);

// Extract audio duration using FFmpeg
std::string getAudioDuration(const std::string& filepath);
```

#### Usage Example

```cpp
#include "fileProcessor.h"

// Parse filename
std::string filename = "20240301_143022_52197_123.mp3";
FileData fileData = parseSDRTrunkFilename(filename);

// Validate duration
if (validateFileDuration(filepath, 9)) {
    // Process file
    std::string duration = getAudioDuration(filepath);
}
```

#### Error Handling

- Returns empty FileData for invalid filenames
- Returns false for duration validation failures
- Logs warnings for parsing errors when debug enabled

### Transcription Processor

**File**: `include/transcriptionProcessor.h`, `src/transcriptionProcessor.cpp`

Handles transcription processing and glossary mapping.

#### Core Structures

```cpp
struct TalkgroupFiles {
    std::vector<std::string> glossaryFiles;
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

// Parse talkgroup ID ranges and individual IDs
std::unordered_set<int> parseTalkgroupIDs(const std::string& idString);

// Read talkgroup file mappings from configuration
std::unordered_map<int, TalkgroupFiles> readTalkgroupFileMappings(const std::string& configFilePath);
```

#### Glossary File Format

Glossary files should be JSON format:
```json
{
  "10-4": "Acknowledgment",
  "10-20": "Location",
  "Code 3": "Emergency Response",
  "Signal 7": "Out of Service"
}
```

#### Usage Example

```cpp
#include "transcriptionProcessor.h"

// Load glossary mappings
auto mappings = readMappingFile("/path/to/tencode_glossary.json");

// Generate enhanced transcription
std::string enhanced = generateV2Transcription(
    "Unit responding Code 3 to location",
    52197,
    123,
    talkgroupFiles
);
// Result: "Unit responding Code 3 [Emergency Response] to location"
```

### CURL Helper

**File**: `include/curlHelper.h`, `src/curlHelper.cpp`

Manages HTTP requests to OpenAI API with rate limiting and error handling.

#### Key Functions

```cpp
// Send transcription request to OpenAI API
std::string sendTranscriptionRequest(const std::string& audioFilePath,
                                    const std::string& apiKey);

// Initialize CURL and set up global options
void initializeCurl();

// Cleanup CURL resources
void cleanupCurl();
```

#### Rate Limiting

- Tracks requests per minute based on configuration
- Implements exponential backoff for rate limit errors
- Monitors error rates and exits if threshold exceeded

#### Error Handling

- Retries failed requests up to MAX_RETRIES
- Handles network timeouts and connection errors
- Parses API error responses and provides appropriate feedback

#### Usage Example

```cpp
#include "curlHelper.h"

// Initialize (call once at startup)
initializeCurl();

// Send transcription request
std::string result = sendTranscriptionRequest(
    "/path/to/audio.mp3",
    "your-openai-api-key"
);

// Cleanup (call once at shutdown)
cleanupCurl();
```

### Faster Whisper Integration

**File**: `include/fasterWhisper.h`, `src/fasterWhisper.cpp`

Integrates with local faster-whisper Python script for offline transcription.

#### Key Functions

```cpp
// Execute local transcription using faster-whisper
std::string transcribeWithFasterWhisper(const std::string& audioFilePath);

// Check if faster-whisper script is available
bool isFasterWhisperAvailable();
```

#### Python Script Requirements

- `fasterWhisper.py` must be in the same directory as the binary
- Script must accept audio file path as command line argument
- Script must output JSON with transcription result

#### Usage Example

```cpp
#include "fasterWhisper.h"

// Check availability
if (isFasterWhisperAvailable()) {
    std::string result = transcribeWithFasterWhisper("/path/to/audio.mp3");
}
```

## Utility Functions

### Debug Utils

**File**: `include/debugUtils.h`, `src/debugUtils.cpp`

Provides debug logging functionality throughout the application.

```cpp
// Debug logging with component filtering
void debugLog(const std::string& component, const std::string& message);

// Check if debug is enabled for component
bool isDebugEnabled(const std::string& component);
```

### Global Flags

**File**: `include/globalFlags.h`

Defines global application flags and constants.

```cpp
extern bool g_useLocalTranscription;
extern bool g_debugMode;
```

## Configuration Schema

### Required Configuration

```yaml
# Core settings
DirectoryToMonitor: "/path/to/recordings"
DATABASE_PATH: "/path/to/database.db"
LoopWaitSeconds: 200
MIN_DURATION_SECONDS: 9

# OpenAI API settings (if not using local mode)
OPENAI_API_KEY: "your_api_key"
MAX_REQUESTS_PER_MINUTE: 50
MAX_RETRIES: 3
```

### Optional Configuration

```yaml
# Rate limiting
ERROR_WINDOW_SECONDS: 300
RATE_LIMIT_WINDOW_SECONDS: 60

# Talkgroup glossaries
TALKGROUP_FILES:
  52197-52201:
    GLOSSARY: ["/path/to/glossary1.json"]
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
- Uses Whisper-1 model for transcription
- Supports all audio formats accepted by OpenAI
- Implements proper error handling and rate limiting

**faster-whisper Integration:**
- Executes Python script as subprocess
- Configurable model parameters in Python script
- JSON-based communication protocol

### Web Interface Integration

**sdrtrunk-transcribed-web Compatibility:**
- Database schema compatible with web interface
- File organization matches expected structure
- Provides sync scripts for file deployment

### System Integration

**Systemd Service:**
- Template service file provided
- Automatic restart on failure
- Logging integration with systemd journal

**Docker Support:**
- Containerized deployment options
- Environment variable configuration
- Volume mounting for data persistence

## Error Handling

### Exception Types

**Configuration Errors:**
- `std::runtime_error`: Invalid configuration file
- `std::invalid_argument`: Missing required configuration

**Database Errors:**
- `std::runtime_error`: Database connection failure
- `std::runtime_error`: SQL execution error

**File Processing Errors:**
- Return empty/default values for parsing failures
- Log warnings when debug mode enabled
- Skip invalid files and continue processing

**Network Errors:**
- Automatic retry with exponential backoff
- Rate limit handling with appropriate delays
- Graceful degradation when API unavailable

### Error Recovery

**Transient Errors:**
- Network timeouts: Automatic retry
- API rate limits: Wait and retry
- File locks: Retry after delay

**Persistent Errors:**
- Invalid configuration: Exit with error code
- Database corruption: Log error and exit
- Missing dependencies: Exit with helpful message

### Logging

**Log Levels:**
- ERROR: Critical errors requiring attention
- WARN: Non-fatal issues that should be monitored
- INFO: General operational information
- DEBUG: Detailed diagnostic information

**Log Destinations:**
- Console output (stdout/stderr)
- System log (syslog/journald when available)
- File output (configurable)

For implementation details and examples, see the source code in the respective files.