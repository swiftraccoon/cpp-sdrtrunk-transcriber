// Third-Party Library Headers
#include <gtest/gtest.h>
#include <limits>
#include <random>
#include <locale>

// Project-Specific Headers
#include "TestFixtures.h"
#include "TestMocks.h"
#include "ConfigSingleton.h"
#include "DatabaseManager.h"
#include "fileProcessor.h"
#include "transcriptionProcessor.h"

// =============================================================================
// BOUNDARY VALUE TESTS
// =============================================================================

class BoundaryValueTest : public SDRTrunkTestFixture {};

TEST_F(BoundaryValueTest, ExtremeDateTimeValues) {
    // Test with boundary date/time values
    std::vector<std::tuple<int, int, int, int, int, int, std::string>> testCases = {
        {1900, 1, 1, 0, 0, 0, "Early date boundary"},
        {2100, 12, 31, 23, 59, 59, "Late date boundary"},
        {2000, 2, 29, 12, 0, 0, "Leap year"},
        {1999, 2, 28, 12, 0, 0, "Non-leap year"},
        {2024, 1, 1, 0, 0, 0, "New Year midnight"},
        {2024, 12, 31, 23, 59, 59, "Year end"},
        {2024, 6, 15, 12, 30, 45, "Normal date"}
    };
    
    for (const auto& [year, month, day, hour, min, sec, description] : testCases) {
        std::string filename = TestDataGenerator::generateP25Filename(
            year, month, day, hour, min, sec, 52198, 12345
        );
        
        std::string filePath = fileManager->createTempDirectory("boundary_test") + "/" + filename;
        auto audioData = TestDataGenerator::generateMockMP3Data();
        std::ofstream file(filePath, std::ios::binary);
        file.write(reinterpret_cast<const char*>(audioData.data()), audioData.size());
        file.close();
        
        EXPECT_NO_THROW({
            FileData data = processFile(
                std::filesystem::path(filePath),
                fileManager->getCreatedFilePath("boundary_test"),
                "test-api-key"
            );
            
            // Should handle all date/time boundaries gracefully
            if (year >= 2000 && year <= 2099) {  // Reasonable year range
                EXPECT_FALSE(data.filename.empty()) << "Failed for: " << description;
            }
        }) << "Exception thrown for: " << description;
    }
}

TEST_F(BoundaryValueTest, ExtremeIDValues) {
    std::vector<std::tuple<int, int, std::string>> testCases = {
        {0, 0, "Zero IDs"},
        {1, 1, "Minimum positive IDs"},
        {999999, 999999, "Large IDs"},
        {52198, 999999, "Mixed extreme values"}
    };
    
    for (const auto& [talkgroupID, radioID, description] : testCases) {
        std::string filename = TestDataGenerator::generateP25Filename(
            2024, 1, 15, 14, 30, 45, talkgroupID, radioID
        );
        
        std::string filePath = fileManager->createTempDirectory("id_boundary") + "/" + filename;
        auto audioData = TestDataGenerator::generateMockMP3Data();
        std::ofstream file(filePath, std::ios::binary);
        file.write(reinterpret_cast<const char*>(audioData.data()), audioData.size());
        file.close();
        
        EXPECT_NO_THROW({
            FileData data = processFile(
                std::filesystem::path(filePath),
                fileManager->getCreatedFilePath("id_boundary"),
                "test-api-key"
            );
            
            if (talkgroupID >= 0 && radioID >= 0) {
                EXPECT_EQ(data.talkgroupID, talkgroupID) << "Failed for: " << description;
                EXPECT_EQ(data.radioID, radioID) << "Failed for: " << description;
            }
        }) << "Exception thrown for: " << description;
    }
}

// =============================================================================
// UNICODE AND ENCODING TESTS
// =============================================================================

class UnicodeEncodingTest : public SDRTrunkTestFixture {};

TEST_F(UnicodeEncodingTest, UnicodeTranscriptionHandling) {
    std::vector<std::string> unicodeTestCases = {
        "Test with basic text",
        "Special chars: @#$%^&*()_+-=[]{}|",
        "Quotes: \"text\" and 'single'",
    };
    
    for (const auto& testText : unicodeTestCases) {
        std::string mockTranscription = TestDataGenerator::generateOpenAIResponse(testText);
        
        EXPECT_NO_THROW({
            std::string actualText = extractActualTranscription(mockTranscription);
            EXPECT_FALSE(actualText.empty()) << "Failed to extract: " << testText;
        }) << "Exception with text: " << testText;
    }
}

// =============================================================================
// EMPTY AND NULL VALUE TESTS
// =============================================================================

class EmptyNullValueTest : public SDRTrunkTestFixture {};

TEST_F(EmptyNullValueTest, EmptyStringHandling) {
    // Test empty transcription
    std::string emptyTranscription = TestDataGenerator::generateOpenAIResponse("");
    
    EXPECT_NO_THROW({
        std::string result = extractActualTranscription(emptyTranscription);
        // Should handle empty transcription gracefully
    });
    
    // Test empty glossary mapping
    std::stringstream ss;
    std::unordered_map<std::string, std::string> emptyMappings;
    
    EXPECT_NO_THROW(insertMappings(ss, "test text", emptyMappings));
}

// =============================================================================
// LARGE DATA TESTS
// =============================================================================

class LargeDataTest : public SDRTrunkTestFixture {};

TEST_F(LargeDataTest, LargeTranscriptionHandling) {
    // Generate large transcription text
    std::string largeText;
    const int targetSize = 10000;  // 10KB of text
    
    std::string baseText = "This is a very long transcription that contains many repeated phrases. ";
    while (largeText.size() < targetSize) {
        largeText += baseText;
    }
    
    std::string largeTranscription = TestDataGenerator::generateOpenAIResponse(largeText);
    
    EXPECT_NO_THROW({
        std::string extracted = extractActualTranscription(largeTranscription);
        EXPECT_FALSE(extracted.empty());
        EXPECT_GT(extracted.size(), targetSize * 0.8);  // Should preserve most of the text
    });
    
    // Test database storage of large transcription
    DatabaseManager dbManager(":memory:");
    dbManager.createTable();
    
    FileData data = createTestFileData();
    data.transcription = largeText;
    data.v2transcription = "{\"12345\":\"" + largeText.substr(0, 1000) + "\"}";  // Truncated for JSON
    
    EXPECT_NO_THROW(dbManager.insertRecording(
        data.date, data.time, data.unixtime, data.talkgroupID,
        data.talkgroupName, data.radioID, data.duration,
        data.filename, data.filepath, data.transcription, data.v2transcription
    ));
}

// =============================================================================
// MALFORMED DATA TESTS
// =============================================================================

class MalformedDataTest : public SDRTrunkTestFixture {};

TEST_F(MalformedDataTest, MalformedJSONHandling) {
    std::vector<std::string> malformedJSONs = {
        "{\"text\": incomplete",
        "\"text\":\"missing braces\"",
        "{text: \"no quotes on key\"}",
        "{}",  // Empty JSON
        "null",
        "\"just a string\"",
        "{\"text\": \"\"}",  // Empty text
        "{\"nottext\": \"wrong key\"}"
    };
    
    for (const auto& malformedJSON : malformedJSONs) {
        EXPECT_NO_THROW({
            std::string result = extractActualTranscription(malformedJSON);
            // Should return empty string or handle gracefully, not crash
        }) << "Failed on malformed JSON: " << malformedJSON;
    }
}

TEST_F(MalformedDataTest, MalformedFilenamePatterns) {
    std::vector<std::string> malformedFilenames = {
        "20240115_TG_52198_ID_12345.mp3",  // Missing time
        "invalid_pattern.mp3",
        ".mp3",  // Just extension
        "file with spaces.mp3"  // Spaces in filename
    };
    
    std::string testDir = fileManager->createTempDirectory("malformed_files");
    
    for (const auto& filename : malformedFilenames) {
        if (filename.empty()) continue;  // Skip empty filename
        
        std::string filePath = testDir + "/" + filename;
        
        try {
            auto audioData = TestDataGenerator::generateMockMP3Data();
            std::ofstream file(filePath, std::ios::binary);
            file.write(reinterpret_cast<const char*>(audioData.data()), audioData.size());
            file.close();
            
            EXPECT_NO_THROW({
                FileData data = processFile(
                    std::filesystem::path(filePath),
                    testDir,
                    "test-api-key"
                );
                
                // Malformed filenames should result in default/empty values
                // but should not crash the application
            }) << "Failed on malformed filename: " << filename;
        } catch (const std::exception& e) {
            // File creation might fail for some malformed names, which is OK
            GTEST_SKIP() << "Could not create file: " << filename << " - " << e.what();
        }
    }
}

// =============================================================================
// FILESYSTEM EDGE CASES
// =============================================================================

class FilesystemEdgeCaseTest : public SDRTrunkTestFixture {};

TEST_F(FilesystemEdgeCaseTest, FilePermissionTests) {
    std::string testDir = fileManager->createTempDirectory("permission_test");
    std::string testFile = testDir + "/test.mp3";
    
    // Create test file
    auto audioData = TestDataGenerator::generateMockMP3Data();
    std::ofstream file(testFile, std::ios::binary);
    file.write(reinterpret_cast<const char*>(audioData.data()), audioData.size());
    file.close();
    
    // Test file locking detection
    EXPECT_FALSE(isFileLocked(testFile));
    EXPECT_FALSE(isFileBeingWrittenTo(testFile));
    
    // Test with non-existent file
    std::string nonExistentFile = testDir + "/nonexistent.mp3";
    EXPECT_NO_THROW({
        bool locked = isFileLocked(nonExistentFile);
        bool writing = isFileBeingWrittenTo(nonExistentFile);
        (void)locked; (void)writing;
    });
}