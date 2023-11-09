# C++ SDRTrunk Transcriber
[![CodeFactor](https://www.codefactor.io/repository/github/swiftraccoon/cpp-sdrtrunk-transcriber/badge)](https://www.codefactor.io/repository/github/swiftraccoon/cpp-sdrtrunk-transcriber)[![Ubuntu CodeQL](https://github.com/swiftraccoon/cpp-sdrtrunk-transcriber/actions/workflows/ubuntu-codeql.yml/badge.svg)](https://github.com/swiftraccoon/cpp-sdrtrunk-transcriber/actions/workflows/ubuntu-codeql.yml)[![CMake Ubuntu](https://github.com/swiftraccoon/cpp-sdrtrunk-transcriber/actions/workflows/cmake-ubuntu.yml/badge.svg)](https://github.com/swiftraccoon/cpp-sdrtrunk-transcriber/actions/workflows/cmake-ubuntu.yml)[![CMake Windows](https://github.com/swiftraccoon/cpp-sdrtrunk-transcriber/actions/workflows/cmake-windows.yml/badge.svg)](https://github.com/swiftraccoon/cpp-sdrtrunk-transcriber/actions/workflows/cmake-windows.yml)

[![GitHub tag](https://img.shields.io/github/tag/swiftraccoon/cpp-sdrtrunk-transcriber?include_prereleases=&sort=semver&color=blue)](https://github.com/swiftraccoon/cpp-sdrtrunk-transcriber/releases/)
[![License](https://img.shields.io/badge/License-GPL3-blue)](#license)
[![issues - cpp-sdrtrunk-transcriber](https://img.shields.io/github/issues/swiftraccoon/cpp-sdrtrunk-transcriber)](https://github.com/swiftraccoon/cpp-sdrtrunk-transcriber/issues)

This project is designed to monitor a directory for SDRTrunk P25 MP3 files, categorize them based on the talkgroup ID, and create transcription files. It utilizes OpenAI's API for audio transcription and SQLite3 for database management.

This is not at all tested on Windows. Feel free to file issues for problems with Windows usage if you're open to assisting me debug the problem. 

## Related Projects
- [sdrtrunk-transcriber](https://github.com/swiftraccoon/sdrtrunk-transcriber) (Python version of this repo)
- [sdrtrunk-transcribed-web](https://github.com/swiftraccoon/sdrtrunk-transcribed-web) (Node.JS website for displaying mp3/txt files processed by this project)

## Table of Contents

- [Features](#features)
- [Dependencies](#dependencies)
- [Installation](#installation)
- [Configuration](#configuration)
- [Usage](#usage)
- [System Service](#system-service)
- [Contributing](#contributing)
- [License](#license)

## Features

- Monitors a directory for new MP3 files
- Transcribes audio using OpenAI's API
- Manages transcriptions and metadata in an SQLite3 database
- (in progress) Provides a systemd service for automated running

## Dependencies

- CMake
- SQLite3
- CURL
- yaml-cpp

### Fedora

- `sudo dnf install libcurl-devel sqlite-devel pkg-config yaml-cpp-devel`

### Ubuntu

- `sudo apt-get install libcurl4-openssl-dev libsqlite3-dev pkg-config libyaml-cpp-dev`

### Windows

- Broken until [#17](https://github.com/swiftraccoon/cpp-sdrtrunk-transcriber/issues/17) is resolved

## Installation

```bash
# Clone the repository
git clone https://github.com/swiftraccoon/cpp-sdrtrunk-transcriber.git

# Navigate to the project directory
cd cpp-sdrtrunk-transcriber

# `git clone` external repos that .gitsubmodules will not grab for you
git clone https://github.com/jbeder/yaml-cpp.git external/yaml-cpp
git clone https://github.com/CLIUtils/CLI11.git external/CLI11

# Generate the Makefile
cmake .

# Compile the project
make
```

## Configuration

A sample configuration file is provided (`sample-config.yaml`). Edit this file to specify the directory to monitor, OpenAI API key, and other settings.

```bash
mv sample-config.yaml config.yaml
```

## Usage

Run the compiled binary with the following options:

- `-c <config_path>`: Path to the configuration file (Optional, default is `./config.yaml`).
- `-h, --help`: Display help message.

## System Service

**This is still a TODO. It should work in theory but I've not yet tested it.**

A BASH script is provided to show an example template for setting up the binary as a Linux system service.

```bash
# Edit install-systemd-service.sh
sudo ./install-systemd-service.sh
```

## Contributing

Feel free to open issues or submit pull requests.

## License

This project is licensed under the GPL-3.0 license. See the [LICENSE](LICENSE) file for details.
