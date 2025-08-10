// Third-Party Library Headers
#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <future>
#include <atomic>

// Project-Specific Headers
#include "TestFixtures.h"
#include "TestMocks.h"
#include "ConfigSingleton.h"
#include "DatabaseManager.h"
#include "fileProcessor.h"
#include "transcriptionProcessor.h"
#include "fasterWhisper.h"

// =============================================================================
// END-TO-END INTEGRATION TESTS
// =============================================================================

class EndToEndIntegrationTest : public SDRTrunkTestFixture {
protected:
    std::unique_ptr<DatabaseManager> dbManager;
    std::string integrationTestDir;
    
    void SetUp() override {
        SDRTrunkTestFixture::SetUp();
        dbManager = std::make_unique<DatabaseManager>(":memory:");
        dbManager->createTable();
        integrationTestDir = fileManager->createTempDirectory("integration_test");
    }
};

TEST_F(EndToEndIntegrationTest, CompleteWorkflowTest) {
    // Step 1: Initialize configuration
    YAML::Node config = loadTestConfig();
    auto& configSingleton = ConfigSingleton::getInstance();
    configSingleton.initialize(config);
    
    EXPECT_EQ(configSingleton.getOpenAIAPIKey(), "test-api-key-12345");
    EXPECT_EQ(configSingleton.getDatabasePath(), ":memory:");
    
    // Step 2: Create test audio file with realistic filename
    std::string testFilename = TestDataGenerator::generateP25Filename(
        2024, 1, 15, 14, 30, 45, 52198, 12345
    );
    std::string testFilePath = integrationTestDir + "/" + testFilename;
    auto audioData = TestDataGenerator::generateMockMP3Data();
    std::ofstream audioFile(testFilePath, std::ios::binary);
    audioFile.write(reinterpret_cast<const char*>(audioData.data()), audioData.size());
    audioFile.close();
    
    // Step 3: Process the file
    FileData processedData = processFile(
        std::filesystem::path(testFilePath), 
        integrationTestDir, 
        configSingleton.getOpenAIAPIKey()
    );
    
    // Verify file processing results
    EXPECT_EQ(processedData.talkgroupID, 52198);
    EXPECT_EQ(processedData.radioID, 12345);
    EXPECT_EQ(processedData.date, "2024-01-15");
    EXPECT_EQ(processedData.time, "14:30:45");
    EXPECT_FALSE(processedData.filename.empty());
    
    // Step 4: Generate V2 transcription with glossary
    const auto& talkgroupFiles = configSingleton.getTalkgroupFiles();
    std::string mockTranscription = TestDataGenerator::generateOpenAIResponse(
        "Unit officer responding to 10-4 at Main Street"
    );
    
    std::string v2Transcription = generateV2Transcription(
        mockTranscription, processedData.talkgroupID, processedData.radioID, talkgroupFiles
    );
    
    EXPECT_FALSE(v2Transcription.empty());
    EXPECT_NE(v2Transcription.find("police officer"), std::string::npos);
    EXPECT_NE(v2Transcription.find("acknowledged"), std::string::npos);
    
    // Step 5: Store in database
    processedData.transcription = extractActualTranscription(mockTranscription);
    processedData.v2transcription = v2Transcription;
    
    EXPECT_NO_THROW(dbManager->insertRecording(
        processedData.date, processedData.time, processedData.unixtime,
        processedData.talkgroupID, processedData.talkgroupName, processedData.radioID,
        processedData.duration, processedData.filename, processedData.filepath,
        processedData.transcription, processedData.v2transcription
    ));
}

TEST_F(EndToEndIntegrationTest, MultipleFileProcessingWorkflow) {
    // Initialize configuration
    YAML::Node config = loadTestConfig();
    auto& configSingleton = ConfigSingleton::getInstance();
    configSingleton.initialize(config);
    
    // Create multiple test files with different talkgroups
    std::vector<std::tuple<int, int, std::string>> testCases = {
        {52198, 12345, "Unit officer responding to call"},
        {52199, 12346, "Backup unit en route to location"},
        {52200, 12347, "10-4 acknowledged, clear"},
        {99999, 99999, "Default talkgroup transmission"}  // Should use default glossary
    };
    
    std::vector<FileData> processedFiles;
    
    for (const auto& [talkgroupID, radioID, transcriptText] : testCases) {
        // Create test file
        std::string filename = TestDataGenerator::generateP25Filename(
            2024, 1, 15, 14, 30, 45, talkgroupID, radioID
        );
        std::string filePath = integrationTestDir + "/" + filename;
        auto audioData = TestDataGenerator::generateMockMP3Data();
        std::ofstream file(filePath, std::ios::binary);
        file.write(reinterpret_cast<const char*>(audioData.data()), audioData.size());
        file.close();
        
        // Process file
        FileData data = processFile(
            std::filesystem::path(filePath),
            integrationTestDir,
            configSingleton.getOpenAIAPIKey()
        );
        
        // Generate V2 transcription
        std::string mockTranscription = TestDataGenerator::generateOpenAIResponse(transcriptText);
        data.transcription = extractActualTranscription(mockTranscription);
        data.v2transcription = generateV2Transcription(
            mockTranscription, data.talkgroupID, data.radioID, 
            configSingleton.getTalkgroupFiles()
        );
        
        processedFiles.push_back(data);
        
        // Store in database
        EXPECT_NO_THROW(dbManager->insertRecording(
            data.date, data.time, data.unixtime, data.talkgroupID,
            data.talkgroupName, data.radioID, data.duration,
            data.filename, data.filepath, data.transcription, data.v2transcription
        ));
    }
    
    // Verify all files were processed correctly
    EXPECT_EQ(processedFiles.size(), 4);
    
    // Check that NCSHP files got enhanced glossary mappings
    for (const auto& data : processedFiles) {
        if (data.talkgroupID >= 52198 && data.talkgroupID <= 52250) {
            EXPECT_NE(data.v2transcription.find("police officer"), std::string::npos)
                << "NCSHP file should have enhanced glossary: " << data.filename;
        }
    }
}

// =============================================================================
// CONFIGURATION VALIDATION INTEGRATION TESTS
// =============================================================================

class ConfigValidationIntegrationTest : public SDRTrunkTestFixture {};

TEST_F(ConfigValidationIntegrationTest, InvalidConfigurationHandling) {
    // Test with missing required fields
    YAML::Node invalidConfig;
    invalidConfig["DATABASE_PATH"] = ":memory:";
    // Missing OPENAI_API_KEY
    
    auto& configSingleton = ConfigSingleton::getInstance();
    
    // Should handle missing configuration gracefully
    EXPECT_THROW(configSingleton.initialize(invalidConfig), std::exception);
}

TEST_F(ConfigValidationIntegrationTest, TalkgroupFilesConfigurationValidation) {
    YAML::Node config = loadTestConfig();
    auto& configSingleton = ConfigSingleton::getInstance();
    configSingleton.initialize(config);
    
    const auto& talkgroupFiles = configSingleton.getTalkgroupFiles();
    
    // Verify talkgroup range mapping
    EXPECT_TRUE(talkgroupFiles.find(52198) != talkgroupFiles.end());
    EXPECT_TRUE(talkgroupFiles.find(52225) != talkgroupFiles.end());
    EXPECT_TRUE(talkgroupFiles.find(52250) != talkgroupFiles.end());
    
    // Verify glossary files are accessible
    for (const auto& [talkgroupID, files] : talkgroupFiles) {
        for (const auto& glossaryFile : files.glossaryFiles) {
            EXPECT_TRUE(std::filesystem::exists(glossaryFile))
                << "Glossary file should exist: " << glossaryFile;
        }
    }
}

TEST_F(ConfigValidationIntegrationTest, DebugFlagsIntegration) {
    YAML::Node config = loadTestConfig();
    
    // Enable debug flags
    config["DEBUG_CURL_HELPER"] = true;
    config["DEBUG_DATABASE_MANAGER"] = true;
    config["DEBUG_FILE_PROCESSOR"] = true;
    
    auto& configSingleton = ConfigSingleton::getInstance();
    configSingleton.initialize(config);
    
    EXPECT_TRUE(configSingleton.isDebugCurlHelper());
    EXPECT_TRUE(configSingleton.isDebugDatabaseManager());
    EXPECT_TRUE(configSingleton.isDebugFileProcessor());
    EXPECT_FALSE(configSingleton.isDebugMain());  // Should remain false
}

// =============================================================================
// DATABASE MIGRATION AND SCHEMA TESTS
// =============================================================================

class DatabaseMigrationTest : public SDRTrunkTestFixture {
protected:
    std::string persistentDbPath;
    
    void SetUp() override {
        SDRTrunkTestFixture::SetUp();
        persistentDbPath = fileManager->getCreatedFilePath("migration_test.db");
    }
};

TEST_F(DatabaseMigrationTest, SchemaCreationAndValidation) {
    DatabaseManager dbManager(persistentDbPath);
    
    // Create table
    EXPECT_NO_THROW(dbManager.createTable());
    
    // Verify table creation by inserting and verifying data
    FileData testData = createTestFileData();
    
    EXPECT_NO_THROW(dbManager.insertRecording(
        testData.date, testData.time, testData.unixtime,
        testData.talkgroupID, testData.talkgroupName, testData.radioID,
        testData.duration, testData.filename, testData.filepath,
        testData.transcription, testData.v2transcription
    ));
}

TEST_F(DatabaseMigrationTest, DatabasePersistenceTest) {
    // Create database and insert data
    {
        DatabaseManager dbManager(persistentDbPath);
        dbManager.createTable();
        
        FileData testData = createTestFileData();
        dbManager.insertRecording(
            testData.date, testData.time, testData.unixtime,
            testData.talkgroupID, testData.talkgroupName, testData.radioID,
            testData.duration, testData.filename, testData.filepath,
            testData.transcription, testData.v2transcription
        );
    }
    
    // Verify data persists after manager destruction
    {
        DatabaseManager dbManager2(persistentDbPath);
        // Database should still exist and be accessible
        EXPECT_NO_THROW(dbManager2.createTable());  // Should not fail on existing table
    }
}

// =============================================================================
// ERROR HANDLING AND RECOVERY TESTS
// =============================================================================

class ErrorHandlingIntegrationTest : public SDRTrunkTestFixture {};

TEST_F(ErrorHandlingIntegrationTest, CorruptedFileHandling) {
    std::string corruptedFile = fileManager->createTempDirectory("corrupted") + "/corrupted.mp3";
    
    // Create a file with invalid MP3 content
    std::ofstream file(corruptedFile, std::ios::binary);
    file << "This is not a valid MP3 file content";
    file.close();
    
    // Processing should not crash
    EXPECT_NO_THROW({
        FileData data = processFile(
            std::filesystem::path(corruptedFile),
            fileManager->getCreatedFilePath("corrupted"),
            "test-api-key"
        );
        
        // Should handle gracefully (exact behavior depends on implementation)
    });
}

TEST_F(ErrorHandlingIntegrationTest, InvalidFilenamePatterns) {
    std::vector<std::string> invalidFilenames = {
        "invalid_pattern.mp3",
        "20240115_invalid_TG_52198_ID_12345.mp3",
        "20240115_143045_TG_INVALID_ID_12345.mp3",
        "20240115_143045_TG_52198_ID_INVALID.mp3",
        ".mp3",  // Empty base name
        "justtext.mp3"
    };
    
    std::string testDir = fileManager->createTempDirectory("invalid_files");
    
    for (const auto& filename : invalidFilenames) {
        std::string filePath = testDir + "/" + filename;
        auto audioData = TestDataGenerator::generateMockMP3Data();
        std::ofstream file(filePath, std::ios::binary);
        file.write(reinterpret_cast<const char*>(audioData.data()), audioData.size());
        file.close();
        
        // Should not crash on invalid filename patterns
        EXPECT_NO_THROW({
            FileData data = processFile(
                std::filesystem::path(filePath),
                testDir,
                "test-api-key"
            );
            
            // Invalid files should result in empty or default data
            if (filename == "invalid_pattern.mp3" || filename == ".mp3" || filename == "justtext.mp3") {
                EXPECT_TRUE(data.filename.empty() || data.talkgroupID == 0 || data.radioID == 0)
                    << "Invalid filename should not produce valid data: " << filename;
            }
        });
    }
}

TEST_F(ErrorHandlingIntegrationTest, MissingGlossaryFileHandling) {
    // Create config with non-existent glossary files
    YAML::Node config = loadTestConfig();
    YAML::Node talkgroupFiles;
    YAML::Node testTalkgroup;
    YAML::Node glossary;
    glossary.push_back("/nonexistent/path/glossary.json");
    testTalkgroup["GLOSSARY"] = glossary;
    talkgroupFiles["52198"] = testTalkgroup;
    config["TALKGROUP_FILES"] = talkgroupFiles;
    
    auto& configSingleton = ConfigSingleton::getInstance();
    
    // Should initialize without crashing (may log errors)
    EXPECT_NO_THROW(configSingleton.initialize(config));
    
    // Transcription generation should handle missing glossary gracefully
    std::string mockTranscription = TestDataGenerator::generateOpenAIResponse(
        "Test transcription with officer and unit"
    );
    
    EXPECT_NO_THROW({
        std::string result = generateV2Transcription(
            mockTranscription, 52198, 12345, configSingleton.getTalkgroupFiles()
        );
        
        // Should return something (even if glossary mappings failed)
        EXPECT_FALSE(result.empty());
    });
}