---
name: Bug Report
about: Report a bug to help us improve the transcriber
title: '[BUG] '
labels: 'bug'
assignees: ''

---

## Bug Description

A clear and concise description of the bug.

## Steps to Reproduce

1. Configuration used (sanitize sensitive information):
   ```yaml
   # Paste relevant config.yaml sections
   ```

2. Commands executed:
   ```bash
   # Commands that led to the issue
   ```

3. Files/input that caused the issue (if applicable)

## Expected Behavior

What you expected to happen.

## Actual Behavior

What actually happened. Include error messages, unexpected output, etc.

## Error Output

```
# Paste full error messages, stack traces, or debug output here
```

## Environment

**System Information:**
- OS: [e.g., Ubuntu 22.04, Windows 11, macOS 13]
- Architecture: [e.g., x86_64, ARM64]
- Compiler: [e.g., GCC 11.2, Clang 14, MSVC 2022]
- CMake version: [e.g., 3.22]

**Application Information:**
- Version/Commit: [e.g., v1.0.0 or commit hash]
- Build type: [e.g., Release, Debug]
- Transcription mode: [OpenAI API / Local faster-whisper]

**Dependencies:**
- FFmpeg version: 
- libcurl version: 
- SQLite version: 
- yaml-cpp version: 

## Configuration Details

**config.yaml (sanitized):**
```yaml
# Remove sensitive information like API keys
# Include relevant sections that might be related to the bug
```

**Debug output (if enabled):**
```
# Enable debug flags and paste relevant output
```

## Additional Context

- How frequently does this occur? [Always / Sometimes / Rarely]
- Does this happen with specific types of audio files?
- Any workarounds you've found?
- Related issues or discussions?

## Sample Files

If the issue is related to specific audio files, please provide:
- Sample SDRTrunk filename that causes issues
- Audio file characteristics (duration, format, etc.)
- Any relevant glossary files (sanitized)

**Note**: Please remove any sensitive information (API keys, personal paths, etc.) before submitting.
