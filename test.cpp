#include <gtest/gtest.h>
#include "curlHelper.h"

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
    const char* ENV_OPENAI_API_KEY = std::getenv("OPENAI_API_KEY");

    setupCurlHeaders(curl, headers);

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
    const char* ENV_OPENAI_API_KEY = std::getenv("OPENAI_API_KEY");

    std::string response = curl_transcribe_audio(file_path);

    // Replace this with expected response
    std::string expected_response = "expected_response";

    ASSERT_EQ(response, expected_response);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}