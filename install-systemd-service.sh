#!/bin/bash

# Check if running as root
if [ "$EUID" -ne 0 ]; then
  echo "Please run as root"
  exit 1
fi

# Variables
SERVICE_NAME="sdrTrunkTranscriber"
DESCRIPTION="service for sdrTrunkTranscriber to process mp3 files"
AFTER="network.target"
EXEC_START="/home/user/bin/sdrTrunkTranscriber"
RESTART="always"
USER="username"
GROUP="groupname"

# Create the unit file
echo "[Unit]
Description=$DESCRIPTION
After=$AFTER

[Service]
ExecStart=$EXEC_START
Restart=$RESTART
User=$USER
Group=$GROUP

[Install]
WantedBy=multi-user.target" > /etc/systemd/system/$SERVICE_NAME.service

# Reload systemd and enable the service
systemctl daemon-reload
systemctl enable --now $SERVICE_NAME.service

# Check the status
systemctl status $SERVICE_NAME.service
