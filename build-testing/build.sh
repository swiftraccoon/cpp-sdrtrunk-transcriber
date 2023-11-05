#!/bin/bash

if [ "$1" = "debug" ]; then
    sudo docker build --target builder -t cpp-sdrtrunk-transcriber .
    sudo docker run -it --name cpp-build-container cpp-sdrtrunk-transcriber /bin/bash
else
    sudo docker-compose build
    sudo docker-compose up
fi