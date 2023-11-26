import sys
import json
from faster_whisper import WhisperModel
import logging

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
MIN_SILENCE_DURATION_MS = 1000
# Radio traffic is usually noisy, so we set the threshold to a high value
THRESHOLD = 0.9
# Supported window_size_samples: [512, 1024, 1536] for 16000 sampling_rate
WINDOW_SIZE_SAMPLES = 1536

# Run on GPU with FP32
model = WhisperModel(MODEL_SIZE, device="cuda", compute_type="float32")

# or run on GPU with INT8
# model = WhisperModel(MODEL_SIZE, device="cuda", compute_type="int8_float16")
# or run on CPU with INT8
# model = WhisperModel(MODEL_SIZE, device="cpu", compute_type="int8")

segments, info = model.transcribe(
    MP3_FILEPATH, beam_size=BEAM_SIZE, vad_filter=True,
    vad_parameters=dict(threshold=THRESHOLD,
                        min_silence_duration_ms=MIN_SILENCE_DURATION_MS,
                        window_size_samples=WINDOW_SIZE_SAMPLES),
    language=LANGUAGE)

segments = list(segments)
# Create a new list with the desired format
FORMATTED_SEGMENTS = [{"text": segment.text} for segment in segments]

# Extract the 'text' values and concatenate them into a single string
FORMATTED_TEXT = " ".join(
    [segment['text'].strip() for segment in FORMATTED_SEGMENTS]
)

# Create a dictionary with the concatenated text
FORMATTED_RESULT = {"text": FORMATTED_TEXT}

# Convert the dictionary to a JSON string
FORMATTED_RESULT_JSON = json.dumps(FORMATTED_RESULT)
FORMATTED_RESULT_JSON = FORMATTED_RESULT_JSON.replace('": "', '":"')

# Print the formatted result
print(FORMATTED_RESULT_JSON)
