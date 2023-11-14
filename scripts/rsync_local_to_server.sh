#!/bin/bash
CUR_PATH=$(pwd)
REMOTE_USER="root"
REMOTE_SERVER="remote.host"
REMOTE_DIR="/root"
SSH_KEY="remote_host_RSA4096"
cd /home/$USER/SDRTrunk/recordings
find . -mindepth 2 -type f -name "*.txt" -print0 | rsync -av -e "ssh -i /home/$USER/.ssh/$SSH_KEY" --files-from=- --from0 ./ $REMOTE_USER@$REMOTE_SERVER:$REMOTE_DIR/sdrtrunk-transcribed-web/public/transcriptions
find . -mindepth 2 -type f -name "*.mp3" -print0 | rsync -av -e "ssh -i /home/$USER/.ssh/$SSH_KEY" --files-from=- --from0 ./ $REMOTE_USER@$REMOTE_SERVER:$REMOTE_DIR/sdrtrunk-transcribed-web/public/audio
cd $CUR_PATH
