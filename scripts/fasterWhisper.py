import sys
import json
import logging
import argparse
from faster_whisper import WhisperModel

# Configure logging
logging.basicConfig(level=logging.CRITICAL,
                    format='%(asctime)s - %(levelname)s - %(message)s')


class Config:
    MODEL_SIZE = "large-v3"
    BEAM_SIZE = 12
    PATIENCE = 25
    BEST_OF = 10
    LANGUAGE = "en"
    MIN_SILENCE_DURATION_MS = 1450
    THRESHOLD = 0.45
    TEMPERATURE = (0.015, 0.055, 0.95)
    REPETITION_PENALTY = 1.15
    WINDOW_SIZE_SAMPLES = 2536
    COMPRESSION_RATIO_THRESHOLD = 2.4
    LOG_PROB_THRESHOLD = -1.0
    NO_SPEECH_THRESHOLD = 0.45
    CONDITION_ON_PREVIOUS_TEXT = True
    PROMPT_RESET_ON_TEMPERATURE = 0.5


def parse_arguments():
    parser = argparse.ArgumentParser(
        description="Transcribe audio file using WhisperModel")
    parser.add_argument("filepath", help="Path to the MP3 file")
    return parser.parse_args()


def initialize_model():
    """Initializes the Whisper model.

        Args:
          model_size_or_path: Size of the model to use (tiny, tiny.en, base, base.en,
            small, small.en, medium, medium.en, large-v1, large-v2, large-v3, or large), a path to a
            converted model directory, or a CTranslate2-converted Whisper model ID from the HF Hub.
            When a size or a model ID is configured, the converted model is downloaded
            from the Hugging Face Hub.
          device: Device to use for computation ("cpu", "cuda", "auto").
          device_index: Device ID to use.
            The model can also be loaded on multiple GPUs by passing a list of IDs
            (e.g. [0, 1, 2, 3]). In that case, multiple transcriptions can run in parallel
            when transcribe() is called from multiple Python threads (see also num_workers).
          compute_type: Type to use for computation.
            See https://opennmt.net/CTranslate2/quantization.html.
          cpu_threads: Number of threads to use when running on CPU (4 by default).
            A non zero value overrides the OMP_NUM_THREADS environment variable.
          num_workers: When transcribe() is called from multiple Python threads,
            having multiple workers enables true parallelism when running the model
            (concurrent calls to self.model.generate() will run in parallel).
            This can improve the global throughput at the cost of increased memory usage.
          download_root: Directory where the models should be saved. If not set, the models
            are saved in the standard Hugging Face cache directory.
          local_files_only:  If True, avoid downloading the file and return the path to the
            local cached file if it exists.
    """
    return WhisperModel(Config.MODEL_SIZE, device="cuda",
                        compute_type="float32")


def transcribe_audio(model, file_path):
    """Transcribes an input file.

        Arguments:
          audio: Path to the input file (or a file-like object), or the audio waveform.
          language: The language spoken in the audio. It should be a language code such
            as "en" or "fr". If not set, the language will be detected in the first 30 seconds
            of audio.
          task: Task to execute (transcribe or translate).
          beam_size: Beam size to use for decoding.
          best_of: Number of candidates when sampling with non-zero temperature.
          patience: Beam search patience factor.
          length_penalty: Exponential length penalty constant.
          repetition_penalty: Penalty applied to the score of previously generated tokens
            (set > 1 to penalize).
          no_repeat_ngram_size: Prevent repetitions of ngrams with this size (set 0 to disable).
          temperature: Temperature for sampling. It can be a tuple of temperatures,
            which will be successively used upon failures according to either
            `compression_ratio_threshold` or `log_prob_threshold`.
          compression_ratio_threshold: If the gzip compression ratio is above this value,
            treat as failed.
          log_prob_threshold: If the average log probability over sampled tokens is
            below this value, treat as failed.
          no_speech_threshold: If the no_speech probability is higher than this value AND
            the average log probability over sampled tokens is below `log_prob_threshold`,
            consider the segment as silent.
          condition_on_previous_text: If True, the previous output of the model is provided
            as a prompt for the next window; disabling may make the text inconsistent across
            windows, but the model becomes less prone to getting stuck in a failure loop,
            such as repetition looping or timestamps going out of sync.
          prompt_reset_on_temperature: Resets prompt if temperature is above this value.
            Arg has effect only if condition_on_previous_text is True.
          initial_prompt: Optional text string or iterable of token ids to provide as a
            prompt for the first window.
          prefix: Optional text to provide as a prefix for the first window.
          suppress_blank: Suppress blank outputs at the beginning of the sampling.
          suppress_tokens: List of token IDs to suppress. -1 will suppress a default set
            of symbols as defined in the model config.json file.
          without_timestamps: Only sample text tokens.
          max_initial_timestamp: The initial timestamp cannot be later than this.
          word_timestamps: Extract word-level timestamps using the cross-attention pattern
            and dynamic time warping, and include the timestamps for each word in each segment.
          prepend_punctuations: If word_timestamps is True, merge these punctuation symbols
            with the next word
          append_punctuations: If word_timestamps is True, merge these punctuation symbols
            with the previous word
          vad_filter: Enable the voice activity detection (VAD) to filter out parts of the audio
            without speech. This step is using the Silero VAD model
            https://github.com/snakers4/silero-vad.
          vad_parameters: Dictionary of Silero VAD parameters or VadOptions class (see available
            parameters and default values in the class `VadOptions`).

        Returns:
          A tuple with:

            - a generator over transcribed segments
            - an instance of TranscriptionInfo

    VAD options.

    Attributes:
      threshold: Speech threshold. Silero VAD outputs speech probabilities for each audio chunk,
        probabilities ABOVE this value are considered as SPEECH. It is better to tune this
        parameter for each dataset separately, but "lazy" 0.5 is pretty good for most datasets.
      min_speech_duration_ms: Final speech chunks shorter min_speech_duration_ms are thrown out.
      max_speech_duration_s: Maximum duration of speech chunks in seconds. Chunks longer
        than max_speech_duration_s will be split at the timestamp of the last silence that
        lasts more than 100ms (if any), to prevent aggressive cutting. Otherwise, they will be
        split aggressively just before max_speech_duration_s.
      min_silence_duration_ms: In the end of each speech chunk wait for min_silence_duration_ms
        before separating it
      window_size_samples: Audio chunks of window_size_samples size are fed to the silero VAD model.
        WARNING! Silero VAD models were trained using 512, 1024, 1536 samples for 16000 sample rate.
        Values other than these may affect model performance!!
      speech_pad_ms: Final speech chunks are padded by speech_pad_ms each side
    """
    try:
        segments, info = model.transcribe(
            file_path, beam_size=Config.BEAM_SIZE, patience=Config.PATIENCE,
            best_of=Config.BEST_OF, no_speech_threshold=Config.NO_SPEECH_THRESHOLD,
            log_prob_threshold=Config.LOG_PROB_THRESHOLD,
            compression_ratio_threshold=Config.COMPRESSION_RATIO_THRESHOLD,
            repetition_penalty=Config.REPETITION_PENALTY,
            condition_on_previous_text=Config.CONDITION_ON_PREVIOUS_TEXT,
            prompt_reset_on_temperature=Config.PROMPT_RESET_ON_TEMPERATURE,
            initial_prompt="", temperature=Config.TEMPERATURE, vad_filter=True,
            vad_parameters=dict(threshold=Config.THRESHOLD,
                                min_silence_duration_ms=Config.MIN_SILENCE_DURATION_MS,
                                window_size_samples=Config.WINDOW_SIZE_SAMPLES),
            language=Config.LANGUAGE)
        return segments, info
    except Exception as e:
        logging.error(f"Error in transcribing audio: {e}")
        sys.exit(1)


def format_segments(segments):
    formatted_segments = [{"text": segment.text} for segment in segments]
    formatted_text = " ".join(segment['text'].strip()
                              for segment in formatted_segments)
    return json.dumps({"text": formatted_text}).replace('": "', '":"')


def main():
    args = parse_arguments()
    model = initialize_model()
    segments, info = transcribe_audio(model, args.filepath)
    # print(info)
    # for segment in segments:
    #     print(segment)
    formatted_result = format_segments(segments)
    print(formatted_result)


if __name__ == "__main__":
    main()
