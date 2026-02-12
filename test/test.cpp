// Third-Party Library Headers
#include <gtest/gtest.h>
#ifdef GMOCK_AVAILABLE
#include <gmock/gmock.h>
#endif

// Standard Library Headers
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
#include "yamlParser.h"
#include "fasterWhisper.h"
#include "FileData.h"
#include "globalFlags.h"
#include "jsonParser.h"
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
extern int64_t generateUnixTimestamp(const std::string &date, const std::string &time);

// Test utilities class
class TestUtils {
public:
    static void createTestConfig(const std::string& path) {
        YamlNode config;
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
        
        YamlNode talkgroupFiles;
        YamlNode talkgroup;
        YamlNode glossary;
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
    YamlNode config = YamlParser::loadFile(TEST_CONFIG_PATH);
    auto& configSingleton = ConfigSingleton::getInstance();
    
    EXPECT_NO_THROW(configSingleton.initialize(config));
    EXPECT_EQ(configSingleton.getOpenAIAPIKey(), TEST_OPENAI_API_KEY);
    EXPECT_EQ(configSingleton.getDatabasePath(), TEST_DB_PATH);
    EXPECT_EQ(configSingleton.getLoopWaitSeconds(), 5);
    EXPECT_EQ(configSingleton.getMaxRetries(), 3);
    EXPECT_FALSE(configSingleton.isDebugMain());
}

TEST_F(ConfigSingletonTest, TalkgroupFilesMapping) {
    YamlNode config = YamlParser::loadFile(TEST_CONFIG_PATH);
    auto& configSingleton = ConfigSingleton::getInstance();
    
    // Debug: Check if TALKGROUP_FILES was loaded
    ASSERT_TRUE(config.hasKey("TALKGROUP_FILES")) << "Config missing TALKGROUP_FILES";
    
    configSingleton.initialize(config);
    
    const auto& talkgroupFiles = configSingleton.getTalkgroupFiles();
    EXPECT_FALSE(talkgroupFiles.empty()) << "TalkgroupFiles map is empty";
    
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
        12345, 5.123, "test.mp3", (getTempDir() + "test.mp3").c_str(),
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
    
    EXPECT_EQ(result.talkgroupID.get(), 52198);
    EXPECT_EQ(result.radioID.get(), 12345);
    EXPECT_EQ(result.date, "20240115");
    EXPECT_EQ(result.time, "143045");
    EXPECT_EQ(result.filename.get().string(), filename);
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
    auto result = local_transcribe_audio(TEST_AUDIO_PATH);
    
    // If successful, should contain transcription
    // If Python script is missing or fails, should have error
    if (result.has_value()) {
        // Success case - should have JSON response
        EXPECT_TRUE(result.value().find("{\"text\":") == 0 || !result.value().empty());
    } else {
        // Error case - Python script missing or other error
        EXPECT_FALSE(result.error().empty());
    }
}

TEST_F(FasterWhisperTest, LocalTranscribeAudioInvalidFile) {
    auto result = local_transcribe_audio("/nonexistent/file.mp3");
    
    // Should return an error (unexpected) for invalid file
    EXPECT_FALSE(result.has_value());
    if (!result.has_value()) {
        // Verify error message contains expected text
        EXPECT_NE(result.error().find("does not exist"), std::string::npos);
    }
}

// =============================================================================
// FILEDATA TESTS
// =============================================================================

TEST(FileDataTest, StructInitialization) {
    FileData data;
    data.date = "2024-01-15";
    data.time = "14:30:45";
    
    // Set timestamp
    std::tm tm = {};
    std::istringstream ss("20240115143045");
    ss >> std::get_time(&tm, "%Y%m%d%H%M%S");
    data.timestamp = std::chrono::system_clock::from_time_t(std::mktime(&tm));
    
    data.talkgroupID = TalkgroupId(52198);
    data.talkgroupName = "NCSHP";
    data.radioID = RadioId(12345);
    data.duration = Duration(std::chrono::seconds(5));
    data.filename = FilePath(std::filesystem::path("test.mp3"));
    data.filepath = FilePath(std::filesystem::path(getTempDir()) / "test.mp3");
    data.transcription = Transcription("Test transcription");
    data.v2transcription = Transcription("{\"12345\":\"Test transcription\"}");
    
    EXPECT_EQ(data.talkgroupID.get(), 52198);
    EXPECT_EQ(data.radioID.get(), 12345);
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
    YamlNode config = YamlParser::loadFile(configPath);
    auto& configSingleton = ConfigSingleton::getInstance();
    configSingleton.initialize(config);
    
    // Process a test file and insert into database
    std::string filename = TestUtils::createTestFilename();
    std::string filepath = testDir + "/" + filename;
    TestUtils::createTestAudioFile(filepath);
    
    FileData data = processFile(std::filesystem::path(filepath), testDir, configSingleton.getOpenAIAPIKey());
    
    EXPECT_NO_THROW(dbManager->insertRecording(
        data.date, data.time, data.unixtime(), data.talkgroupID.get(),
        data.talkgroupName, data.radioID.get(), static_cast<double>(data.duration.get().count()),
        data.filename.get().string(), data.filepath.get().string(), data.transcription.get(), data.v2transcription.get()
    ));
}

TEST_F(IntegrationTest, EndToEndTranscriptionFlow) {
    // This would test the complete flow from file detection to database storage
    // In practice, this would require mocking external services
    YamlNode config = YamlParser::loadFile(configPath);
    auto& configSingleton = ConfigSingleton::getInstance();
    configSingleton.initialize(config);
    
    std::string filename = TestUtils::createTestFilename();
    std::string filepath = testDir + "/" + filename;
    TestUtils::createTestAudioFile(filepath);
    
    // Since we can't actually call the API in tests, we'll test the components separately
    FileData data;
    data.date = "20240115";
    data.time = "143045";
    // Set timestamp for unixtime() function
    std::tm tm = {};
    std::istringstream ss(data.date + data.time);
    ss >> std::get_time(&tm, "%Y%m%d%H%M%S");
    data.timestamp = std::chrono::system_clock::from_time_t(std::mktime(&tm));
    data.talkgroupID = TalkgroupId(52198);
    data.radioID = RadioId(12345);
    data.filename = FilePath(std::filesystem::path(filename));
    data.filepath = FilePath(std::filesystem::path(filepath));
    data.duration = Duration(std::chrono::seconds(5));
    
    // Generate V2 transcription with glossary
    const auto& talkgroupFiles = configSingleton.getTalkgroupFiles();
    std::string mockTranscription = R"({"text":"Unit officer responding to call"})";
    std::string v2Transcription = generateV2Transcription(
        mockTranscription, data.talkgroupID.get(), data.radioID.get(), talkgroupFiles
    );
    
    EXPECT_FALSE(v2Transcription.empty());
    EXPECT_NE(v2Transcription.find("police officer"), std::string::npos);
}

// =============================================================================
// MULTI-KEY GLOSSARY PARSING TESTS (Issue #28)
// =============================================================================

class MultiKeyGlossaryTest : public ::testing::Test {
protected:
    std::string tempDir;
    std::vector<std::string> createdFiles;

    void SetUp() override {
        tempDir = getTempDir();
    }

    void TearDown() override {
        for (const auto &f : createdFiles) std::filesystem::remove(f);
    }

    std::string writeTempJson(const std::string &name, const std::string &content) {
        std::string path = tempDir + name;
        std::ofstream f(path);
        f << content;
        createdFiles.push_back(path);
        return path;
    }
};

TEST_F(MultiKeyGlossaryTest, ParseMultiKeyFormat) {
    auto path = writeTempJson("mk_glossary.json", R"({
        "GLOSSARY": [
            {"keys": ["10-4", "104"], "value": "Affirmative"},
            {"keys": ["10-20", "1020"], "value": "Location"},
            {"keys": ["officer", "ofc"], "value": "police officer"}
        ]
    })");
    auto entries = JsonParser::parseGlossaryFile(path);

    ASSERT_EQ(entries.size(), 3);
    EXPECT_EQ(entries[0].keys.size(), 2);
    EXPECT_EQ(entries[0].keys[0], "10-4");
    EXPECT_EQ(entries[0].keys[1], "104");
    EXPECT_EQ(entries[0].value, "Affirmative");
    EXPECT_EQ(entries[2].keys[0], "officer");
    EXPECT_EQ(entries[2].value, "police officer");
}

TEST_F(MultiKeyGlossaryTest, FlatFormatReturnsEmpty) {
    auto path = writeTempJson("flat_g.json", R"({"10-4": "Affirmative"})");
    auto entries = JsonParser::parseGlossaryFile(path);
    EXPECT_TRUE(entries.empty()) << "Flat format should return empty vector";
}

TEST_F(MultiKeyGlossaryTest, EmptyGlossaryArray) {
    auto path = writeTempJson("empty_g.json", R"({"GLOSSARY": []})");
    EXPECT_TRUE(JsonParser::parseGlossaryFile(path).empty());
}

TEST_F(MultiKeyGlossaryTest, SingleKeyEntry) {
    auto path = writeTempJson("single_g.json", R"({
        "GLOSSARY": [{"keys": ["ETA"], "value": "estimated time of arrival"}]
    })");
    auto entries = JsonParser::parseGlossaryFile(path);
    ASSERT_EQ(entries.size(), 1);
    EXPECT_EQ(entries[0].keys[0], "ETA");
}

TEST_F(MultiKeyGlossaryTest, NonexistentFileReturnsEmpty) {
    EXPECT_TRUE(JsonParser::parseGlossaryFile("/nonexistent/path.json").empty());
}

TEST_F(MultiKeyGlossaryTest, MissingValueFieldSkipped) {
    auto path = writeTempJson("no_val.json", R"({"GLOSSARY": [{"keys": ["10-4"]}]})");
    EXPECT_TRUE(JsonParser::parseGlossaryFile(path).empty());
}

// =============================================================================
// HYPHEN-STRIPPED VARIANT GENERATION TESTS (Issue #28)
// =============================================================================

class HyphenStrippingTest : public MultiKeyGlossaryTest {};

TEST_F(HyphenStrippingTest, FlatFormatAutoGeneratesStrippedVariants) {
    auto path = writeTempJson("hyp_flat.json", R"({
        "10-4": "Affirmative", "10-20": "Location", "officer": "police officer"
    })");
    auto mappings = readMappingFile(path);

    EXPECT_EQ(mappings["10-4"], "Affirmative");
    EXPECT_EQ(mappings["104"], "Affirmative");
    EXPECT_EQ(mappings["1020"], "Location");
    EXPECT_EQ(mappings.count("officer"), 1); // no hyphen, no extra variant
}

TEST_F(HyphenStrippingTest, MultiKeyFormatAutoGeneratesStrippedVariants) {
    auto path = writeTempJson("hyp_multi.json", R"({
        "GLOSSARY": [{"keys": ["10-7"], "value": "Out-of-Service"}]
    })");
    auto mappings = readMappingFile(path);

    EXPECT_EQ(mappings["10-7"], "Out-of-Service");
    EXPECT_EQ(mappings["107"], "Out-of-Service");
}

TEST_F(HyphenStrippingTest, NoOverwriteExistingKey) {
    auto path = writeTempJson("no_overwrite.json", R"({
        "GLOSSARY": [
            {"keys": ["10-4"], "value": "from hyphen"},
            {"keys": ["104"], "value": "explicit"}
        ]
    })");
    auto mappings = readMappingFile(path);
    EXPECT_EQ(mappings["104"], "explicit");
    EXPECT_EQ(mappings["10-4"], "from hyphen");
}

// =============================================================================
// readMappingFile BACKWARD COMPATIBILITY TESTS (Issue #28)
// =============================================================================

TEST_F(HyphenStrippingTest, ReadsFlatFormat) {
    auto path = writeTempJson("compat_flat.json", R"({
        "10-4": "acknowledged", "officer": "police officer"
    })");
    auto m = readMappingFile(path);
    EXPECT_EQ(m["10-4"], "acknowledged");
    EXPECT_EQ(m["officer"], "police officer");
}

TEST_F(HyphenStrippingTest, ReadsMultiKeyFormat) {
    auto path = writeTempJson("compat_multi.json", R"({
        "GLOSSARY": [
            {"keys": ["10-4", "104"], "value": "acknowledged"},
            {"keys": ["officer", "ofc"], "value": "police officer"}
        ]
    })");
    auto m = readMappingFile(path);
    EXPECT_EQ(m["10-4"], "acknowledged");
    EXPECT_EQ(m["104"], "acknowledged");
    EXPECT_EQ(m["ofc"], "police officer");
}

TEST_F(HyphenStrippingTest, NonexistentFileReturnsEmptyMap) {
    EXPECT_TRUE(readMappingFile("/nonexistent/file.json").empty());
}

// =============================================================================
// extractTalkgroupIdFromFilename TESTS (Issue #12)
// =============================================================================

extern int extractTalkgroupIdFromFilename(const std::string &filename);

TEST(ExtractTalkgroupIdTest, StandardFilename) {
    EXPECT_EQ(extractTalkgroupIdFromFilename(
        "20240115_143045Test_System__TO_52198_FROM_12345.mp3"), 52198);
}

TEST(ExtractTalkgroupIdTest, FilenameWithPPrefix) {
    EXPECT_EQ(extractTalkgroupIdFromFilename(
        "20240115_143045Test_System__TO_P_52198_FROM_12345.mp3"), 52198);
}

TEST(ExtractTalkgroupIdTest, RealWorldFilename) {
    EXPECT_EQ(extractTalkgroupIdFromFilename(
        "20250715_112349North_Carolina_VIPER_Rutherford_T-SPDControl__TO_52324_FROM_2097268.mp3"), 52324);
}

TEST(ExtractTalkgroupIdTest, InvalidFilenameReturnsZero) {
    EXPECT_EQ(extractTalkgroupIdFromFilename("invalid_filename.mp3"), 0);
    EXPECT_EQ(extractTalkgroupIdFromFilename(""), 0);
}

TEST(ExtractTalkgroupIdTest, PGroupWithBrackets) {
    // Real SDRTrunk P-group format: TO_P{patchId}-[{talkgroupId}]
    EXPECT_EQ(extractTalkgroupIdFromFilename(
        "20260208_121343_North-Carolina-VIPER_Rutherford_T-Control__TO_P52197-[52198]_FROM_2151534.mp3"), 52198);
}

TEST(ExtractTalkgroupIdTest, PGroupWithBracketsAnotherRadio) {
    EXPECT_EQ(extractTalkgroupIdFromFilename(
        "20260208_115608_North-Carolina-VIPER_Rutherford_T-Control__TO_P52197-[52198]_FROM_2152290.mp3"), 52198);
}

TEST(ExtractTalkgroupIdTest, NBFMWithoutFrom) {
    // NBFM files have no FROM field
    EXPECT_EQ(extractTalkgroupIdFromFilename(
        "20260208_120926_WR4SC_Greenville_14537NBFM__TO_9969.mp3"), 9969);
}

TEST(ExtractTalkgroupIdTest, NBFMBlueRidge) {
    EXPECT_EQ(extractTalkgroupIdFromFilename(
        "20260208_114210_BlueRidge_Unkn_14661NBFM__TO_9967.mp3"), 9967);
}

TEST(ExtractTalkgroupIdTest, StandardP25DifferentTG) {
    EXPECT_EQ(extractTalkgroupIdFromFilename(
        "20260208_121634_North-Carolina-VIPER_Rutherford_T-Control__TO_41001_FROM_1610018.mp3"), 41001);
}

TEST(ExtractTalkgroupIdTest, VersionSuffixV2) {
    EXPECT_EQ(extractTalkgroupIdFromFilename(
        "20260208_121934_North-Carolina-VIPER__TO_41001_V2_FROM_1612753.mp3"), 41001);
}

TEST(ExtractTalkgroupIdTest, VersionSuffixV2NoFrom) {
    EXPECT_EQ(extractTalkgroupIdFromFilename(
        "20260208_123531_North-Carolina-VIPER__TO_9999_V2.mp3"), 9999);
}

TEST(ExtractTalkgroupIdTest, VersionSuffixV3) {
    EXPECT_EQ(extractTalkgroupIdFromFilename(
        "20260208_123531_North-Carolina-VIPER__TO_9999_V3.mp3"), 9999);
}

TEST(ExtractTalkgroupIdTest, PGroupAloneNoBackets) {
    // TO_P52197 without any brackets
    EXPECT_EQ(extractTalkgroupIdFromFilename(
        "20260208_121343_NC-VIPER__TO_P52197.mp3"), 52197);
}

TEST(ExtractTalkgroupIdTest, PGroupUnderscoreSepSingleBracket) {
    // TO_P52197_[52198] (underscore separator)
    EXPECT_EQ(extractTalkgroupIdFromFilename(
        "20260208_121343_NC-VIPER__TO_P52197_[52198]_FROM_2152734.mp3"), 52198);
}

TEST(ExtractTalkgroupIdTest, PGroupMultiGroupHyphen) {
    // TO_P52197-[52198--51426--56881] (multiple groups, -- separator)
    EXPECT_EQ(extractTalkgroupIdFromFilename(
        "20260208_121343_NC-VIPER__TO_P52197-[52198--51426--56881]_FROM_2499936.mp3"), 52198);
}

TEST(ExtractTalkgroupIdTest, PGroupMultiGroupUnderscore) {
    // TO_P52197_[52198__52199] (multiple groups, __ separator)
    EXPECT_EQ(extractTalkgroupIdFromFilename(
        "20260208_121343_NC-VIPER__TO_P52197_[52198__52199]_FROM_2075060.mp3"), 52198);
}

TEST(ExtractTalkgroupIdTest, PGroupBracketWithV2) {
    // TO_P52197-[52198]_V2_FROM_ (version suffix after bracket)
    EXPECT_EQ(extractTalkgroupIdFromFilename(
        "20260208_121343_NC-VIPER__TO_P52197-[52198]_V2_FROM_2152734.mp3"), 52198);
}

TEST(ExtractTalkgroupIdTest, PGroupManyGroups) {
    // 4+ groups: TO_P52197-[52198--52047--50662--52196]
    EXPECT_EQ(extractTalkgroupIdFromFilename(
        "20260208_121343_NC__TO_P52197-[52198--52047--50662--52196]_FROM_2499843.mp3"), 52198);
}

// =============================================================================
// REAL MP3 FILE TESTS (diverse SDRTrunk filename patterns)
// =============================================================================

static const std::string TEST_DATA_DIR =
    "/home/foxtrot/git/cpp-sdrtrunk-transcriber/test/test_data/";

TEST(RealMP3Test, RealFileExists) {
    std::string path = TEST_DATA_DIR +
        "20250715_112349North_Carolina_VIPER_Rutherford_T-SPDControl__TO_52324_FROM_2097268.mp3";
    ASSERT_TRUE(std::filesystem::exists(path)) << "Real MP3 test file should exist";
    EXPECT_GT(std::filesystem::file_size(path), 1000) << "Should have substantial content";
}

TEST(RealMP3Test, ExtractFileInfoFromRealFilename) {
    std::string fn = "20250715_112349North_Carolina_VIPER_Rutherford_T-SPDControl__TO_52324_FROM_2097268.mp3";
    FileData data;
    extractFileInfo(data, fn, R"({"text":"test transcription"})");
    EXPECT_EQ(data.talkgroupID.get(), 52324);
    EXPECT_EQ(data.radioID.get(), 2097268);
    EXPECT_EQ(data.date, "20250715");
    EXPECT_EQ(data.time, "112349");
    EXPECT_EQ(data.talkgroupName, "North_Carolina_VIPER_Rutherford_T-SPDControl");
}

TEST(RealMP3Test, PGroupWithBrackets_FileExists) {
    std::string path = TEST_DATA_DIR +
        "20260208_121343_North-Carolina-VIPER_Rutherford_T-Control__TO_P52197-[52198]_FROM_2151534.mp3";
    ASSERT_TRUE(std::filesystem::exists(path));
    EXPECT_GT(std::filesystem::file_size(path), 100000) << "P-group file should be large (long speech)";
}

TEST(RealMP3Test, PGroupWithBrackets_ExtractFileInfo) {
    std::string fn = "20260208_121343_North-Carolina-VIPER_Rutherford_T-Control__TO_P52197-[52198]_FROM_2151534.mp3";
    FileData data;
    extractFileInfo(data, fn, R"({"text":"test"})");
    EXPECT_EQ(data.talkgroupID.get(), 52198) << "Should extract bracketed talkgroup from P-group";
    EXPECT_EQ(data.radioID.get(), 2151534);
    EXPECT_EQ(data.date, "20260208");
    EXPECT_EQ(data.time, "121343");
    EXPECT_EQ(data.talkgroupName, "North-Carolina-VIPER_Rutherford_T-Control");
}

TEST(RealMP3Test, NBFMWithoutFrom_FileExists) {
    std::string path = TEST_DATA_DIR +
        "20260208_120926_WR4SC_Greenville_14537NBFM__TO_9969.mp3";
    ASSERT_TRUE(std::filesystem::exists(path));
}

TEST(RealMP3Test, NBFMWithoutFrom_ExtractFileInfo) {
    std::string fn = "20260208_120926_WR4SC_Greenville_14537NBFM__TO_9969.mp3";
    FileData data;
    extractFileInfo(data, fn, R"({"text":"test"})");
    EXPECT_EQ(data.talkgroupID.get(), 9969);
    EXPECT_EQ(data.radioID.get(), 1234567) << "NBFM without FROM should use default radio ID";
    EXPECT_EQ(data.date, "20260208");
    EXPECT_EQ(data.time, "120926");
    EXPECT_EQ(data.talkgroupName, "WR4SC_Greenville_14537NBFM");
}

TEST(RealMP3Test, BlueRidgeNBFM_ExtractFileInfo) {
    std::string fn = "20260208_114210_BlueRidge_Unkn_14661NBFM__TO_9967.mp3";
    FileData data;
    extractFileInfo(data, fn, R"({"text":"test"})");
    EXPECT_EQ(data.talkgroupID.get(), 9967);
    EXPECT_EQ(data.radioID.get(), 1234567) << "NBFM without FROM should use default radio ID";
    EXPECT_EQ(data.talkgroupName, "BlueRidge_Unkn_14661NBFM");
}

TEST(RealMP3Test, ShortFile_FileExists) {
    std::string path = TEST_DATA_DIR +
        "20260208_114511_North-Carolina-VIPER_Rutherford_T-Control__TO_41001_FROM_1610075.mp3";
    ASSERT_TRUE(std::filesystem::exists(path));
    EXPECT_LT(std::filesystem::file_size(path), 5000) << "Short file should be small";
}

TEST(RealMP3Test, StandardP25_ExtractFileInfo) {
    std::string fn = "20260208_121634_North-Carolina-VIPER_Rutherford_T-Control__TO_41001_FROM_1610018.mp3";
    FileData data;
    extractFileInfo(data, fn, R"({"text":"test"})");
    EXPECT_EQ(data.talkgroupID.get(), 41001);
    EXPECT_EQ(data.radioID.get(), 1610018);
    EXPECT_EQ(data.date, "20260208");
    EXPECT_EQ(data.time, "121634");
    EXPECT_EQ(data.talkgroupName, "North-Carolina-VIPER_Rutherford_T-Control");
}

// =============================================================================
// JSON PARSER ENHANCED TESTS (Issue #28)
// =============================================================================

TEST(JsonParserTest, ParseNumberValues) {
    auto r = JsonParser::parseString(R"({"integer": 42, "decimal": 3.14})");
    EXPECT_DOUBLE_EQ(std::get<double>(r["integer"]), 42.0);
    EXPECT_DOUBLE_EQ(std::get<double>(r["decimal"]), 3.14);
}

TEST(JsonParserTest, ParseBooleanValues) {
    auto r = JsonParser::parseString(R"({"yes": true, "no": false})");
    EXPECT_TRUE(std::get<bool>(r["yes"]));
    EXPECT_FALSE(std::get<bool>(r["no"]));
}

TEST(JsonParserTest, ParseNullValue) {
    auto r = JsonParser::parseString(R"({"nothing": null})");
    EXPECT_TRUE(std::holds_alternative<std::nullptr_t>(r["nothing"]));
}

TEST(JsonParserTest, ParseEscapedStrings) {
    auto r = JsonParser::parseString(R"({"e": "line1\nline2\ttab"})");
    EXPECT_EQ(std::get<std::string>(r["e"]), "line1\nline2\ttab");
}

TEST(JsonParserTest, EmptyObject) {
    EXPECT_TRUE(JsonParser::parseString("{}").empty());
}

TEST(JsonParserTest, InvalidJsonThrows) {
    EXPECT_THROW(JsonParser::parseString("not json"), std::runtime_error);
}

// =============================================================================
// PER-TALKGROUP PROMPT TESTS (Issue #12)
// =============================================================================

TEST(PerTalkgroupPromptTest, StructFieldDefaults) {
    TalkgroupFiles files;
    EXPECT_TRUE(files.prompt.empty());
    EXPECT_TRUE(files.glossaryFiles.empty());
}

TEST(PerTalkgroupPromptTest, StructFieldAssignment) {
    TalkgroupFiles files;
    files.prompt = "Police radio dispatch.";
    files.glossaryFiles.push_back("/path/glossary.json");
    EXPECT_EQ(files.prompt, "Police radio dispatch.");
    EXPECT_EQ(files.glossaryFiles.size(), 1);
}

// =============================================================================
// parseTalkgroupIDs EXTENDED TESTS
// =============================================================================

TEST(ParseTalkgroupIDsExtendedTest, CommaSeparatedList) {
    auto ids = parseTalkgroupIDs("28513,41003,41004");
    EXPECT_EQ(ids.size(), 3);
    EXPECT_TRUE(ids.count(28513));
    EXPECT_TRUE(ids.count(41003));
    EXPECT_TRUE(ids.count(41004));
}

TEST(ParseTalkgroupIDsExtendedTest, MixedRangesAndSingles) {
    auto ids = parseTalkgroupIDs("52197-52201,28513");
    EXPECT_EQ(ids.size(), 6);
    EXPECT_TRUE(ids.count(52197));
    EXPECT_TRUE(ids.count(52201));
    EXPECT_TRUE(ids.count(28513));
}

TEST(ParseTalkgroupIDsExtendedTest, RangeOfOne) {
    auto ids = parseTalkgroupIDs("100-100");
    EXPECT_EQ(ids.size(), 1);
    EXPECT_TRUE(ids.count(100));
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