#pragma once

// Standard Library Headers
#include <string>

// Third-Party Library Headers
#include <curl/curl.h>


// Callback function to write the CURL response to a string
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);

// Setup CURL headers
void setupCurlHeaders(CURL *curl, struct curl_slist *&headers, const std::string& OPENAI_API_KEY);

// Setup CURL post fields
void setupCurlPostFields(CURL* curl, curl_mime*& mime, const std::string& file_path);

// Make a CURL request and return the response
std::string makeCurlRequest(CURL* curl, curl_mime* mime);

// Transcribe audio using CURL
std::string curl_transcribe_audio(const std::string &file_path, const std::string& OPENAI_API_KEY);
