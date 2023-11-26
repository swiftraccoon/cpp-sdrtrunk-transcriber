import sys
from faster_whisper import WhisperModel
import logging
import json

# Check if a filename is provided
if len(sys.argv) < 2:
    print("Usage: python script.py <filename.mp3>")
    sys.exit(1)

# Retrieve the filename from the command line argument
MP3_FILEPATH = sys.argv[1]

# logging.basicConfig()
# logging.getLogger("faster_whisper").setLevel(logging.ERROR)

MODEL_SIZE = "large-v2"
BEAM_SIZE = 1
LANGUAGE = "en"
MIN_SILENCE_DURATION_MS = 1500

# Run on GPU with FP16
model = WhisperModel(MODEL_SIZE, device="cuda", compute_type="float16")

# or run on GPU with INT8
# model = WhisperModel(MODEL_SIZE, device="cuda", compute_type="int8_float16")
# or run on CPU with INT8
# model = WhisperModel(MODEL_SIZE, device="cpu", compute_type="int8")

segments, info = model.transcribe(
    MP3_FILEPATH, beam_size=BEAM_SIZE, vad_filter=True,
    vad_parameters=dict(min_silence_duration_ms=MIN_SILENCE_DURATION_MS),
    language=LANGUAGE)

segments = list(segments)
# Create a new list with the desired format
FORMATTED_SEGMENTS = [{"text": segment.text} for segment in segments]

# Extract the 'text' values and concatenate them into a single string
FORMATTED_TEXT = " ".join([segment['text'] for segment in FORMATTED_SEGMENTS])

# Create a dictionary with the concatenated text
FORMATTED_RESULT = {"text": FORMATTED_TEXT}

# Convert the dictionary to a JSON string
FORMATTED_RESULT_JSON = json.dumps(FORMATTED_RESULT)
FORMATTED_RESULT_JSON = FORMATTED_RESULT_JSON.replace('": "', '":"')

# Print the formatted result
print(FORMATTED_RESULT_JSON)
