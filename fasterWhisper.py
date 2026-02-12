#!/usr/bin/env python3
"""
Faster Whisper transcription module for SDRTrunk Transcriber.
Can be used both as a standalone script and as a Python module.
"""

import sys
import json
import logging
from pathlib import Path

# Lazy import to avoid loading the model when just importing the module
_model = None

# Configuration constants
MODEL_SIZE = "large-v3"
BEAM_SIZE = 9
PATIENCE = 10
BEST_OF = 9
LANGUAGE = "en"
MIN_SILENCE_DURATION_MS = 1500
THRESHOLD = 0.45  # Radio traffic is usually noisy
TEMPERATURE = 0.1
REPETITION_PENALTY = 1.2
WINDOW_SIZE_SAMPLES = 1536  # Supported: [512, 1024, 1536] for 16000 sampling_rate
INITIAL_PROMPT = ""  # You are transcribing radio traffic from emergency services


def get_model():
    """Lazy initialization of the Whisper model."""
    global _model
    if _model is None:
        from faster_whisper import WhisperModel
        
        # Try GPU first, fall back to CPU if not available
        # int8_float32 uses ~2.3GB VRAM vs ~7.6GB for float32 with identical quality.
        # Pascal GPUs (GTX 10-series) don't support float16/int8_float16.
        try:
            _model = WhisperModel(MODEL_SIZE, device="cuda", compute_type="int8_float32")
            print(f"Loaded model {MODEL_SIZE} on CUDA (int8_float32)", file=sys.stderr)
        except Exception as e:
            print(f"Failed to load on CUDA: {e}, falling back to CPU", file=sys.stderr)
            try:
                _model = WhisperModel(MODEL_SIZE, device="cpu", compute_type="int8")
                print(f"Loaded model {MODEL_SIZE} on CPU", file=sys.stderr)
            except Exception as cpu_e:
                raise RuntimeError(f"Failed to load Whisper model: {cpu_e}")
    
    return _model


def transcribe(filepath):
    """
    Transcribe an audio file using Faster Whisper.
    
    Args:
        filepath: Path to the audio file (MP3, WAV, etc.)
        
    Returns:
        JSON string with transcription in format: {"text": "transcribed text"}
    """
    # Validate file exists
    audio_path = Path(filepath)
    if not audio_path.exists():
        raise FileNotFoundError(f"Audio file not found: {filepath}")
    
    # Get or initialize the model
    model = get_model()
    
    # Perform transcription
    # Note: window_size_samples removed as it's not supported in all versions
    segments, info = model.transcribe(
        str(audio_path),
        beam_size=BEAM_SIZE,
        patience=PATIENCE,
        best_of=BEST_OF,
        repetition_penalty=REPETITION_PENALTY,
        initial_prompt=INITIAL_PROMPT,
        temperature=TEMPERATURE,
        vad_filter=True,
        vad_parameters=dict(
            threshold=THRESHOLD,
            min_silence_duration_ms=MIN_SILENCE_DURATION_MS
        ),
        language=LANGUAGE
    )
    
    # Process segments
    segments = list(segments)
    
    # Extract text and concatenate
    text_parts = [segment.text.strip() for segment in segments]
    formatted_text = " ".join(text_parts)
    
    # Create result dictionary
    result = {"text": formatted_text}
    
    # Convert to compact JSON
    result_json = json.dumps(result, separators=(',', ':'))
    
    return result_json


def main():
    """Main function for standalone script usage."""
    # Check if a filename is provided
    if len(sys.argv) < 2:
        print("Usage: python fasterWhisper.py <filename.mp3>", file=sys.stderr)
        sys.exit(1)
    
    # Get filename from command line
    filepath = sys.argv[1]
    
    try:
        # Transcribe and print result
        result = transcribe(filepath)
        print(result)
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    # Run as standalone script
    main()