# Configuration Reference

Complete configuration guide for the C++ SDRTrunk Transcriber, including all options, examples, and best practices.

## Table of Contents

- [Quick Start](#quick-start)
- [Configuration File Format](#configuration-file-format)
- [Core Configuration](#core-configuration)
- [Transcription Settings](#transcription-settings)
- [Talkgroup Configuration](#talkgroup-configuration)
- [Performance Tuning](#performance-tuning)
- [Debug Configuration](#debug-configuration)
- [Environment Variables](#environment-variables)
- [Configuration Validation](#configuration-validation)
- [Examples](#examples)
- [Migration Guide](#migration-guide)

## Quick Start

```bash
# Copy sample configuration
cp sample-config.yaml config.yaml

# Edit with your settings
vim config.yaml

# Validate configuration
./build/sdrtrunk-transcriber --config config.yaml --help
```

## Configuration File Format

The application uses YAML format for configuration. The default configuration file is `config.yaml` in the current directory, but you can specify a custom path using the `-c` or `--config` command-line option.

### YAML Syntax Guidelines

- Use spaces for indentation (2 or 4 spaces recommended)
- Strings with special characters should be quoted
- Boolean values: `true`/`false` (lowercase)
- Comments start with `#`
- Lists use `-` prefix or `[item1, item2]` format

### File Structure

```yaml
# Core application settings
DirectoryToMonitor: "/path/to/recordings"
DATABASE_PATH: "/path/to/database.db"
LoopWaitSeconds: 200

# API configuration
OPENAI_API_KEY: "your_api_key"

# Talkgroup-specific settings
TALKGROUP_FILES:
  # ... talkgroup configuration

# Performance and debugging
MIN_DURATION_SECONDS: 9
DEBUG_MAIN: false
```

## Core Configuration

### Required Settings

#### DirectoryToMonitor
**Type**: String  
**Required**: Yes  
**Description**: Path to the directory containing SDRTrunk MP3 recordings

```yaml
DirectoryToMonitor: "/home/user/SDRTrunk/recordings"
```

**Notes**:
- Must be an absolute path
- Directory must exist and be readable
- Application monitors this directory for new `.mp3` files
- Supports nested directories if recordings are organized in subdirectories

#### DATABASE_PATH
**Type**: String  
**Required**: Yes  
**Description**: Path to SQLite database file for storing transcriptions

```yaml
DATABASE_PATH: "/home/user/SDRTrunk/recordings.db"
```

**Notes**:
- Can be absolute or relative path
- Database file will be created if it doesn't exist
- Parent directory must exist and be writable
- Regular backups recommended for important data

#### LoopWaitSeconds
**Type**: Integer  
**Required**: Yes  
**Default**: 200  
**Description**: Polling interval in milliseconds between directory checks

```yaml
LoopWaitSeconds: 200  # Check every 200ms
```

**Tuning Guidelines**:
- Lower values (100-500ms): More responsive to new files, higher CPU usage
- Higher values (1000-5000ms): Lower CPU usage, less responsive
- Very high volume: Consider 1000ms or higher
- Low volume/testing: 200-500ms is fine

### Optional Core Settings

#### MIN_DURATION_SECONDS
**Type**: Integer  
**Default**: 9  
**Description**: Minimum duration in seconds for files to be processed

```yaml
MIN_DURATION_SECONDS: 9
```

**Use Cases**:
- Skip very short recordings (button presses, noise)
- Reduce processing load from insignificant audio
- Typical range: 5-15 seconds

## Transcription Settings

### OpenAI API Configuration

#### OPENAI_API_KEY
**Type**: String  
**Required**: Yes (for API mode)  
**Description**: OpenAI API key for Whisper transcription service

```yaml
OPENAI_API_KEY: "sk-your-openai-api-key-here"
```

**Security Notes**:
- Never commit API keys to version control
- Use environment variables in production
- Rotate keys regularly
- Monitor API usage and costs

#### MAX_REQUESTS_PER_MINUTE
**Type**: Integer  
**Default**: 50  
**Description**: Maximum API requests per minute to avoid rate limiting

```yaml
MAX_REQUESTS_PER_MINUTE: 50
```

**Rate Limiting Guidelines**:
- OpenAI Whisper API default limit: 50 requests/minute
- Paid accounts may have higher limits
- Monitor actual usage and adjust accordingly
- Consider local transcription for high volume

#### MAX_RETRIES
**Type**: Integer  
**Default**: 3  
**Description**: Maximum retry attempts for failed API requests

```yaml
MAX_RETRIES: 3
```

#### ERROR_WINDOW_SECONDS
**Type**: Integer  
**Default**: 300  
**Description**: Time window for error rate monitoring (5 minutes)

```yaml
ERROR_WINDOW_SECONDS: 300
```

**Error Handling**:
- Application exits if error rate exceeds 25% within this window
- Prevents runaway failures and API costs
- Adjust based on expected reliability

#### RATE_LIMIT_WINDOW_SECONDS
**Type**: Integer  
**Default**: 60  
**Description**: Time window for rate limit enforcement

```yaml
RATE_LIMIT_WINDOW_SECONDS: 60
```

### Local Transcription (faster-whisper)

For local transcription mode (`--local` flag), no additional configuration is required in the YAML file. The application will use the `fasterWhisper.py` script.

**Prerequisites**:
- `faster-whisper` Python package installed
- `fasterWhisper.py` script in the same directory as the binary
- Sufficient system resources (CPU/GPU/RAM)

## Talkgroup Configuration

### TALKGROUP_FILES Structure

Talkgroup-specific glossary mappings allow automatic translation of tencodes, signals, and callsigns in transcriptions.

```yaml
TALKGROUP_FILES:
  talkgroup_spec:
    GLOSSARY:
      - "/path/to/glossary1.json"
      - "/path/to/glossary2.json"
```

### Talkgroup Specification Formats

#### Individual Talkgroups
```yaml
TALKGROUP_FILES:
  28513:
    GLOSSARY: ["/path/to/police_codes.json"]
  41003:
    GLOSSARY: ["/path/to/fire_codes.json"]
  41004:
    GLOSSARY: ["/path/to/ems_codes.json"]
```

#### Comma-Separated Lists
```yaml
TALKGROUP_FILES:
  "28513,41003,41004,41013,41020":
    GLOSSARY: ["/path/to/common_codes.json"]
```

#### Ranges
```yaml
TALKGROUP_FILES:
  "52197-52201":
    GLOSSARY:
      - "/path/to/tencode_glossary.json"
      - "/path/to/signals_glossary.json"
```

#### Mixed Specifications
```yaml
TALKGROUP_FILES:
  # Police departments (range)
  "52197-52201":
    GLOSSARY:
      - "/path/to/police_tencodes.json"
      - "/path/to/police_signals.json"
  
  # Fire departments (individual and list)
  "28513,28514,28515":
    GLOSSARY:
      - "/path/to/fire_codes.json"
  
  # EMS (individual)
  41003:
    GLOSSARY:
      - "/path/to/ems_codes.json"
      - "/path/to/medical_abbreviations.json"
```

### Glossary File Format

Glossary files must be valid JSON with string key-value pairs:

```json
{
  "10-4": "Acknowledgment",
  "10-20": "What is your location?",
  "10-23": "Stand by",
  "Code 3": "Emergency Response with Lights and Siren",
  "Signal 7": "Out of Service",
  "QRT": "Quick Response Team",
  "ETA": "Estimated Time of Arrival"
}
```

**Glossary Best Practices**:
- Use consistent formatting
- Include common variations ("10-4", "10 4", "ten four")
- Provide meaningful, context-appropriate definitions
- Organize by category (tencodes, signals, abbreviations)
- Regular updates as terminology evolves

### Example Complete Talkgroup Configuration

```yaml
TALKGROUP_FILES:
  # Metro Police Districts 1-5
  "52197-52201":
    GLOSSARY:
      - "/home/user/glossaries/police_ten_codes.json"
      - "/home/user/glossaries/police_signals.json"
      - "/home/user/glossaries/common_abbreviations.json"
  
  # Fire Department Companies
  "28513,28514,28515,28516":
    GLOSSARY:
      - "/home/user/glossaries/fire_codes.json"
      - "/home/user/glossaries/hazmat_codes.json"
  
  # EMS and Ambulance Services
  41003:
    GLOSSARY:
      - "/home/user/glossaries/medical_codes.json"
      - "/home/user/glossaries/ems_protocols.json"
  
  # Airport Operations
  45001:
    GLOSSARY:
      - "/home/user/glossaries/aviation_codes.json"
  
  # Highway Maintenance
  "47000-47099":
    GLOSSARY:
      - "/home/user/glossaries/dot_codes.json"
```

## Performance Tuning

### High-Volume Deployments

```yaml
# Reduce polling frequency for high-volume directories
LoopWaitSeconds: 1000  # 1 second

# Increase minimum duration to skip very short files
MIN_DURATION_SECONDS: 15

# Conservative API rate limiting
MAX_REQUESTS_PER_MINUTE: 30

# Disable unnecessary debug output
DEBUG_MAIN: false
DEBUG_FILE_PROCESSOR: false
DEBUG_DATABASE_MANAGER: false
DEBUG_CURL_HELPER: false
DEBUG_TRANSCRIPTION_PROCESSOR: false
```

### Low-Latency Configurations

```yaml
# More frequent polling for immediate processing
LoopWaitSeconds: 100  # 100ms

# Process even short files
MIN_DURATION_SECONDS: 3

# Aggressive API usage (if limits allow)
MAX_REQUESTS_PER_MINUTE: 60

# Quick retry on failures
MAX_RETRIES: 2
ERROR_WINDOW_SECONDS: 180
```

### Resource-Constrained Environments

```yaml
# Longer polling intervals to reduce CPU usage
LoopWaitSeconds: 5000  # 5 seconds

# Skip short files to reduce processing load
MIN_DURATION_SECONDS: 20

# Conservative API usage
MAX_REQUESTS_PER_MINUTE: 20

# Longer error windows
ERROR_WINDOW_SECONDS: 600
```

## Debug Configuration

Debug flags enable detailed logging for specific components. Enable only what's needed to avoid log spam.

### Debug Flags

#### DEBUG_MAIN
**Type**: Boolean  
**Default**: false  
**Description**: Main application loop and overall flow

```yaml
DEBUG_MAIN: true
```

**Output Examples**:
- Directory polling status
- File discovery notifications
- Application lifecycle events

#### DEBUG_FILE_PROCESSOR
**Type**: Boolean  
**Default**: false  
**Description**: File parsing, validation, and metadata extraction

```yaml
DEBUG_FILE_PROCESSOR: true
```

**Output Examples**:
- Filename parsing details
- Duration validation results
- File metadata extraction

#### DEBUG_DATABASE_MANAGER
**Type**: Boolean  
**Default**: false  
**Description**: Database operations and SQL execution

```yaml
DEBUG_DATABASE_MANAGER: true
```

**Output Examples**:
- SQL statement execution
- Database connection status
- Record insertion results

#### DEBUG_CURL_HELPER
**Type**: Boolean  
**Default**: false  
**Description**: HTTP requests, API communication, and rate limiting

```yaml
DEBUG_CURL_HELPER: true
```

**Output Examples**:
- API request details
- Rate limiting status
- HTTP response codes and timing

#### DEBUG_TRANSCRIPTION_PROCESSOR
**Type**: Boolean  
**Default**: false  
**Description**: Transcription processing and glossary mapping

```yaml
DEBUG_TRANSCRIPTION_PROCESSOR: true
```

**Output Examples**:
- Glossary loading status
- Term matching results
- Enhanced transcription generation

### Debug Configuration Examples

#### Development/Troubleshooting
```yaml
# Enable all debug output
DEBUG_MAIN: true
DEBUG_FILE_PROCESSOR: true
DEBUG_DATABASE_MANAGER: true
DEBUG_CURL_HELPER: true
DEBUG_TRANSCRIPTION_PROCESSOR: true
```

#### API Issues
```yaml
# Focus on API-related debugging
DEBUG_MAIN: true
DEBUG_CURL_HELPER: true
DEBUG_FILE_PROCESSOR: false
DEBUG_DATABASE_MANAGER: false
DEBUG_TRANSCRIPTION_PROCESSOR: false
```

#### File Processing Issues
```yaml
# Focus on file handling
DEBUG_MAIN: true
DEBUG_FILE_PROCESSOR: true
DEBUG_CURL_HELPER: false
DEBUG_DATABASE_MANAGER: false
DEBUG_TRANSCRIPTION_PROCESSOR: false
```

#### Production (Minimal Debugging)
```yaml
# Disable all debug output for production
DEBUG_MAIN: false
DEBUG_FILE_PROCESSOR: false
DEBUG_DATABASE_MANAGER: false
DEBUG_CURL_HELPER: false
DEBUG_TRANSCRIPTION_PROCESSOR: false
```

## Environment Variables

Some configuration options can be overridden with environment variables, useful for containerized deployments or CI/CD.

### Supported Environment Variables

```bash
# Override OpenAI API key
export OPENAI_API_KEY="sk-your-api-key"

# Override database path
export DATABASE_PATH="/var/lib/sdrtrunk/recordings.db"

# Override monitoring directory
export DIRECTORY_TO_MONITOR="/var/spool/sdrtrunk"
```

### Docker Environment Example

```dockerfile
ENV OPENAI_API_KEY="sk-your-api-key"
ENV DATABASE_PATH="/data/recordings.db"
ENV DIRECTORY_TO_MONITOR="/recordings"
ENV DEBUG_MAIN="false"
```

## Configuration Validation

### Validation Methods

#### YAML Syntax Validation
```bash
# Using yamllint
yamllint config.yaml

# Using Python
python3 -c "import yaml; yaml.safe_load(open('config.yaml'))"

# Using yq
yq eval '.' config.yaml
```

#### Application Validation
```bash
# Test configuration loading
./build/sdrtrunk-transcriber --config config.yaml --help

# Verbose validation with debug output
DEBUG_MAIN=true ./build/sdrtrunk-transcriber --config config.yaml --help
```

### Common Validation Errors

#### Invalid YAML Syntax
```
Error: YAML parsing error at line 15: invalid indentation
```
**Solution**: Check indentation, quotes, and YAML syntax

#### Missing Required Fields
```
Error: Required configuration field 'DirectoryToMonitor' not found
```
**Solution**: Add all required configuration fields

#### Invalid Path
```
Error: Directory '/invalid/path' does not exist or is not readable
```
**Solution**: Verify paths exist and have correct permissions

#### Invalid Talkgroup Specification
```
Error: Invalid talkgroup specification 'abc-def'
```
**Solution**: Use numeric talkgroup IDs only

## Examples

### Basic Configuration

```yaml
# Minimal working configuration
OPENAI_API_KEY: "sk-your-api-key-here"
LoopWaitSeconds: 200
DirectoryToMonitor: "/home/user/SDRTrunk/recordings"
DATABASE_PATH: "/home/user/SDRTrunk/recordings.db"
MIN_DURATION_SECONDS: 9
MAX_RETRIES: 3
MAX_REQUESTS_PER_MINUTE: 50
ERROR_WINDOW_SECONDS: 300
RATE_LIMIT_WINDOW_SECONDS: 60

# Disable all debug output
DEBUG_CURL_HELPER: false
DEBUG_DATABASE_MANAGER: false
DEBUG_FILE_PROCESSOR: false
DEBUG_MAIN: false
DEBUG_TRANSCRIPTION_PROCESSOR: false
```

### Advanced Configuration with Talkgroups

```yaml
# OpenAI API Configuration
OPENAI_API_KEY: "sk-your-openai-api-key"
MAX_REQUESTS_PER_MINUTE: 45
MAX_RETRIES: 3
ERROR_WINDOW_SECONDS: 300
RATE_LIMIT_WINDOW_SECONDS: 60

# Core Settings
LoopWaitSeconds: 500
DirectoryToMonitor: "/opt/sdrtrunk/recordings"
DATABASE_PATH: "/opt/sdrtrunk/data/transcriptions.db"
MIN_DURATION_SECONDS: 12

# Comprehensive Talkgroup Configuration
TALKGROUP_FILES:
  # Police Departments
  "52197-52201":
    GLOSSARY:
      - "/opt/sdrtrunk/glossaries/police_ten_codes.json"
      - "/opt/sdrtrunk/glossaries/police_signals.json"
      - "/opt/sdrtrunk/glossaries/officer_callsigns.json"
  
  # Fire Department
  "28513,28514,28515":
    GLOSSARY:
      - "/opt/sdrtrunk/glossaries/fire_codes.json"
      - "/opt/sdrtrunk/glossaries/hazmat_codes.json"
  
  # EMS Services
  41003:
    GLOSSARY:
      - "/opt/sdrtrunk/glossaries/medical_codes.json"
      - "/opt/sdrtrunk/glossaries/hospital_codes.json"
  
  # Public Works
  "47000-47050":
    GLOSSARY:
      - "/opt/sdrtrunk/glossaries/public_works_codes.json"

# Selective Debug Output
DEBUG_MAIN: false
DEBUG_FILE_PROCESSOR: false
DEBUG_DATABASE_MANAGER: false
DEBUG_CURL_HELPER: false
DEBUG_TRANSCRIPTION_PROCESSOR: true  # Only glossary debugging

# Optional: SDRTrunk Integration (future use)
XML_PATH: "/opt/sdrtrunk/playlist/default.xml"
```

### High-Volume Production Configuration

```yaml
# Production API Settings
OPENAI_API_KEY: "sk-production-api-key"
MAX_REQUESTS_PER_MINUTE: 40  # Conservative rate limiting
MAX_RETRIES: 5  # More retries for reliability
ERROR_WINDOW_SECONDS: 600  # Longer error window
RATE_LIMIT_WINDOW_SECONDS: 60

# Performance Optimizations
LoopWaitSeconds: 2000  # 2-second polling for high volume
MIN_DURATION_SECONDS: 20  # Skip very short recordings

# Production Paths
DirectoryToMonitor: "/var/spool/sdrtrunk/recordings"
DATABASE_PATH: "/var/lib/sdrtrunk/production.db"

# Extensive Talkgroup Coverage
TALKGROUP_FILES:
  # All police districts
  "52000-52999":
    GLOSSARY:
      - "/etc/sdrtrunk/glossaries/police_comprehensive.json"
      - "/etc/sdrtrunk/glossaries/regional_codes.json"
  
  # All fire departments
  "28000-28999":
    GLOSSARY:
      - "/etc/sdrtrunk/glossaries/fire_comprehensive.json"
  
  # All EMS services
  "41000-41999":
    GLOSSARY:
      - "/etc/sdrtrunk/glossaries/medical_comprehensive.json"

# Production: No debug output
DEBUG_MAIN: false
DEBUG_FILE_PROCESSOR: false
DEBUG_DATABASE_MANAGER: false
DEBUG_CURL_HELPER: false
DEBUG_TRANSCRIPTION_PROCESSOR: false
```

### Development/Testing Configuration

```yaml
# Development API (or use local mode)
OPENAI_API_KEY: "sk-development-api-key"
MAX_REQUESTS_PER_MINUTE: 10  # Lower limits for testing
MAX_RETRIES: 2
ERROR_WINDOW_SECONDS: 180
RATE_LIMIT_WINDOW_SECONDS: 60

# Development Settings
LoopWaitSeconds: 100  # Fast polling for testing
MIN_DURATION_SECONDS: 3  # Process even very short files

# Development Paths
DirectoryToMonitor: "./test-recordings"
DATABASE_PATH: "./test.db"

# Test Talkgroups
TALKGROUP_FILES:
  "12345":
    GLOSSARY:
      - "./test-glossaries/test_codes.json"

# Full Debug Output
DEBUG_MAIN: true
DEBUG_FILE_PROCESSOR: true
DEBUG_DATABASE_MANAGER: true
DEBUG_CURL_HELPER: true
DEBUG_TRANSCRIPTION_PROCESSOR: true
```

## Migration Guide

### Version 1.0 to Current

No breaking changes in current version.

### Future Migration Considerations

- Configuration file format will remain stable within major versions
- New optional fields may be added in minor versions
- Deprecated fields will be supported for at least one major version
- Migration scripts will be provided for breaking changes

### Best Practices for Forward Compatibility

1. **Use explicit values**: Don't rely on default values that might change
2. **Comment configuration**: Document why specific values are chosen
3. **Version control**: Keep configuration files in version control
4. **Test updates**: Always test configuration changes in development first
5. **Backup before updates**: Keep working configurations as backup

### Configuration Backup Strategy

```bash
# Backup current configuration
cp config.yaml config.yaml.backup.$(date +%Y%m%d)

# Version control
git add config.yaml
git commit -m "Update configuration for [reason]"

# Test new configuration
./build/sdrtrunk-transcriber --config config.yaml.new --help
```

For additional configuration support, see the [GitHub Issues](https://github.com/swiftraccoon/cpp-sdrtrunk-transcriber/issues) or [Discussions](https://github.com/swiftraccoon/cpp-sdrtrunk-transcriber/discussions).