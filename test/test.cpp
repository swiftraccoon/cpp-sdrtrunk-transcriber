// Third-Party Library Headers
#include <gtest/gtest.h>
#include <yaml-cpp/yaml.h>

// Project-Specific Headers
#include "curlHelper.h"
#include "transcriptionProcessor.h"

std::string OPENAI_API_KEY;

void SetUpConfig()
{
    YAML::Node config;
    try
    {
        config = YAML::LoadFile("config.yaml");
    }
    catch (const YAML::BadFile &e)
    {
        std::cerr << "Error: Could not open config.yaml" << std::endl;
        exit(1);
    }

    std::cout << "Debug: " << config << std::endl; // Debug print

    if (config["OPENAI_API_KEY"])
    {
        OPENAI_API_KEY = config["OPENAI_API_KEY"].as<std::string>();
    }
    else
    {
        std::cerr << "Error: OPENAI_API_KEY not found in config.yaml" << std::endl;
        exit(1);
    }
}

// Test for readMappingFile function
TEST(TranscriptionProcessorTest, ReadMappingFile_ValidFile)
{
    auto result = readMappingFile("valid_file_path.json");
    ASSERT_EQ(result["some_key"], "some_value");
}

TEST(TranscriptionProcessorTest, ReadMappingFile_InvalidFile)
{
    auto result = readMappingFile("invalid_file_path.json");
    ASSERT_TRUE(result.empty());
}

TEST(TranscriptionProcessorTest, ReadMappingFile_InvalidJSON)
{
    auto result = readMappingFile("invalid_json_file.json");
    ASSERT_TRUE(result.empty());
}

// Test for extractActualTranscription function
TEST(TranscriptionProcessorTest, ExtractActualTranscription_ValidString)
{
    auto result = extractActualTranscription("{\"text\":\"actual_text\"}");
    ASSERT_EQ(result, "actual_text");
}

TEST(TranscriptionProcessorTest, ExtractActualTranscription_InvalidString)
{
    auto result = extractActualTranscription("invalid_string");
    ASSERT_TRUE(result.empty());
}

// Test for insertMappings function
TEST(TranscriptionProcessorTest, InsertMappings)
{
    std::stringstream ss;
    std::unordered_map<std::string, std::string> mappings = {{"key1", "value1"}, {"key2", "value2"}};
    insertMappings(ss, "some_text_with_key1", mappings);
    ASSERT_EQ(ss.str(), ",\"key1\":\"value1\"");
}

// Test for getAppropriateFile function
TEST(TranscriptionProcessorTest, GetAppropriateFile_SpecialTalkgroup)
{
    auto result = getAppropriateFile(
        52198, []()
        { return "NCSHP_file"; },
        []()
        { return "default_file"; });
    ASSERT_EQ(result, "NCSHP_file");
}

TEST(TranscriptionProcessorTest, GetAppropriateFile_DefaultTalkgroup)
{
    auto result = getAppropriateFile(
        12345, []()
        { return "NCSHP_file"; },
        []()
        { return "default_file"; });
    ASSERT_EQ(result, "default_file");
}

// Test for generateV2Transcription function
TEST(TranscriptionProcessorTest, GenerateV2Transcription)
{
    auto result = generateV2Transcription("{\"text\":\"actual_text\"}", 12345, 67890);
    ASSERT_FALSE(result.empty());
    ASSERT_NE(result.find("\"67890\":\"actual_text\""), std::string::npos);
}

TEST(CurlHelperTest, WriteCallback)
{
    std::string data = "test_data";
    size_t size = 1;
    size_t nmemb = data.size();
    std::string output;

    size_t result = WriteCallback((void *)data.c_str(), size, nmemb, &output);

    ASSERT_EQ(result, data.size());
    ASSERT_EQ(output, data);
}

TEST(CurlHelperTest, SetupCurlHeaders)
{
    CURL *curl = curl_easy_init();
    struct curl_slist *headers = NULL;

    setupCurlHeaders(curl, headers, OPENAI_API_KEY);

    ASSERT_NE(headers, nullptr);
    // Additional checks can be added here
}

TEST(CurlHelperTest, SetupCurlPostFields)
{
    CURL *curl = curl_easy_init();
    curl_mime *mime;
    std::string file_path = "test_file_path";

    setupCurlPostFields(curl, mime, file_path);

    ASSERT_NE(mime, nullptr);
    // Additional checks can be added here
}

TEST(CurlHelperTest, MakeCurlRequest)
{
    CURL *curl = curl_easy_init();
    curl_mime *mime;

    std::string response = makeCurlRequest(curl, mime);

    // Replace this with expected response
    std::string expected_response = "expected_response";

    ASSERT_EQ(response, expected_response);
}

TEST(CurlHelperTest, CurlTranscribeAudio)
{
    std::string file_path = "test_file_path";

    std::string response = curl_transcribe_audio(file_path, OPENAI_API_KEY);

    // Replace this with expected response
    std::string expected_response = "expected_response";

    ASSERT_EQ(response, expected_response);
}

int main(int argc, char **argv)
{
    SetUpConfig();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}