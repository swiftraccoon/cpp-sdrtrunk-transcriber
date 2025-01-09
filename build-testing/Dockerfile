# Use the latest Ubuntu LTS version as the base image
FROM ubuntu:24.10 AS builder

# Set the working directory in the container
WORKDIR /usr/src/app

# Install build essentials and other dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libcurl4-openssl-dev \
    libsqlite3-dev \
    pkg-config \
    libyaml-cpp-dev \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Clone the submodules
RUN git clone --recurse-submodules https://github.com/swiftraccoon/cpp-sdrtrunk-transcriber.git

# .gitmodules isn't working for some reason, so we'll do it manually
RUN cd /usr/src/app/cpp-sdrtrunk-transcriber/external/ && \
    git clone https://github.com/jbeder/yaml-cpp.git yaml-cpp && \
    git clone https://github.com/CLIUtils/CLI11.git CLI11

# Create build/ and prepare config YAMLs from root dir
RUN cd /usr/src/app/cpp-sdrtrunk-transcriber && mkdir build && cp sample-config.yaml build/config.yaml && mv sample-config.yaml config.yaml

# Move into build/ and begin build process
RUN cd /usr/src/app/cpp-sdrtrunk-transcriber/build && cmake .. && make && cp sdrTrunkTranscriber .. && touch recordings.db

# The command to run when the container starts (if you have an executable to run)
CMD ["./sdrTrunkTranscriber"]
