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

MODEL_SIZE = "large-v3"
BEAM_SIZE = 9
# default is 1
PATIENCE = 10
BEST_OF = 9
LANGUAGE = "en"
MIN_SILENCE_DURATION_MS = 1500
# Radio traffic is usually noisy, so we set the threshold to a high value
THRESHOLD = 0.45
TEMPERATURE = 0.1
REPETITION_PENALTY = 1.2

# Supported window_size_samples: [512, 1024, 1536] for 16000 sampling_rate
# Bigger window size, the higher the quality. But it will be slower.
WINDOW_SIZE_SAMPLES = 1536

# You are transcribing radio traffic from emergency services; callsigns and tencodes are common.
INITIAL_PROMPT = ""


# Run on GPU with FP32
model = WhisperModel(MODEL_SIZE, device="cuda", compute_type="float32")

# or run on GPU with INT8
# model = WhisperModel(MODEL_SIZE, device="cuda", compute_type="int8_float16")
# or run on CPU with INT8
# model = WhisperModel(MODEL_SIZE, device="cpu", compute_type="int8")

segments, info = model.transcribe(
    MP3_FILEPATH, beam_size=BEAM_SIZE, patience=PATIENCE,
    best_of=BEST_OF, repetition_penalty=REPETITION_PENALTY,
    initial_prompt=INITIAL_PROMPT,
    temperature=TEMPERATURE, vad_filter=True,
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