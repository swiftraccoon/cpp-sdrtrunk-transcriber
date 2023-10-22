// Third-Party Library Headers
#include <gtest/gtest.h>
#include <yaml-cpp/yaml.h>

// Project-Specific Headers
#include "curlHelper.h"


std::string OPENAI_API_KEY;

void SetUpConfig() {
    YAML::Node config;
    try {
        config = YAML::LoadFile("config.yaml");
    } catch (const YAML::BadFile& e) {
        std::cerr << "Error: Could not open config.yaml" << std::endl;
        exit(1);
    }

    std::cout << "Debug: " << config << std::endl;  // Debug print

    if (config["OPENAI_API_KEY"]) {
        OPENAI_API_KEY = config["OPENAI_API_KEY"].as<std::string>();
    } else {
        std::cerr << "Error: OPENAI_API_KEY not found in config.yaml" << std::endl;
        exit(1);
    }
}


TEST(CurlHelperTest, WriteCallback) {
    std::string data = "test_data";
    size_t size = 1;
    size_t nmemb = data.size();
    std::string output;

    size_t result = WriteCallback((void*)data.c_str(), size, nmemb, &output);

    ASSERT_EQ(result, data.size());
    ASSERT_EQ(output, data);
}

TEST(CurlHelperTest, SetupCurlHeaders) {
    CURL *curl = curl_easy_init();
    struct curl_slist *headers = NULL;

    setupCurlHeaders(curl, headers, OPENAI_API_KEY);

    ASSERT_NE(headers, nullptr);
    // Additional checks can be added here
}

TEST(CurlHelperTest, SetupCurlPostFields) {
    CURL *curl = curl_easy_init();
    curl_mime *mime;
    std::string file_path = "test_file_path";

    setupCurlPostFields(curl, mime, file_path);

    ASSERT_NE(mime, nullptr);
    // Additional checks can be added here
}

TEST(CurlHelperTest, MakeCurlRequest) {
    CURL *curl = curl_easy_init();
    curl_mime *mime;

    std::string response = makeCurlRequest(curl, mime);

    // Replace this with expected response
    std::string expected_response = "expected_response";

    ASSERT_EQ(response, expected_response);
}

TEST(CurlHelperTest, CurlTranscribeAudio) {
    std::string file_path = "test_file_path";

    std::string response = curl_transcribe_audio(file_path, OPENAI_API_KEY);

    // Replace this with expected response
    std::string expected_response = "expected_response";

    ASSERT_EQ(response, expected_response);
}

int main(int argc, char **argv) {
    SetUpConfig();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}