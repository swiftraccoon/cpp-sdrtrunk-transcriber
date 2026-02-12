#include <iostream>
#include <string>
#include <filesystem>
#include <stdexcept>
#include <vector>
#include <thread>
#include <chrono>
#include <expected>
#include <mutex>
#include "security.h"

#ifdef USE_PYBIND11
#include <pybind11/embed.h>
#include <pybind11/stl.h>
namespace py = pybind11;
#else
// Fallback to process execution if pybind11 not available
#include <array>
#include <memory>
#include <cstdio>
#ifdef _WIN32
#include <stdio.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#include <cstring>
#endif
#endif

std::string trim(const std::string &str)
{
    const char *whitespace = " \t\n\r\f\v";

    size_t start = str.find_first_not_of(whitespace);
    size_t end = str.find_last_not_of(whitespace);

    return (start == std::string::npos || end == std::string::npos) ? "" : str.substr(start, end - start + 1);
}

#ifdef USE_PYBIND11

// Global Python interpreter guard - initialized once
static std::unique_ptr<py::scoped_interpreter> python_guard;
// Release the GIL after init so worker threads can acquire it
static std::unique_ptr<py::gil_scoped_release> gil_release;
static py::module_ faster_whisper_module;
static std::once_flag python_init_flag;
static bool python_initialized = false;
// Serialize all Python transcription calls — ctranslate2 releases the GIL
// during CUDA operations, so the GIL alone is insufficient for thread safety
static std::mutex transcribe_mutex;

void initialize_python_if_needed()
{
    std::call_once(python_init_flag, []() {
        try {
            // Initialize Python interpreter (acquires GIL)
            python_guard = std::make_unique<py::scoped_interpreter>();

            // Scope Python objects so they're destroyed while GIL is held
            {
                py::module_ sys = py::module_::import("sys");
                py::list path = sys.attr("path");

                // Add current directory to Python path
                path.append(".");

                // Alternative venv paths for different Python versions
                std::vector<std::string> venv_paths = {
                    ".venv/lib/python3.13/site-packages",
                    ".venv/lib/python3.12/site-packages",
                    ".venv/lib/python3.11/site-packages",
                    ".venv/lib/python3.10/site-packages",
                    ".venv/lib/python3.9/site-packages"
                };

                for (const auto& vpath : venv_paths) {
                    if (std::filesystem::exists(vpath)) {
                        path.insert(0, vpath);
                        std::cout << "Added venv path: " << vpath << std::endl;
                        break;
                    }
                }

                // Import our fasterWhisper module
                faster_whisper_module = py::module_::import("fasterWhisper");
            }

            python_initialized = true;

            // Release the GIL so any thread can acquire it for transcription
            gil_release = std::make_unique<py::gil_scoped_release>();
        } catch (const py::error_already_set& e) {
            throw std::runtime_error("Failed to initialize Python: " + std::string(e.what()));
        }
    });
}

std::expected<std::string, std::string> local_transcribe_audio(const std::string &mp3FilePath)
{
    // Validate the input file path
    if (!std::filesystem::exists(mp3FilePath)) {
        return std::unexpected("Input file does not exist: " + mp3FilePath);
    }
    
    // Get canonical path to prevent directory traversal
    std::filesystem::path safePath;
    try {
        safePath = std::filesystem::canonical(mp3FilePath);
    } catch (const std::filesystem::filesystem_error& e) {
        return std::unexpected("Failed to get canonical path: " + std::string(e.what()));
    }
    
    try {
        // Ensure Python is initialized
        initialize_python_if_needed();

        // Serialize transcription calls — ctranslate2 releases the GIL during
        // CUDA ops, so concurrent Python transcribe() calls race on model state
        std::lock_guard<std::mutex> lock(transcribe_mutex);

        // Acquire GIL for thread safety
        py::gil_scoped_acquire acquire;

        // Call the transcribe function from the Python module
        py::object result = faster_whisper_module.attr("transcribe")(safePath.string());

        // Convert result to string
        std::string transcription = py::str(result);

        // Extract JSON if it's embedded in output
        size_t jsonStartPos = transcription.find('{');
        if (jsonStartPos != std::string::npos) {
            return trim(transcription.substr(jsonStartPos));
        }

        return transcription;

    } catch (const py::error_already_set& e) {
        // Python exception occurred
        return std::unexpected("Python error in transcription: " + std::string(e.what()));
    } catch (const std::exception& e) {
        return std::unexpected("Error in transcription: " + std::string(e.what()));
    }
}

// Clean up Python interpreter on program exit
void cleanup_python()
{
    if (python_initialized) {
        // Leak all Python/CUDA resources to avoid shutdown crashes.
        // ctranslate2's CUDA cleanup throws std::runtime_error from destructors
        // when the CUDA driver is shutting down, which calls std::terminate.
        // Releasing without destroying prevents any Python/CUDA object destruction.
        // The OS reclaims all resources when the process exits.
        faster_whisper_module.release();  // Detach without Py_XDECREF
        gil_release.release();            // Detach without re-acquiring GIL
        python_guard.release();           // Detach without Py_Finalize
        python_initialized = false;
    }
}

#else

// Fallback implementation using process execution
std::expected<std::string, std::string> local_transcribe_audio(const std::string &mp3FilePath)
{
    // Validate the input file path
    if (!std::filesystem::exists(mp3FilePath)) {
        return std::unexpected("Input file does not exist: " + mp3FilePath);
    }
    
    // Get canonical path to prevent directory traversal
    std::filesystem::path safePath;
    try {
        safePath = std::filesystem::canonical(mp3FilePath);
    } catch (const std::filesystem::filesystem_error& e) {
        return std::unexpected("Failed to get canonical path: " + std::string(e.what()));
    }
    
    // Use escaped shell argument for safety
    std::string escapedPath = Security::escapeShellArg(safePath.string());
    std::string command = "python fasterWhisper.py " + escapedPath;

    std::array<char, 128> buffer;
    std::string result;

#ifdef _WIN32
    std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(command.c_str(), "r"), _pclose);
#else
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
#endif

    if (!pipe) {
        return std::unexpected("Failed to execute Python script: popen() failed");
    }

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }

    size_t jsonStartPos = result.find('{');
    if (jsonStartPos != std::string::npos) {
        return trim(result.substr(jsonStartPos));
    }

    return std::unexpected("Invalid response from Python script: no JSON found");
}

void cleanup_python()
{
    // No-op for non-pybind11 version
}

#endif