for file in $(find /home/$USER/SDRTrunk/recordings -mindepth 2 -type f -name "*.txt"); do
    if ! [ -s "$file" ] || grep -qE "Bad gateway|Internal server error|UNABLE_TO_TRANSCRIBE_CHECK_FILE|curlHelper.cpp_UNABLE_TO_TRANSCRIBE_CHECK_FILE" "$file"; then
        echo "Deleting $file"
        rm "$file"
    fi
done
