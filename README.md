[![CodeFactor](https://www.codefactor.io/repository/github/swiftraccoon/cpp-sdrtrunk-transcriber/badge)](https://www.codefactor.io/repository/github/swiftraccoon/cpp-sdrtrunk-transcriber)

(Python version: https://github.com/swiftraccoon/sdrtrunk-transcriber)

(released Node.JS website for displaying data: https://github.com/swiftraccoon/sdrtrunk-transcribed-web)
# C++ SDRTrunk Transcriber
## Overview

This project is designed to monitor a directory for SDRTrunk MP3 files, categorize them into a subdirectory based on talkgroup ID, and create transcription files. It utilizes OpenAI's API for audio transcription and SQLite3 for database management.
## Features

    Monitors a directory for new MP3 files
    Transcribes audio using OpenAI's API
    Manages transcriptions and metadata in an SQLite3 database
    Provides a systemd service for automated running
    Unit tests for key functionalities

## Dependencies

    CMake
    SQLite3
    CURL
    yaml-cpp
    Google Test (for unit tests)

## Installation

    Clone the repository.
    Navigate to the project directory.
    Run cmake . to generate the Makefile.
    Run make to compile the project.

Or check Releases.

## Configuration

A sample configuration file is provided (sample-config.yaml). 

Edit this file to specify the directory to monitor, OpenAI API key, and other settings.

`mv sample-config.yaml config.yaml`

The script assumes `config.yaml` is in the directory of the binary. If it isn't going to be, ensure to specify with `-c`

## Usage

Run the compiled binary with the following options:

    -c <config_path>: Path to the configuration file (Optional, default is './config.yaml').
    -h, --help: Display help message.

## System Service

There is a provided BASH script to show an example template for setting up the binary as a Linux system service.

    Edit `install-systemd-service.sh`
    `sudo ./install-systemd-service.sh`

## Contributing

Feel free to open issues or submit pull requests.
## License

This project is licensed under the GPL-3.0 license. See the LICENSE file for details.
