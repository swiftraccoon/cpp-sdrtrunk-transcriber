// Third-Party Library Headers
#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include <vector>
#include <memory>

// Project-Specific Headers
#include "TestFixtures.h"
#include "ConfigSingleton.h"
#include "DatabaseManager.h"
#include "transcriptionProcessor.h"

// =============================================================================
// SIMPLE PERFORMANCE TESTS
// =============================================================================

class SimplePerformanceTest : public SDRTrunkTestFixture {
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

TEST_F(SimplePerformanceTest, ConfigInitializationPerformance) {
    YamlNode config = loadTestConfig();
    auto& configSingleton = ConfigSingleton::getInstance();
    
    startTimer();
    configSingleton.initialize(config);
    expectPerformanceUnder(100.0, "ConfigSingleton initialization");
}

TEST_F(SimplePerformanceTest, DatabaseInsertPerformance) {
    DatabaseManager dbManager(":memory:");
    dbManager.createTable();
    
    FileData data = createTestFileData();
    
    startTimer();
    dbManager.insertRecording(
        data.date, data.time, data.unixtime(), data.talkgroupID.get(),
        data.talkgroupName, data.radioID.get(), static_cast<double>(data.duration.get().count()),
        data.filename.get().string(), data.filepath.get().string(), data.transcription.get(), data.v2transcription.get()
    );
    expectPerformanceUnder(50.0, "Single database insert");
}

TEST_F(SimplePerformanceTest, GlossaryReadingPerformance) {
    std::string glossaryPath = fileManager->getCreatedFilePath("ncshp_glossary.json");
    
    startTimer();
    auto mappings = readMappingFile(glossaryPath);
    expectPerformanceUnder(10.0, "Glossary file reading");
    
    EXPECT_FALSE(mappings.empty());
}

TEST_F(SimplePerformanceTest, BulkDatabaseInsertPerformance) {
    DatabaseManager dbManager(":memory:");
    dbManager.createTable();
    
    const int recordCount = 100;
    std::vector<FileData> testData;
    
    // Prepare test data
    for (int i = 0; i < recordCount; ++i) {
        testData.push_back(createTestFileData(52198 + (i % 10), 12345 + i));
    }
    
    startTimer();
    
    for (const auto& data : testData) {
        dbManager.insertRecording(
            data.date, data.time, data.unixtime(), data.talkgroupID.get(),
            data.talkgroupName, data.radioID.get(), static_cast<double>(data.duration.get().count()),
            data.filename.get().string(), data.filepath.get().string(), data.transcription.get(), data.v2transcription.get()
        );
    }
    
    expectPerformanceUnder(1000.0, "Bulk database insert (100 records)");
}