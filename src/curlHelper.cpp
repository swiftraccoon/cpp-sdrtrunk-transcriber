// Standard Library Headers
#include <chrono>
#include <cstdlib>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

// Project-Specific Headers
#include "../include/curlHelper.h"
#include "../include/debugUtils.h"
#include "../include/ConfigSingleton.h"

ConfigSingleton &config = ConfigSingleton::getInstance();
const std::string API_URL = "https://api.openai.com/v1/audio/transcriptions";
std::deque<std::chrono::steady_clock::time_point> requestTimestamps;
std::deque<std::chrono::steady_clock::time_point> errorTimestamps;
int apiErrorCount = 0;

// Callback function to write the CURL response to a string
size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
}

// Setup CURL headers
void setupCurlHeaders(CURL *curl, struct curl_slist *&headers, const std::string &OPENAI_API_KEY)
{
    headers = curl_slist_append(headers, ("Authorization: Bearer " + std::string(OPENAI_API_KEY)).c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
}

// Setup CURL post fields
void setupCurlPostFields(CURL *curl, curl_mime *&mime, const std::string &file_path)
{
    curl_mimepart *part;
    mime = curl_mime_init(curl);

    // Add the file
    part = curl_mime_addpart(mime);
    curl_mime_name(part, "file");
    curl_mime_filedata(part, file_path.c_str());

    // Add other data fields
    part = curl_mime_addpart(mime);
    curl_mime_name(part, "model");
    curl_mime_data(part, "whisper-1", CURL_ZERO_TERMINATED);

    part = curl_mime_addpart(mime);
    curl_mime_name(part, "response_format");
    curl_mime_data(part, "json", CURL_ZERO_TERMINATED);

    part = curl_mime_addpart(mime);
    curl_mime_name(part, "temperature");
    curl_mime_data(part, "0", CURL_ZERO_TERMINATED);

    part = curl_mime_addpart(mime);
    curl_mime_name(part, "language");
    curl_mime_data(part, "en", CURL_ZERO_TERMINATED);
}

// Make a CURL request and return the response
std::string makeCurlRequest(CURL *curl, curl_mime *mime)
{
    std::string response;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK)
    {
        throw std::runtime_error("[" + getCurrentTime() + "]" + " curlHelper.cpp makeCurlRequest CURL request failed: " + std::string(curl_easy_strerror(res)));
    }
    return response;
}

// Check if the response contains a valid transcription and no halucinations
bool isValidResponse(const std::string &response)
{
    return !response.empty() && response != "Thank you for watching";
}

// List of API error messages to check against
const std::vector<std::string> apiErrorMessages = {
    "Bad gateway",
    "Internal server error",
    "Invalid file format.",
    "server_error"};

// Function to check if the response contains any of the error messages
bool containsApiError(const std::string &response)
{
    for (const auto &errorMsg : apiErrorMessages)
    {
        if (response.find(errorMsg) != std::string::npos)
        {
            return true;
        }
    }
    return false;
}

// Check file existence and readability
void checkFileValidity(const std::string &file_path)
{
    std::cout << "[" << getCurrentTime() << "] curlHelper.cpp checkFileValidity Checking if file exists." << std::endl;
    if (!std::filesystem::exists(file_path))
    {
        throw std::runtime_error("[" + getCurrentTime() + "]" + " curlHelper.cpp checkFileValidity File does not exist: " + file_path);
        std::cout.flush();
    }

    std::ifstream file(file_path);
    std::cout << "[" << getCurrentTime() << "] curlHelper.cpp checkFileValidity Checking if file is readable." << std::endl;
    if (!file.good())
    {
        throw std::runtime_error("[" + getCurrentTime() + "]" + " curlHelper.cpp checkFileValidity Cannot read file: " + file_path);
        std::cout.flush();
    }
    file.close();
}

// Handle rate limiting
void handleRateLimiting()
{
    std::chrono::seconds rateLimitWindow(config.getRateLimitWindowSeconds());
    int maxRequestsPerMinute = config.getMaxRequestsPerMinute();
    std::cout << "[" << getCurrentTime() << "] curlHelper.cpp Entered handleRateLimiting" << std::endl;
    auto now = std::chrono::steady_clock::now();
    std::cout << "[" << getCurrentTime() << "] curlHelper.cpp handleRateLimiting rateLimitWindow: " << rateLimitWindow.count() << " seconds" << std::endl;
    // Remove timestamps outside the 1-minute window
    while (!requestTimestamps.empty() && (now - requestTimestamps.front() > rateLimitWindow))
    {
        requestTimestamps.pop_front();
        if (!errorTimestamps.empty())
        {
            errorTimestamps.pop_front();
            apiErrorCount--;
        }
    }

    // Check if we've exceeded the rate limit
    std::cout << "[" << getCurrentTime() << "] curlHelper.cpp handleRateLimiting maxRequestsPerMinute: " << maxRequestsPerMinute << std::endl;
    if (requestTimestamps.size() >= maxRequestsPerMinute)
    {
        auto sleep_duration = rateLimitWindow - (now - requestTimestamps.front());
        std::cout << "[" << getCurrentTime() << "] curlHelper.cpp handleRateLimiting Rate limit reached, sleeping for " << sleep_duration.count() << " seconds." << std::endl;
        std::this_thread::sleep_for(sleep_duration);
    }
}

// Transcribe audio using CURL
std::string curl_transcribe_audio(const std::string &file_path, const std::string &OPENAI_API_KEY)
{
    int maxRetries = config.getMaxRetries();
    int maxRequestsPerMinute = config.getMaxRequestsPerMinute();
    // std::chrono::seconds errorWindow(config.getErrorWindowSeconds());
    std::cout << "[" << getCurrentTime() << "] curlHelper.cpp curl_transcribe_audio called with file path: " << file_path << std::endl;
    checkFileValidity(file_path);
    std::cout << "[" << getCurrentTime() << "] curlHelper.cpp curl_transcribe_audio Entering retry loop." << std::endl;
    std::cout.flush();
    try
    {
        std::cout << "[" << getCurrentTime() << "] curlHelper.cpp curl_transcribe_audio maxRetries: " << maxRetries << std::endl;
        std::cout << "[" << getCurrentTime() << "] curlHelper.cpp curl_transcribe_audio maxRequestsPerMinute: " << maxRequestsPerMinute << std::endl;
        for (int retryCount = 0; retryCount < maxRetries; ++retryCount)
        {
            std::cout << "[" << getCurrentTime() << "] curlHelper.cpp curl_transcribe_audio attempt " << (retryCount + 1) << " of " << maxRetries << std::endl;
            std::cout.flush();
            handleRateLimiting();

            CURL *curl = curl_easy_init();
            if (!curl)
            {
                throw std::runtime_error("[" + getCurrentTime() + "]" + " curlHelper.cpp curl_transcribe_audio CURL initialization failed");
            }

            struct curl_slist *headers = NULL;
            setupCurlHeaders(curl, headers, OPENAI_API_KEY);

            curl_mime *mime;
            setupCurlPostFields(curl, mime, file_path);

            curl_easy_setopt(curl, CURLOPT_URL, API_URL.c_str());
            curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);

            std::cout << "[" << getCurrentTime() << "] curlHelper.cpp curl_transcribe_audio Making API call." << std::endl;
            std::string response = makeCurlRequest(curl, mime);
            std::cout << "[" << getCurrentTime() << "] curlHelper.cpp curl_transcribe_audio Received response: " << response << std::endl;

            curl_easy_cleanup(curl);
            curl_mime_free(mime);

            if (!containsApiError(response) && isValidResponse(response))
            {
                std::cout << "[" << getCurrentTime() << "] curlHelper.cpp curl_transcribe_audio Valid response received." << std::endl;
                return response; // Success, return the response
            }

            if (containsApiError(response))
            {
                std::cout << "[" << getCurrentTime() << "] curlHelper.cpp curl_transcribe_audio API error detected in response." << std::endl;
                apiErrorCount++;
                auto now = std::chrono::steady_clock::now();
                errorTimestamps.push_back(now);
            }

            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "[" << getCurrentTime() << "] curlHelper.cpp curl_transcribe_audio Exception caught in retry loop: " << e.what() << std::endl;
    }

    if (apiErrorCount > maxRequestsPerMinute / 4)
    {
        std::cerr << "[" << getCurrentTime() << "] curlHelper.cpp curl_transcribe_audio Majority of retries failed due to API errors." << std::endl;
        exit(EXIT_FAILURE);
    }

    // return "curlHelper.cpp_UNABLE_TO_TRANSCRIBE_CHECK_FILE";
}