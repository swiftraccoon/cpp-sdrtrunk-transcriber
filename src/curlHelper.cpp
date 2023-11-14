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

// Project-Specific Headers
#include "../include/curlHelper.h"
#include "../include/debugUtils.h"

const std::string API_URL = "https://api.openai.com/v1/audio/transcriptions";
const int MAX_RETRIES = 3;
const int MAX_REQUESTS_PER_MINUTE = 50;
const std::chrono::seconds ERROR_WINDOW(300);
const std::chrono::seconds RATE_LIMIT_WINDOW(60);
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

    // part = curl_mime_addpart(mime);
    // curl_mime_name(part, "prompt");
    // curl_mime_data(part, "Transcribe the radio dispatch audio. The speaker is usually a dispatcher, police officer, or EMS responder. There are often callsigns, ten-codes, and addresses said.", CURL_ZERO_TERMINATED);

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
        throw std::runtime_error("[" + getCurrentTime() + "]" + "curlHelper.cpp CURL request failed: " + std::string(curl_easy_strerror(res)));
    }
    return response;
}

bool isValidResponse(const std::string &response)
{
    return !response.empty() && response != "Thanks for watching!";
}

// Transcribe audio using CURL
std::string curl_transcribe_audio(const std::string &file_path, const std::string &OPENAI_API_KEY)
{
    // Check if the file exists
    if (!std::filesystem::exists(file_path))
    {
        throw std::runtime_error("[" + getCurrentTime() + "] curlHelper.cpp File does not exist: " + file_path);
    }

    // Check if the file is readable
    std::ifstream file(file_path);
    if (!file.good())
    {
        throw std::runtime_error("[" + getCurrentTime() + "] curlHelper.cpp Cannot read file: " + file_path);
    }
    file.close();

    std::regex apiErrorPattern(R"(Bad gateway|Internal server error|Invalid file format.|server_error)");

    // Rate-limiting logic
    auto now = std::chrono::steady_clock::now();

    // Remove timestamps and corresponding errors outside the 1-minute window
    while (!requestTimestamps.empty() && (now - requestTimestamps.front() > RATE_LIMIT_WINDOW))
    {
        requestTimestamps.pop_front();
        if (!errorTimestamps.empty())
        {
            errorTimestamps.pop_front();
            apiErrorCount--;
        }
    }

    // Check if we've exceeded the rate limit
    if (requestTimestamps.size() >= MAX_REQUESTS_PER_MINUTE)
    {
        // Calculate sleep duration: time until the oldest request in the window "expires"
        auto sleep_duration = RATE_LIMIT_WINDOW - (now - requestTimestamps.front());
        std::this_thread::sleep_for(sleep_duration);
    }

    for (int retryCount = 0; retryCount < MAX_RETRIES; ++retryCount)
    {
        CURL *curl = curl_easy_init();
        if (!curl)
        {
            throw std::runtime_error("[" + getCurrentTime() + "] curlHelper.cpp CURL initialization failed");
        }

        struct curl_slist *headers = NULL;
        setupCurlHeaders(curl, headers, OPENAI_API_KEY);

        curl_mime *mime;
        setupCurlPostFields(curl, mime, file_path);

        curl_easy_setopt(curl, CURLOPT_URL, API_URL.c_str());
        curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);

        std::string response = makeCurlRequest(curl, mime);

        curl_easy_cleanup(curl);
        curl_mime_free(mime);

        if (!std::regex_search(response, apiErrorPattern) && isValidResponse(response))
        {
            return response; // Success, return the response
        }

        // Increment error count if it's an API error
        if (std::regex_search(response, apiErrorPattern))
        {
            apiErrorCount++;
            errorTimestamps.push_back(std::chrono::steady_clock::now());
        }

        // Add a delay before retrying
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }

    // Check if 25% of the MAX_REQUESTS_PER_MINUTE were API errors
    if (apiErrorCount > MAX_REQUESTS_PER_MINUTE / 4)
    {
        std::cerr << "[" << getCurrentTime() << "] Exiting! More than 25% of requests failed due to API errors within the rate limit window." << std::endl;
        exit(EXIT_FAILURE);
    }

    // If unable to transcribe the file after MAX_RETRIES, return an error message
    return "UNABLE_TO_TRANSCRIBE_CHECK_FILE";
}