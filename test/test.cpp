// Third-Party Library Headers
#include <gtest/gtest.h>
#ifdef GMOCK_AVAILABLE
#include <gmock/gmock.h>
#endif
#include <yaml-cpp/yaml.h>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <iomanip>
#include <thread>
#include <chrono>

// Project-Specific Headers
#include "ConfigSingleton.h"
#include "DatabaseManager.h"
#include "fileProcessor.h"
#include "curlHelper.h"
#include "transcriptionProcessor.h"
#include "fasterWhisper.h"
#include "FileData.h"
#include "globalFlags.h"
#include <cstdlib>

// Test environment globals
std::string TEST_OPENAI_API_KEY = "test-api-key";
std::string TEST_DB_PATH = ":memory:";

// Cross-platform temporary directory helper
inline std::string getTempDir() {
#ifdef _WIN32
    const char* temp = std::getenv("TEMP");
    if (!temp) temp = std::getenv("TMP");
    if (!temp) temp = "C:\\Windows\\Temp";
    return std::string(temp) + "\\";
#else
    return "/tmp/";
#endif
}

std::string TEST_CONFIG_PATH = getTempDir() + "test_config.yaml";
std::string TEST_GLOSSARY_PATH = getTempDir() + "test_glossary.json";
std::string TEST_AUDIO_PATH = getTempDir() + "test_audio.mp3";

// Define missing global flag for tests
bool gLocalFlag = false;

// Declaration for helper function from fileProcessor.cpp
extern int generateUnixTimestamp(const std::string &date, const std::string &time);

// Test utilities class
class TestUtils {
public:
    static void createTestConfig(const std::string& path) {
        YAML::Node config;
        config["OPENAI_API_KEY"] = TEST_OPENAI_API_KEY;
        config["DATABASE_PATH"] = TEST_DB_PATH;
        config["DirectoryToMonitor"] = getTempDir() + "test_monitor";
        config["LoopWaitSeconds"] = 5;
        config["MAX_RETRIES"] = 3;
        config["MAX_REQUESTS_PER_MINUTE"] = 60;
        config["ERROR_WINDOW_SECONDS"] = 300;
        config["RATE_LIMIT_WINDOW_SECONDS"] = 60;
        config["MIN_DURATION_SECONDS"] = 1;
        config["DEBUG_CURL_HELPER"] = false;
        config["DEBUG_DATABASE_MANAGER"] = false;
        config["DEBUG_FILE_PROCESSOR"] = false;
        config["DEBUG_MAIN"] = false;
        config["DEBUG_TRANSCRIPTION_PROCESSOR"] = false;
        
        YAML::Node talkgroupFiles;
        YAML::Node talkgroup;
        YAML::Node glossary;
        glossary.push_back(TEST_GLOSSARY_PATH);
        talkgroup["GLOSSARY"] = glossary;
        talkgroupFiles["52198"] = talkgroup;
        config["TALKGROUP_FILES"] = talkgroupFiles;
        
        std::ofstream file(path);
        file << config;
    }
    
    static void createTestGlossary(const std::string& path) {
        std::ofstream file(path);
        file << R"({
  "officer": "police officer",
  "unit": "patrol unit",
  "10-4": "acknowledged"
})";
    }
    
    static void createTestAudioFile(const std::string& path) {
        // Create a minimal MP3-like file for testing
        std::ofstream file(path, std::ios::binary);
        // Write minimal MP3 header
        const unsigned char mp3Header[] = {0xFF, 0xFB, 0x90, 0x00};
        file.write(reinterpret_cast<const char*>(mp3Header), sizeof(mp3Header));
        // Add some dummy data
        for (int i = 0; i < 1000; ++i) {
            file.put(static_cast<char>(i % 256));
        }
    }
    
    static std::string createTestFilename(int year = 2024, int month = 1, int day = 15, 
                                         int hour = 14, int min = 30, int sec = 45,
                                         int talkgroupID = 52198, int radioID = 12345) {
        std::stringstream ss;
        ss << std::setfill('0') << std::setw(4) << year
           << std::setw(2) << month << std::setw(2) << day << "_"
           << std::setw(2) << hour << std::setw(2) << min << std::setw(2) << sec
           << "Test_System__TO_" << talkgroupID << "_FROM_" << radioID << ".mp3";
        return ss.str();
    }
};

// =============================================================================
// CONFIG SINGLETON TESTS
// =============================================================================

class ConfigSingletonTest : public ::testing::Test {
protected:
    void SetUp() override {
        TestUtils::createTestConfig(TEST_CONFIG_PATH);
        TestUtils::createTestGlossary(TEST_GLOSSARY_PATH);
    }
    
    void TearDown() override {
        std::filesystem::remove(TEST_CONFIG_PATH);
        std::filesystem::remove(TEST_GLOSSARY_PATH);
    }
};

TEST_F(ConfigSingletonTest, SingletonPattern) {
    auto& instance1 = ConfigSingleton::getInstance();
    auto& instance2 = ConfigSingleton::getInstance();
    EXPECT_EQ(&instance1, &instance2);
}

TEST_F(ConfigSingletonTest, InitializeFromConfig) {
    YAML::Node config = YAML::LoadFile(TEST_CONFIG_PATH);
    auto& configSingleton = ConfigSingleton::getInstance();
    
    EXPECT_NO_THROW(configSingleton.initialize(config));
    EXPECT_EQ(configSingleton.getOpenAIAPIKey(), TEST_OPENAI_API_KEY);
    EXPECT_EQ(configSingleton.getDatabasePath(), TEST_DB_PATH);
    EXPECT_EQ(configSingleton.getLoopWaitSeconds(), 5);
    EXPECT_EQ(configSingleton.getMaxRetries(), 3);
    EXPECT_FALSE(configSingleton.isDebugMain());
}

TEST_F(ConfigSingletonTest, TalkgroupFilesMapping) {
    YAML::Node config = YAML::LoadFile(TEST_CONFIG_PATH);
    auto& configSingleton = ConfigSingleton::getInstance();
    configSingleton.initialize(config);
    
    const auto& talkgroupFiles = configSingleton.getTalkgroupFiles();
    EXPECT_FALSE(talkgroupFiles.empty());
    
    auto it = talkgroupFiles.find(52198);
    EXPECT_NE(it, talkgroupFiles.end());
    EXPECT_FALSE(it->second.glossaryFiles.empty());
}

// =============================================================================
// DATABASE MANAGER TESTS  
// =============================================================================

class DatabaseManagerTest : public ::testing::Test {
protected:
    std::unique_ptr<DatabaseManager> dbManager;
    
    void SetUp() override {
        dbManager = std::make_unique<DatabaseManager>(":memory:");
        dbManager->createTable();
    }
    
    void TearDown() override {
        dbManager.reset();
    }
};

TEST_F(DatabaseManagerTest, CreateTable) {
    // Table creation is done in SetUp, test that it doesn't throw
    EXPECT_NO_THROW(dbManager->createTable());
}

TEST_F(DatabaseManagerTest, InsertRecording) {
    EXPECT_NO_THROW(dbManager->insertRecording(
        "2024-01-15", "14:30:45", 1705330245, 52198, "NCSHP", 
        12345, "00:05.123", "test.mp3", (getTempDir() + "test.mp3").c_str(), 
        "Test transcription", "{\"12345\":\"Test transcription\"}"
    ));
}

TEST_F(DatabaseManagerTest, InvalidDatabasePath) {
    EXPECT_THROW(DatabaseManager("/invalid/path/db.sqlite"), std::runtime_error);
}

// =============================================================================
// FILE PROCESSOR TESTS
// =============================================================================

class FileProcessorTest : public ::testing::Test {
protected:
    std::string testDir;
    std::string testFilePath;
    
    void SetUp() override {
        testDir = getTempDir() + "test_monitor";
        std::filesystem::create_directories(testDir);
        
        std::string filename = TestUtils::createTestFilename();
        testFilePath = testDir + "/" + filename;
        TestUtils::createTestAudioFile(testFilePath);
    }
    
    void TearDown() override {
        std::filesystem::remove_all(testDir);
    }
};

TEST_F(FileProcessorTest, ParseValidFilename) {
    // Test with production filename format
    std::string filename = "20240115_143045Test_System__TO_52198_FROM_12345.mp3";
    FileData result;
    extractFileInfo(result, filename, "Test transcription");
    
    EXPECT_EQ(result.talkgroupID, 52198);
    EXPECT_EQ(result.radioID, 12345);
    EXPECT_EQ(result.date, "20240115");
    EXPECT_EQ(result.time, "143045");
    EXPECT_EQ(result.filename, filename);
}

TEST_F(FileProcessorTest, FileBeingWrittenCheck) {
    // Test file locking detection
    EXPECT_FALSE(isFileBeingWrittenTo(testFilePath));
    EXPECT_FALSE(isFileLocked(testFilePath));
}

TEST_F(FileProcessorTest, FindAndMoveMp3WithoutTxt) {
    // This test would require more complex setup with actual file operations
    EXPECT_NO_THROW(find_and_move_mp3_without_txt(testDir));
}

// =============================================================================
// TRANSCRIPTION PROCESSOR TESTS
// =============================================================================

class TranscriptionProcessorTest : public ::testing::Test {
protected:
    void SetUp() override {
        TestUtils::createTestGlossary(TEST_GLOSSARY_PATH);
    }
    
    void TearDown() override {
        std::filesystem::remove(TEST_GLOSSARY_PATH);
    }
};

TEST_F(TranscriptionProcessorTest, ReadValidMappingFile) {
    auto result = readMappingFile(TEST_GLOSSARY_PATH);
    EXPECT_FALSE(result.empty());
    EXPECT_EQ(result["officer"], "police officer");
    EXPECT_EQ(result["10-4"], "acknowledged");
}

TEST_F(TranscriptionProcessorTest, ReadInvalidMappingFile) {
    auto result = readMappingFile("/nonexistent/path.json");
    EXPECT_TRUE(result.empty());
}

TEST_F(TranscriptionProcessorTest, ExtractActualTranscription) {
    std::string validJson = R"({"text":"This is a test transcription"})";    
    auto result = extractActualTranscription(validJson);
    EXPECT_EQ(result, "This is a test transcription");
    
    // Test invalid JSON
    auto invalidResult = extractActualTranscription("invalid json");
    EXPECT_TRUE(invalidResult.empty());
}

// Note: getAppropriateFile function not available in current implementation
// These tests are commented out until the function is implemented

/*
TEST_F(TranscriptionProcessorTest, GetAppropriateFileSpecialTalkgroup) {
    auto result = getAppropriateFile(
        52198,
        []() { return "NCSHP_file"; },
        []() { return "default_file"; }
    );
    EXPECT_EQ(result, "NCSHP_file");
}

TEST_F(TranscriptionProcessorTest, GetAppropriateFileDefaultTalkgroup) {
    auto result = getAppropriateFile(
        12345,
        []() { return "NCSHP_file"; },
        []() { return "default_file"; }
    );
    EXPECT_EQ(result, "default_file");
}
*/

TEST_F(TranscriptionProcessorTest, InsertMappings) {
    std::stringstream ss;
    std::unordered_map<std::string, std::string> mappings = {
        {"officer", "police officer"},
        {"unit", "patrol unit"}
    };
    std::string text = "The officer called the unit";
    
    insertMappings(ss, text, mappings);
    std::string result = ss.str();
    
    EXPECT_FALSE(result.empty());
    EXPECT_NE(result.find("officer"), std::string::npos);
    EXPECT_NE(result.find("unit"), std::string::npos);
}

TEST_F(TranscriptionProcessorTest, ParseTalkgroupIDs) {
    auto result = parseTalkgroupIDs("52198");
    EXPECT_EQ(result.size(), 1);
    EXPECT_TRUE(result.count(52198));
    
    auto rangeResult = parseTalkgroupIDs("52198-52200");
    EXPECT_EQ(rangeResult.size(), 3);
    EXPECT_TRUE(rangeResult.count(52198));
    EXPECT_TRUE(rangeResult.count(52199));
    EXPECT_TRUE(rangeResult.count(52200));
}

TEST_F(TranscriptionProcessorTest, GenerateV2Transcription) {
    std::unordered_map<int, TalkgroupFiles> talkgroupFiles;
    TalkgroupFiles files;
    files.glossaryFiles.push_back(TEST_GLOSSARY_PATH);
    talkgroupFiles[52198] = files;
    
    std::string transcription = R"({"text":"The officer is on patrol"})";    
    auto result = generateV2Transcription(transcription, 52198, 12345, talkgroupFiles);
    
    EXPECT_FALSE(result.empty());
    EXPECT_NE(result.find("\"12345\""), std::string::npos);
    EXPECT_NE(result.find("police officer"), std::string::npos);
}

// =============================================================================
// CURL HELPER TESTS
// =============================================================================

class CurlHelperTest : public ::testing::Test {
protected:
    void SetUp() override {
        curl_global_init(CURL_GLOBAL_DEFAULT);
    }
    
    void TearDown() override {
        curl_global_cleanup();
    }
};

TEST_F(CurlHelperTest, WriteCallback) {
    std::string data = "test response data";
    std::string output;
    
    size_t result = WriteCallback(
        const_cast<char*>(data.c_str()), 
        1, 
        data.size(), 
        &output
    );
    
    EXPECT_EQ(result, data.size());
    EXPECT_EQ(output, data);
}

TEST_F(CurlHelperTest, SetupCurlHeaders) {
    CURL* curl = curl_easy_init();
    ASSERT_NE(curl, nullptr);
    
    struct curl_slist* headers = nullptr;
    setupCurlHeaders(curl, headers, TEST_OPENAI_API_KEY);
    
    EXPECT_NE(headers, nullptr);
    
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
}

TEST_F(CurlHelperTest, SetupCurlPostFields) {
    TestUtils::createTestAudioFile(TEST_AUDIO_PATH);
    
    CURL* curl = curl_easy_init();
    ASSERT_NE(curl, nullptr);
    
    curl_mime* mime = nullptr;
    EXPECT_NO_THROW(setupCurlPostFields(curl, mime, TEST_AUDIO_PATH));
    
    if (mime) {
        curl_mime_free(mime);
    }
    curl_easy_cleanup(curl);
    
    std::filesystem::remove(TEST_AUDIO_PATH);
}

// Note: Actual HTTP request tests would require mocking or integration test setup
TEST_F(CurlHelperTest, CurlTranscribeAudioMockTest) {
    // This test would ideally use a mock HTTP server
    // For now, we test that the function handles invalid files gracefully
    // The function throws an exception for non-existent files
    EXPECT_THROW(curl_transcribe_audio("/nonexistent/file.mp3", TEST_OPENAI_API_KEY), std::runtime_error);
}

// =============================================================================
// FASTER WHISPER TESTS
// =============================================================================

class FasterWhisperTest : public ::testing::Test {
protected:
    void SetUp() override {
        TestUtils::createTestAudioFile(TEST_AUDIO_PATH);
    }
    
    void TearDown() override {
        std::filesystem::remove(TEST_AUDIO_PATH);
    }
};

TEST_F(FasterWhisperTest, LocalTranscribeAudio) {
    // This test depends on the Python script being available
    // In a real test environment, we would mock the Python execution
    // For now, just test that the function doesn't crash with a valid file
    std::string result = local_transcribe_audio(TEST_AUDIO_PATH);
    
    // If Python script is missing, result will be "MUCH_BROKEN_very_wow"
    EXPECT_TRUE(result == "MUCH_BROKEN_very_wow" || result.find("{\"text\":")==0);
}

TEST_F(FasterWhisperTest, LocalTranscribeAudioInvalidFile) {
    std::string result = local_transcribe_audio("/nonexistent/file.mp3");
    
    // Should return error marker for invalid file
    EXPECT_TRUE(result == "MUCH_BROKEN_very_wow" || result.empty());
}

// =============================================================================
// FILEDATA TESTS
// =============================================================================

TEST(FileDataTest, StructInitialization) {
    FileData data;
    data.date = "2024-01-15";
    data.time = "14:30:45";
    data.unixtime = 1705330245;
    data.talkgroupID = 52198;
    data.talkgroupName = "NCSHP";
    data.radioID = 12345;
    data.duration = "00:05.123";
    data.filename = "test.mp3";
    data.filepath = getTempDir() + "test.mp3";
    data.transcription = "Test transcription";
    data.v2transcription = "{\"12345\":\"Test transcription\"}";
    
    EXPECT_EQ(data.talkgroupID, 52198);
    EXPECT_EQ(data.radioID, 12345);
    EXPECT_EQ(data.date, "2024-01-15");
}

// =============================================================================
// INTEGRATION TESTS
// =============================================================================

class IntegrationTest : public ::testing::Test {
protected:
    std::unique_ptr<DatabaseManager> dbManager;
    std::string testDir;
    std::string configPath;
    
    void SetUp() override {
        configPath = getTempDir() + "integration_config.yaml";
        testDir = getTempDir() + "integration_test";
        
        TestUtils::createTestConfig(configPath);
        TestUtils::createTestGlossary(TEST_GLOSSARY_PATH);
        std::filesystem::create_directories(testDir);
        
        dbManager = std::make_unique<DatabaseManager>(":memory:");
        dbManager->createTable();
    }
    
    void TearDown() override {
        std::filesystem::remove(configPath);
        std::filesystem::remove(TEST_GLOSSARY_PATH);
        std::filesystem::remove_all(testDir);
        dbManager.reset();
    }
};

TEST_F(IntegrationTest, ConfigToDatabase) {
    // Test loading config and using it with database
    YAML::Node config = YAML::LoadFile(configPath);
    auto& configSingleton = ConfigSingleton::getInstance();
    configSingleton.initialize(config);
    
    // Process a test file and insert into database
    std::string filename = TestUtils::createTestFilename();
    std::string filepath = testDir + "/" + filename;
    TestUtils::createTestAudioFile(filepath);
    
    FileData data = processFile(std::filesystem::path(filepath), testDir, configSingleton.getOpenAIAPIKey());
    
    EXPECT_NO_THROW(dbManager->insertRecording(
        data.date, data.time, data.unixtime, data.talkgroupID, 
        data.talkgroupName, data.radioID, data.duration, 
        data.filename, data.filepath, data.transcription, data.v2transcription
    ));
}

TEST_F(IntegrationTest, EndToEndTranscriptionFlow) {
    // This would test the complete flow from file detection to database storage
    // In practice, this would require mocking external services
    YAML::Node config = YAML::LoadFile(configPath);
    auto& configSingleton = ConfigSingleton::getInstance();
    configSingleton.initialize(config);
    
    std::string filename = TestUtils::createTestFilename();
    std::string filepath = testDir + "/" + filename;
    TestUtils::createTestAudioFile(filepath);
    
    // Since we can't actually call the API in tests, we'll test the components separately
    FileData data;
    data.date = "20240115";
    data.time = "143045";
    data.unixtime = generateUnixTimestamp(data.date, data.time);
    data.talkgroupID = 52198;
    data.radioID = 12345;
    data.filename = filename;
    data.filepath = filepath;
    data.duration = "5.000";
    
    // Generate V2 transcription with glossary
    const auto& talkgroupFiles = configSingleton.getTalkgroupFiles();
    std::string mockTranscription = R"({"text":"Unit officer responding to call"})";
    std::string v2Transcription = generateV2Transcription(
        mockTranscription, data.talkgroupID, data.radioID, talkgroupFiles
    );
    
    EXPECT_FALSE(v2Transcription.empty());
    EXPECT_NE(v2Transcription.find("police officer"), std::string::npos);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    
    // Initialize curl globally for tests
    curl_global_init(CURL_GLOBAL_DEFAULT);
    
    // Run all tests
    int result = RUN_ALL_TESTS();
    
    // Cleanup
    curl_global_cleanup();
    
    return result;
}