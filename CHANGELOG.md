# Changelog

All notable changes to the C++ SDRTrunk Transcriber project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- Comprehensive documentation system
- Contributing guidelines and development setup instructions
- API documentation with class and function references
- Build system documentation with troubleshooting guides
- GitHub issue and pull request templates
- Doxygen configuration for automated documentation generation

### Changed
- Enhanced README.md with detailed installation and usage instructions
- Improved project structure documentation
- Updated CI/CD workflow badges and status indicators

## [1.0.0] - 2024-03-01

### Added
- Initial release of C++ SDRTrunk Transcriber
- Directory monitoring for SDRTrunk P25 MP3 files
- Dual transcription modes: OpenAI API and local faster-whisper
- SQLite3 database for storing transcriptions and metadata
- Automatic filename parsing for SDRTrunk file format
- Talkgroup-specific glossary mapping for tencodes and signals
- Rate limiting and error handling for API requests
- YAML-based configuration system
- Cross-platform support (Linux primary, Windows experimental)
- Command-line interface with CLI11
- Debug logging for all major components
- Minimum duration filtering to skip brief recordings
- Automatic translation of tencodes, signals, and callsigns

### Technical Features
- Modern CMake build system with dependency management
- C++17 codebase with RAII and smart pointer usage
- Singleton pattern for configuration management
- Comprehensive error handling with retry logic
- Memory-safe string handling and resource management
- External dependency integration (yaml-cpp, CLI11, nlohmann/json)

### Dependencies
- CMake 3.16+
- C++17 compatible compiler
- FFmpeg for audio processing
- SQLite3 for database operations
- libcurl for HTTP requests
- yaml-cpp for configuration parsing
- CLI11 for command-line argument parsing
- nlohmann/json for JSON processing

### Configuration Options
- OpenAI API key management
- Directory monitoring configuration
- Database path specification
- Polling frequency adjustment
- Rate limiting parameters
- Error threshold settings
- Debug output controls
- Talkgroup-specific glossary files
- Minimum duration thresholds

### Platform Support
- Linux: Full support with package manager integration
- Windows: Experimental support with vcpkg dependency management
- macOS: Homebrew-based dependency installation

### Integration
- SDRTrunk P25 recording filename format support
- faster-whisper Python script integration
- OpenAI Whisper API compatibility
- sdrtrunk-transcribed-web project compatibility
- Systemd service template for Linux

---

## Version History Notes

### Versioning Strategy
- **Major version**: Breaking changes, significant architecture updates
- **Minor version**: New features, enhancements, non-breaking changes
- **Patch version**: Bug fixes, security updates, documentation improvements

### Release Schedule
- Major releases: As needed for significant features or breaking changes
- Minor releases: Monthly for new features and enhancements
- Patch releases: As needed for critical bugs and security issues

### Backward Compatibility
- Configuration file format stability within major versions
- API compatibility for integration points
- Database schema migration support for upgrades

### Deprecation Policy
- Features marked deprecated in minor releases
- Deprecated features removed in next major release
- Minimum 6 months notice for breaking changes

### Security Updates
- Security patches released as soon as possible
- CVE tracking for all dependencies
- Regular dependency updates in patch releases