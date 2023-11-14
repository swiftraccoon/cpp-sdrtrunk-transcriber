for file in $(find /home/$USER/SDRTrunk/recordings -mindepth 2 -type f -name "*.mp3" | while read -r mp3file; do
    txtfile="${mp3file%.mp3}.txt"
    [[ ! -f "$txtfile" ]] && echo "$mp3file"
done); do
    echo "Moving $file"
    mv "$file" /home/$USER/SDRTrunk/recordings
done
