# C++ SDRTrunk Transcriber

[![Build Status](https://github.com/swiftraccoon/cpp-sdrtrunk-transcriber/actions/workflows/cmake-ubuntu.yml/badge.svg)](https://github.com/swiftraccoon/cpp-sdrtrunk-transcriber/actions/workflows/cmake-ubuntu.yml)
[![Windows Build](https://github.com/swiftraccoon/cpp-sdrtrunk-transcriber/actions/workflows/cmake-windows.yml/badge.svg)](https://github.com/swiftraccoon/cpp-sdrtrunk-transcriber/actions/workflows/cmake-windows.yml)
[![Security Scan](https://github.com/swiftraccoon/cpp-sdrtrunk-transcriber/actions/workflows/snyk-security.yml/badge.svg)](https://github.com/swiftraccoon/cpp-sdrtrunk-transcriber/actions/workflows/snyk-security.yml)
[![CodeQL](https://github.com/swiftraccoon/cpp-sdrtrunk-transcriber/actions/workflows/ubuntu-codeql.yml/badge.svg)](https://github.com/swiftraccoon/cpp-sdrtrunk-transcriber/actions/workflows/ubuntu-codeql.yml)
[![Code Quality](https://www.codefactor.io/repository/github/swiftraccoon/cpp-sdrtrunk-transcriber/badge)](https://www.codefactor.io/repository/github/swiftraccoon/cpp-sdrtrunk-transcriber)

[![Latest Release](https://img.shields.io/github/tag/swiftraccoon/cpp-sdrtrunk-transcriber?include_prereleases=&sort=semver&color=blue&label=version)](https://github.com/swiftraccoon/cpp-sdrtrunk-transcriber/releases/)
[![License: GPL v3](https://img.shields.io/badge/License-GPL_v3-blue.svg)](LICENSE)
[![Issues](https://img.shields.io/github/issues/swiftraccoon/cpp-sdrtrunk-transcriber)](https://github.com/swiftraccoon/cpp-sdrtrunk-transcriber/issues)
[![Pull Requests](https://img.shields.io/github/issues-pr/swiftraccoon/cpp-sdrtrunk-transcriber)](https://github.com/swiftraccoon/cpp-sdrtrunk-transcriber/pulls)
[![Contributors](https://img.shields.io/github/contributors/swiftraccoon/cpp-sdrtrunk-transcriber)](https://github.com/swiftraccoon/cpp-sdrtrunk-transcriber/graphs/contributors)

A C++ application designed to monitor directories for SDRTrunk P25 MP3 recordings, transcribe them using either local [faster-whisper](https://github.com/SYSTRAN/faster-whisper) or OpenAI's API, and organize the results with talkgroup categorization and terminology translation.

## Features at a Glance

- **Dual Transcription Modes**: Local processing with faster-whisper or cloud-based with OpenAI API
- **Per-Talkgroup Prompts**: Optional Whisper API prompt per talkgroup for improved accuracy
- **Parallel Processing**: Thread pool with configurable `MAX_THREADS` for concurrent file processing
- **File Processing**: Parsing of SDRTrunk filename metadata
- **Database Management**: SQLite3 with WAL mode, indexes, thread-safe writes, and auto-migration
- **Terminology Translation**: Automatic tencode, signal, and callsign translation with multi-key glossary support
- **Cross-Platform**: Supports Linux and Windows (experimental)
- **Rate Limiting**: Built-in API rate limiting and error handling
- **Configurable**: Comprehensive YAML-based configuration system

## Related Projects
- [sdrtrunk-transcriber](https://github.com/swiftraccoon/sdrtrunk-transcriber) (Python version of this repo)
- [sdrtrunk-transcribed-web](https://github.com/swiftraccoon/sdrtrunk-transcribed-web) (Node.JS website for displaying mp3/txt files processed by this project)

## Table of Contents

- [Quick Start](#quick-start)
- [Features](#features)
- [Installation](#installation)
  - [Prerequisites](#prerequisites)
  - [Building from Source](#building-from-source)
  - [Platform-Specific Instructions](#platform-specific-instructions)
- [Configuration](#configuration)
- [Usage](#usage)
  - [Basic Usage](#basic-usage)
  - [Command Line Options](#command-line-options)
  - [Examples](#examples)
- [Transcription Modes](#transcription-modes)
  - [OpenAI API Mode](#openai-api-mode)
  - [Local Mode (faster-whisper)](#local-mode-faster-whisper)
- [System Integration](#system-integration)
  - [System Service](#system-service)
  - [Web Interface](#web-interface)
- [Documentation](#documentation)
- [Troubleshooting](#troubleshooting)
- [Contributing](#contributing)
- [License](#license)

## Quick Start

```bash
# 1. Install dependencies (Ubuntu/Debian)
sudo apt-get install libmpg123-dev libcurl4-openssl-dev libsqlite3-dev python3-dev pkg-config

# 2. Clone and build
git clone https://github.com/swiftraccoon/cpp-sdrtrunk-transcriber.git
cd cpp-sdrtrunk-transcriber
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# 3. Configure
cp sample-config.yaml config.yaml
# Edit config.yaml with your settings

# 4. Run
./build/sdrtrunk-transcriber
```

## Features

### Core Functionality
- **Directory Monitoring**: Continuously monitors specified directories for new SDRTrunk P25 MP3 files
- **Metadata Extraction**: Automatically parses filename metadata (timestamp, talkgroup ID, radio ID)
- **Duration Filtering**: Configurable minimum duration threshold to skip brief recordings
- **Dual Transcription**: Choose between OpenAI API or local faster-whisper processing

### Advanced Features
- **Per-Talkgroup Prompts**: Optional Whisper API prompt per talkgroup to improve transcription accuracy
- **Multi-Key Glossary**: New glossary format supporting multiple keys per entry and automatic hyphen-stripped matching
- **Parallel Processing**: Configurable thread pool (`MAX_THREADS`) with `-p` flag for concurrent file processing
- **Intelligent Translation**: Searches transcriptions for tencodes, signals, and callsigns with automatic translation lookup
- **Database Management**: SQLite3 with WAL mode, indexes, unique constraints, thread-safe writes, and automatic schema migration
- **Rate Limiting**: Built-in API rate limiting with configurable thresholds
- **Error Handling**: Robust retry logic with automatic failure recovery
- **Performance Monitoring**: Configurable debug output for all major components
- **Cross-Platform**: Native support for Linux, experimental Windows support

## Installation

### Prerequisites

**System Requirements:**
- C++23 compatible compiler (GCC 13+, Clang 19+, MSVC 2022+)
- CMake 3.16 or higher
- Git

**Core Dependencies:**
- libmpg123 (MP3 duration extraction)
- SQLite3 (database)
- libcurl (HTTP client)

### Platform-Specific Instructions

#### Ubuntu/Debian
```bash
sudo apt-get update
sudo apt-get install build-essential cmake git pkg-config \
    libmpg123-dev libcurl4-openssl-dev \
    libsqlite3-dev python3-dev
```

#### Fedora/RHEL/CentOS
```bash
sudo dnf install gcc-c++ cmake git pkg-config \
    mpg123-devel libcurl-devel \
    sqlite-devel python3-devel
```

#### macOS (via Homebrew)
```bash
brew install cmake mpg123 sqlite3 curl python3
```

#### Windows
For Windows users, we recommend using vcpkg for dependency management:
```powershell
# Install vcpkg if not already installed
git clone https://github.com/Microsoft/vcpkg.git
.\vcpkg\bootstrap-vcpkg.bat

# Install dependencies
.\vcpkg\vcpkg install curl sqlite3 mpg123 --triplet x64-windows
```

### Building from Source

1. **Clone the repository:**
   ```bash
   git clone https://github.com/swiftraccoon/cpp-sdrtrunk-transcriber.git
   cd cpp-sdrtrunk-transcriber
   ```

2. **Configure and build:**
   ```bash
   # Release build (recommended)
   cmake -B build -DCMAKE_BUILD_TYPE=Release
   cmake --build build --config Release
   
   # Debug build (for development)
   cmake -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
   cmake --build build --config Debug
   ```

3. **Install (optional):**
   ```bash
   sudo cmake --install build
   ```


## Configuration

The application uses a YAML configuration file for all settings. Start with the provided sample:

```bash
cp sample-config.yaml config.yaml
vim config.yaml  # or your preferred editor
```

### Essential Configuration

**Basic Setup:**
```yaml
# Directory containing SDRTrunk MP3 files
DirectoryToMonitor: "/path/to/sdrtrunk/recordings"

# SQLite database for storing transcriptions
DATABASE_PATH: "/path/to/recordings.db"

# Polling frequency in milliseconds
LoopWaitSeconds: 200

# Skip files shorter than this (seconds)
MIN_DURATION_SECONDS: 9
```

**OpenAI API Configuration:**
```yaml
OPENAI_API_KEY: "your_api_key_here"
MAX_REQUESTS_PER_MINUTE: 50
MAX_RETRIES: 3
ERROR_WINDOW_SECONDS: 300
RATE_LIMIT_WINDOW_SECONDS: 60
```

**Talkgroup-Specific Glossaries and Prompts:**
```yaml
TALKGROUP_FILES:
  52197-52201:  # Range of talkgroup IDs
    GLOSSARY:
      - "/path/to/tencode_glossary.json"
      - "/path/to/signals_glossary.json"
    PROMPT: "Police radio dispatch, North Carolina State Highway Patrol."
  28513,41003,41004:  # Specific talkgroup IDs
    GLOSSARY: ["/path/to/tencode_glossary.json"]

# Thread pool for parallel processing (used with -p flag)
MAX_THREADS: 4
```

See [docs/CONFIGURATION.md](docs/CONFIGURATION.md) for complete configuration reference.

## Usage

### Basic Usage

After building and configuring, run the transcriber:

```bash
# Default mode (OpenAI API)
./build/sdrtrunk-transcriber

# Local transcription mode
./build/sdrtrunk-transcriber --local

# Custom configuration file
./build/sdrtrunk-transcriber -c /path/to/custom-config.yaml
```

### Command Line Options

| Option | Description | Default |
|--------|-------------|----------|
| `-c, --config <path>` | Configuration file path | `./config.yaml` |
| `-l, --local` | Enable local transcription (faster-whisper) | Off (uses OpenAI API) |
| `-p, --parallel` | Enable parallel file processing (uses MAX_THREADS from config) | Off (single-threaded) |
| `-h, --help` | Display help message and exit | - |

### Examples

**Monitor with custom polling interval:**
```yaml
# In config.yaml
LoopWaitSeconds: 5000  # Check every 5 seconds
```

**Process only longer recordings:**
```yaml
# In config.yaml
MIN_DURATION_SECONDS: 30  # Skip files under 30 seconds
```

**Enable debug output:**
```yaml
# In config.yaml
DEBUG_MAIN: true
DEBUG_FILE_PROCESSOR: true
```

## Transcription Modes

### OpenAI API Mode

Default mode using OpenAI's Whisper API for transcription:

**Prerequisites:**
- OpenAI API key with Whisper access
- Internet connection

**Configuration:**
```yaml
OPENAI_API_KEY: "your_api_key_here"
MAX_REQUESTS_PER_MINUTE: 50
MAX_RETRIES: 3
RATE_LIMIT_WINDOW_SECONDS: 60
```

### Local Mode (faster-whisper)

Use local processing for offline transcription or enhanced privacy:

**Prerequisites:**
1. Install faster-whisper:
   ```bash
   pip install faster-whisper
   ```

2. Ensure `fasterWhisper.py` is in the same directory as the binary

3. GPU support (optional but recommended):
   ```bash
   # For NVIDIA GPU support
   pip install faster-whisper[gpu]
   ```

**Usage:**
```bash
./build/sdrtrunk-transcriber --local
```

**Benefits:**
- No API costs or rate limits
- Works offline
- Enhanced privacy (no data sent to third parties)
- Customizable model parameters

## System Integration

### System Service

Run as a systemd service for continuous operation:

1. **Edit the service template:**
   ```bash
   cp scripts/install-systemd-service.sh install-service.sh
   vim install-service.sh  # Update paths and user
   ```

2. **Install the service:**
   ```bash
   sudo ./install-service.sh
   ```

3. **Manage the service:**
   ```bash
   sudo systemctl start sdrtrunk-transcriber
   sudo systemctl enable sdrtrunk-transcriber
   sudo systemctl status sdrtrunk-transcriber
   ```

### Web Interface

Display processed recordings using [sdrtrunk-transcribed-web](https://github.com/swiftraccoon/sdrtrunk-transcribed-web):

1. **Set up the web interface:**
   ```bash
   git clone https://github.com/swiftraccoon/sdrtrunk-transcribed-web.git
   cd sdrtrunk-transcribed-web
   npm install
   ```

2. **Sync files automatically:**
   ```bash
   # Use the provided sync script
   cp scripts/rsync_local_to_server.sh sync-to-web.sh
   vim sync-to-web.sh  # Update paths
   ./sync-to-web.sh
   ```

3. **Or set up automated sync:**
   ```bash
   # Add to crontab for automatic syncing
   */5 * * * * /path/to/sync-to-web.sh
   ```

## Documentation

- **[API Documentation](docs/API.md)** - Classes, functions, and integration points
- **[Build Guide](docs/BUILD.md)** - Detailed build instructions and troubleshooting
- **[Configuration Reference](docs/CONFIGURATION.md)** - Complete configuration documentation
- **[Contributing Guide](CONTRIBUTING.md)** - Development setup and guidelines
- **[Changelog](CHANGELOG.md)** - Version history and changes

## Troubleshooting

### Common Issues

**Build Errors:**
- Ensure all dependencies are installed
- Try cleaning the build directory: `rm -rf build && mkdir build`
- Check CMake version: `cmake --version` (requires 3.16+)

**Runtime Issues:**
- Verify config.yaml syntax with `yamllint config.yaml`
- Check file permissions on monitored directory
- Enable debug output for detailed logging

**Transcription Problems:**
- For OpenAI API: verify API key and network connectivity
- For local mode: ensure fasterWhisper.py is in the correct location
- Check minimum duration settings

**Performance Issues:**
- Adjust polling frequency (LoopWaitSeconds)
- Consider local mode for high-volume processing
- Monitor system resources during operation

For more detailed troubleshooting, see [docs/BUILD.md#troubleshooting](docs/BUILD.md#troubleshooting).

### Getting Help

- **Issues**: [GitHub Issues](https://github.com/swiftraccoon/cpp-sdrtrunk-transcriber/issues)
- **Discussions**: [GitHub Discussions](https://github.com/swiftraccoon/cpp-sdrtrunk-transcriber/discussions)
- **Wiki**: [Project Wiki](https://github.com/swiftraccoon/cpp-sdrtrunk-transcriber/wiki)

## Contributing

We welcome contributions! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for:

- Development environment setup
- Code style guidelines
- Testing requirements
- Pull request process
- Issue reporting guidelines

### Quick Start for Contributors

```bash
# First, fork the repository on GitHub (click the Fork button on the repo page)
# Then clone YOUR fork (replace YOUR_USERNAME with your actual GitHub username)
git clone https://github.com/YOUR_USERNAME/cpp-sdrtrunk-transcriber.git
cd cpp-sdrtrunk-transcriber

# Add the original repository as upstream
git remote add upstream https://github.com/swiftraccoon/cpp-sdrtrunk-transcriber.git

# Set up development environment
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
cmake --build build

# Run tests
cd build && ctest
```

## License

This project is licensed under the GPL-3.0 license. See the [LICENSE](LICENSE) file for details.
