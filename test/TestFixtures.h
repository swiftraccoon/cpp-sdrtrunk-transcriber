#pragma once

// Standard Library Headers
#include <string>
#include <fstream>
#include <filesystem>
#include <vector>
#include <memory>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <cstdlib>

// Third-Party Library Headers
#include <gtest/gtest.h>

// Project-Specific Headers
#include "FileData.h"
#include "yamlParser.h"
#include "TestMocks.h"

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

// =============================================================================
// TEST DATA GENERATORS
// =============================================================================

class TestDataGenerator {
public:
    // Generate realistic P25 filename patterns
    static std::string generateP25Filename(
        int year = 2024, int month = 1, int day = 15,
        int hour = 14, int min = 30, int sec = 45,
        int talkgroupID = 52198, int radioID = 12345
    ) {
        std::stringstream ss;
        ss << std::setfill('0') << std::setw(4) << year
           << std::setw(2) << month << std::setw(2) << day << "_"
           << std::setw(2) << hour << std::setw(2) << min << std::setw(2) << sec
           << "Test_System__TO_" << talkgroupID << "_FROM_" << radioID << ".mp3";
        return ss.str();
    }
    
    // Generate various filename patterns for edge case testing
    static std::vector<std::string> generateFilenameVariations() {
        return {
            "20240115_143045Test_System__TO_52198_FROM_12345.mp3",  // Standard format
            "20231201_000000Test_System__TO_1_FROM_999999.mp3",     // Edge values
            "20241231_235959Test_System__TO_99999_FROM_1.mp3",      // Year/time boundaries
            "20240229_120000Test_System__TO_52198_FROM_12345.mp3",  // Leap year
            "invalid_filename.mp3",                                  // Invalid format
            "20240115_143045Test_System__TO_ABC_FROM_12345.mp3",    // Non-numeric talkgroup
            "20240115_143045Test_System__TO_52198_FROM_XYZ.mp3",    // Non-numeric radio ID
            "20240115_143045Test_System__TO_P_52198_FROM_12345.mp3", // With P_ prefix
            ""  // Empty filename
        };
    }
    
    // Generate test configuration YAML
    static YamlNode generateTestConfig() {
        YamlNode config;
        config["OPENAI_API_KEY"] = "test-api-key-12345";
        config["DATABASE_PATH"] = ":memory:";
        config["DirectoryToMonitor"] = getTempDir() + "test_monitor";
        config["LoopWaitSeconds"] = 5;
        config["MAX_RETRIES"] = 3;
        config["MAX_REQUESTS_PER_MINUTE"] = 60;
        config["ERROR_WINDOW_SECONDS"] = 300;
        config["RATE_LIMIT_WINDOW_SECONDS"] = 60;
        config["MIN_DURATION_SECONDS"] = 1;
        
        // Debug flags
        config["DEBUG_CURL_HELPER"] = false;
        config["DEBUG_DATABASE_MANAGER"] = false;
        config["DEBUG_FILE_PROCESSOR"] = false;
        config["DEBUG_MAIN"] = false;
        config["DEBUG_TRANSCRIPTION_PROCESSOR"] = false;
        
        // Talkgroup files configuration
        YamlNode talkgroupFiles;
        
        // NCSHP configuration
        YamlNode ncshpConfig;
        YamlNode ncshpGlossary;
        ncshpGlossary.push_back(getTempDir() + "ncshp_glossary.json");
        ncshpGlossary.push_back(getTempDir() + "law_enforcement.json");
        ncshpConfig["GLOSSARY"] = ncshpGlossary;
        talkgroupFiles["52198-52250"] = ncshpConfig;
        
        // Default configuration - use specific range instead of wildcard
        YamlNode defaultConfig;
        YamlNode defaultGlossary;
        defaultGlossary.push_back(getTempDir() + "default_glossary.json");
        defaultConfig["GLOSSARY"] = defaultGlossary;
        talkgroupFiles["99999"] = defaultConfig;  // Single default talkgroup instead of wildcard
        
        config["TALKGROUP_FILES"] = talkgroupFiles;
        
        return config;
    }
    
    // Generate test glossary files
    static std::string generateNCSHPGlossary() {
        return R"({
  "10-4": "acknowledged",
  "10-8": "in service",
  "10-20": "location",
  "10-76": "en route",
  "officer": "police officer",
  "unit": "patrol unit",
  "suspect": "individual of interest",
  "victim": "complainant",
  "ETA": "estimated time of arrival",
  "BOL": "be on the lookout",
  "APB": "all points bulletin",
  "DUI": "driving under the influence",
  "MVA": "motor vehicle accident",
  "backup": "additional units",
  "clear": "available for calls"
})";}
    
    static std::string generateLawEnforcementGlossary() {
        return R"({
  "perp": "perpetrator",
  "vic": "victim", 
  "wit": "witness",
  "CI": "confidential informant",
  "UC": "undercover",
  "PC": "probable cause",
  "RO": "responding officer",
  "IC": "incident commander",
  "EMS": "emergency medical services",
  "FD": "fire department",
  "Code 1": "routine response",
  "Code 2": "urgent response",
  "Code 3": "emergency response with lights and siren"
})";}
    
    static std::string generateDefaultGlossary() {
        return R"({
  "copy": "understood",
  "negative": "no",
  "affirmative": "yes",
  "standby": "wait",
  "roger": "received",
  "over": "end transmission",
  "out": "end communication"
})";}
    
    // Generate test FileData structures
    static FileData generateTestFileData(
        int talkgroupID = 52198, 
        int radioID = 12345,
        const std::string& transcription = "Test transcription"
    ) {
        FileData data;
        data.date = "2024-01-15";
        data.time = "14:30:45";
        // Set timestamp
        std::tm tm = {};
        std::istringstream ss(data.date + data.time);
        ss >> std::get_time(&tm, "%Y-%m-%d%H:%M:%S");
        data.timestamp = std::chrono::system_clock::from_time_t(std::mktime(&tm));
        data.talkgroupID = TalkgroupId(talkgroupID);
        data.talkgroupName = (talkgroupID >= 52198 && talkgroupID <= 52250) ? "NCSHP" : "Unknown";
        data.radioID = RadioId(radioID);
        data.duration = Duration(std::chrono::seconds(5));
        auto fname = generateP25Filename(2024, 1, 15, 14, 30, 45, talkgroupID, radioID);
        data.filename = FilePath(std::filesystem::path(fname));
        data.filepath = FilePath(std::filesystem::path(getTempDir()) / "test_audio" / fname);
        data.transcription = Transcription(transcription);
        data.v2transcription = Transcription("{\"" + std::to_string(radioID) + "\":\"" + transcription + "\"}");
        return data;
    }
    
    // Get real MP3 file content from test file - ensures valid MP3 for all tests
    static std::vector<unsigned char> generateMockMP3Data() {
        // Use static to cache the data - load once, use many times
        static std::vector<unsigned char> cachedData;
        static bool dataLoaded = false;
        
        if (!dataLoaded) {
            // Use the real MP3 file provided in test directory
            std::string realMp3Path = "/home/foxtrot/git/cpp-sdrtrunk-transcriber/test/test_data/20250715_112349North_Carolina_VIPER_Rutherford_T-SPDControl__TO_52324_FROM_2097268.mp3";
            
            std::ifstream file(realMp3Path, std::ios::binary | std::ios::ate);
            if (file.is_open()) {
                std::streamsize size = file.tellg();
                file.seekg(0, std::ios::beg);
                
                cachedData.resize(size);
                if (file.read(reinterpret_cast<char*>(cachedData.data()), size)) {
                    dataLoaded = true;
                }
            }
            
            if (!dataLoaded) {
                // If real file not found, log error but continue with empty data
                // Tests will fail gracefully with new error handling
                std::cerr << "Warning: Test MP3 file not found at: " << realMp3Path << std::endl;
                std::cerr << "Tests may fail if they require valid MP3 data." << std::endl;
                // Return minimal valid MP3 structure that won't crash ffprobe
                cachedData = {0xFF, 0xFB, 0x90, 0x00}; // Minimal MP3 header
                dataLoaded = true;
            }
        }
        
        return cachedData;
    }
    
    // Generate various test transcription responses
    static std::string generateOpenAIResponse(const std::string& text) {
        return "{\n  \"text\": \"" + text + "\"\n}";
    }
    
    static std::string generateErrorResponse(const std::string& error, const std::string& type = "invalid_request_error") {
        return "{\n  \"error\": {\n    \"message\": \"" + error + "\",\n    \"type\": \"" + type + "\"\n  }\n}";
    }
};

// =============================================================================
// TEST FILE MANAGER
// =============================================================================

class TestFileManager {
private:
    std::vector<std::string> createdFiles;
    std::vector<std::string> createdDirectories;
    
public:
    ~TestFileManager() {
        cleanup();
    }
    
    std::string createTempFile(const std::string& filename, const std::string& content) {
        std::string fullPath = getTempDir() + filename;
        std::ofstream file(fullPath);
        file << content;
        file.close();
        createdFiles.push_back(fullPath);
        return fullPath;
    }
    
    std::string createTempBinaryFile(const std::string& filename, const std::vector<unsigned char>& data) {
        std::string fullPath = getTempDir() + filename;
        std::ofstream file(fullPath, std::ios::binary);
        file.write(reinterpret_cast<const char*>(data.data()), data.size());
        file.close();
        createdFiles.push_back(fullPath);
        return fullPath;
    }
    
    std::string createTempDirectory(const std::string& dirname) {
        std::string fullPath = getTempDir() + dirname;
        std::filesystem::create_directories(fullPath);
        createdDirectories.push_back(fullPath);
        return fullPath;
    }
    
    void cleanup() {
        for (const auto& file : createdFiles) {
            std::filesystem::remove(file);
        }
        for (const auto& dir : createdDirectories) {
            std::filesystem::remove_all(dir);
        }
        createdFiles.clear();
        createdDirectories.clear();
    }
    
    std::string getCreatedFilePath(const std::string& filename) const {
        return getTempDir() + filename;
    }
};

// =============================================================================
// TEST FIXTURES BASE CLASS
// =============================================================================

class SDRTrunkTestFixture : public ::testing::Test {
protected:
    std::unique_ptr<TestFileManager> fileManager;
    std::string testConfigPath;
    std::string testGlossaryPath;
    std::string testAudioPath;
    std::string testDirectory;
    
    void SetUp() override {
        fileManager = std::make_unique<TestFileManager>();
        
        // Create test directory structure
        testDirectory = fileManager->createTempDirectory("sdrtrunk_test");
        
        // Create test configuration
        YamlNode config = TestDataGenerator::generateTestConfig();
        std::stringstream configStream;
        configStream << config;
        testConfigPath = fileManager->createTempFile("test_config.yaml", configStream.str());
        
        // Create test glossary files
        std::string ncshpGlossary = TestDataGenerator::generateNCSHPGlossary();
        fileManager->createTempFile("ncshp_glossary.json", ncshpGlossary);
        
        std::string lawEnforcementGlossary = TestDataGenerator::generateLawEnforcementGlossary();
        fileManager->createTempFile("law_enforcement.json", lawEnforcementGlossary);
        
        std::string defaultGlossary = TestDataGenerator::generateDefaultGlossary();
        testGlossaryPath = fileManager->createTempFile("default_glossary.json", defaultGlossary);
        
        // Create test audio file
        auto audioData = TestDataGenerator::generateMockMP3Data();
        std::string audioFilename = TestDataGenerator::generateP25Filename();
        testAudioPath = fileManager->createTempBinaryFile(audioFilename, audioData);
    }
    
    void TearDown() override {
        fileManager.reset();  // Automatically cleans up files
        
        // Reset dependency injector
        TestDependencyInjector::getInstance().reset();
    }
    
    // Helper methods for common test operations
    YamlNode loadTestConfig() {
        return YamlParser::loadFile(testConfigPath);
    }
    
    std::string createTestAudioFile(int talkgroupID, int radioID) {
        std::string filename = TestDataGenerator::generateP25Filename(
            2024, 1, 15, 14, 30, 45, talkgroupID, radioID
        );
        auto audioData = TestDataGenerator::generateMockMP3Data();
        return fileManager->createTempBinaryFile(filename, audioData);
    }
    
    FileData createTestFileData(int talkgroupID = 52198, int radioID = 12345) {
        return TestDataGenerator::generateTestFileData(talkgroupID, radioID);
    }
};

// =============================================================================
// SPECIALIZED TEST FIXTURES
// =============================================================================

class ConfigSingletonTestFixture : public SDRTrunkTestFixture {
protected:
    void SetUp() override {
        SDRTrunkTestFixture::SetUp();
        // Additional setup specific to ConfigSingleton tests
    }
};

class DatabaseTestFixture : public SDRTrunkTestFixture {
protected:
    std::string dbPath;
    
    void SetUp() override {
        SDRTrunkTestFixture::SetUp();
        dbPath = ":memory:";  // Use in-memory database for tests
    }
};

class FileProcessorTestFixture : public SDRTrunkTestFixture {
protected:
    std::string monitorDirectory;
    
    void SetUp() override {
        SDRTrunkTestFixture::SetUp();
        monitorDirectory = fileManager->createTempDirectory("monitor_test");
    }
    
    std::string createTestFileInMonitorDir(int talkgroupID, int radioID) {
        std::string filename = TestDataGenerator::generateP25Filename(
            2024, 1, 15, 14, 30, 45, talkgroupID, radioID
        );
        std::string fullPath = monitorDirectory + "/" + filename;
        auto audioData = TestDataGenerator::generateMockMP3Data();
        std::ofstream file(fullPath, std::ios::binary);
        file.write(reinterpret_cast<const char*>(audioData.data()), audioData.size());
        return fullPath;
    }
};

#ifdef GMOCK_AVAILABLE
class CurlHelperTestFixture : public SDRTrunkTestFixture {
protected:
    std::shared_ptr<MockCurl> mockCurl;
    
    void SetUp() override {
        SDRTrunkTestFixture::SetUp();
        mockCurl = std::make_shared<MockCurl>();
        TestDependencyInjector::getInstance().setCurlMock(mockCurl);
    }
};
#else
// Simplified version without mocking when GMock is not available
class CurlHelperTestFixture : public SDRTrunkTestFixture {
protected:
    void SetUp() override {
        SDRTrunkTestFixture::SetUp();
    }
};
#endif

class TranscriptionTestFixture : public SDRTrunkTestFixture {
protected:
    std::string mockTranscriptionText;
    
    void SetUp() override {
        SDRTrunkTestFixture::SetUp();
        mockTranscriptionText = "Unit 123 responding to 10-20 at Main Street, officer en route with ETA 5 minutes";
    }
    
    std::string getMockOpenAIResponse() {
        return TestDataGenerator::generateOpenAIResponse(mockTranscriptionText);
    }
};

// =============================================================================
// PERFORMANCE TEST FIXTURES
// =============================================================================

class PerformanceTestFixture : public SDRTrunkTestFixture {
protected:
    std::chrono::high_resolution_clock::time_point startTime;
    
    void startTimer() {
        startTime = std::chrono::high_resolution_clock::now();
    }
    
    double getElapsedMilliseconds() {
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        return duration.count() / 1000.0;  // Convert to milliseconds
    }
    
    void expectPerformanceUnder(double maxMilliseconds, const std::string& operationName) {
        double elapsed = getElapsedMilliseconds();
        EXPECT_LT(elapsed, maxMilliseconds) 
            << operationName << " took " << elapsed << "ms, expected < " << maxMilliseconds << "ms";
    }
};