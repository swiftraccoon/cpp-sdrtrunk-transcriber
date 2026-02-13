# Use the latest Ubuntu LTS version as the base image
FROM ubuntu:24.10 AS builder

# Set the working directory in the container
WORKDIR /usr/src/app

# Install build essentials and dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libcurl4-openssl-dev \
    libsqlite3-dev \
    libmpg123-dev \
    libgtest-dev \
    python3-dev \
    pkg-config \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Clone the repository
RUN git clone https://github.com/swiftraccoon/cpp-sdrtrunk-transcriber.git

# Create build/ and prepare config YAMLs from root dir
RUN cd /usr/src/app/cpp-sdrtrunk-transcriber && cp sample-config.yaml config.yaml

# Build the project
RUN cd /usr/src/app/cpp-sdrtrunk-transcriber && \
    cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON && \
    cmake --build build && \
    touch recordings.db

# Run tests
RUN cd /usr/src/app/cpp-sdrtrunk-transcriber/build && ctest --output-on-failure

# The command to run when the container starts
CMD ["./build/sdrTrunkTranscriber"]
