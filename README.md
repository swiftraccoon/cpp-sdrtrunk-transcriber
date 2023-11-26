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
- [faster-whisper Local Transcription](#faster-whisper-local-transcription)
- [SDRTrunk Transcribed Web](#sdrtrunk-transcribed-web)
- [Contributing](#contributing)
- [License](#license)

## Features

- Monitors a directory for new MP3 files
- Transcribes audio using OpenAI's API or local transcription with [faster-whisper](https://github.com/SYSTRAN/faster-whisper)
- Searches transcriptions for tencodes/signals/callsigns in provided JSON files and appends their translation to end of transcription
- Manages transcriptions and metadata in an SQLite3 database
- (in progress) Provides a systemd service for automated running

## Dependencies

- CMake
- ffmpeg
- SQLite3
- CURL
- yaml-cpp

### Fedora

- `sudo dnf install ffmpeg ffmpeg-devel libcurl-devel sqlite-devel pkg-config yaml-cpp-devel`

### Ubuntu

- `sudo apt-get install ffmpeg libavcodec-dev libcurl4-openssl-dev libsqlite3-dev pkg-config libyaml-cpp-dev`

### Windows

- Refer to the [Windows CMake](https://github.com/swiftraccoon/cpp-sdrtrunk-transcriber/blob/main/.github/workflows/cmake-windows.yml)

(Please feel free to update this with better steps for Windows users. I do not use Windows so I'm not sure what's necessary.)

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

- `-c, --config <config_path>`: Path to the configuration file (Optional, default is `./config.yaml`).
- `-l, --local`: Set this to enable local transcription via faster-whisper
- `-h, --help`: Display help message.

## System Service

**This is still a TODO. It should work in theory but I've not yet tested it.**

A BASH script is provided to show an example template for setting up the binary as a Linux system service.

```bash
# Edit install-systemd-service.sh
sudo ./install-systemd-service.sh
```

## faster-whisper Local Transcription

Follow the [faster-whisper installation](https://github.com/SYSTRAN/faster-whisper#installation) and ensure it's working after.

Once you've confirmed `faster-whisper` is ready to go, next simply ensure that `fasterWhisper.py` from the `scripts/` folder is in the same folder with the `sdrTrunkTranscriber` binary.

Now when you launch `sdrTrunkTranscriber --local` the transcriptions will instead be handled locally via `fasterWhisper.py`.


## SDRTrunk Transcribed Web

Now that you have a folder of processed recordings you can set them up to be displayed within [sdrtrunk-transcribed-web](https://github.com/swiftraccoon/sdrtrunk-transcribed-web).

You just need to copy the:
- `.txt` files into the `sdrtrunk-transcribed-web/public/transcriptions` folder
- `.mp3` files into `sdrtrunk-transcribed-web/public/audio` folder

An example script for Linux users to do that is in `scripts/` of this repo: 
- [rsync_local_to_server.sh](https://github.com/swiftraccoon/cpp-sdrtrunk-transcriber/blob/main/scripts/rsync_local_to_server.sh)

(I'll soon enable you to [utilize different folders](https://github.com/swiftraccoon/sdrtrunk-transcribed-web/issues/13))

## Contributing

Feel free to open issues or submit pull requests.

## License

This project is licensed under the GPL-3.0 license. See the [LICENSE](LICENSE) file for details.
