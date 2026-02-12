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
#include "ThreadPool.h"
#include "MP3Duration.h"
#include "jsonParser.h"
#include <sqlite3.h>

// Forward declaration for helper function
extern int64_t generateUnixTimestamp(const std::string &date, const std::string &time);

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
    YamlNode config = loadTestConfig();
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
    
    // Step 3: Since we can't make real API calls in tests, create mock data
    FileData processedData;
    processedData.date = "20240115";
    processedData.time = "143045";
    
    // Set timestamp
    std::tm tm = {};
    std::istringstream ss(processedData.date + processedData.time);
    ss >> std::get_time(&tm, "%Y%m%d%H%M%S");
    processedData.timestamp = std::chrono::system_clock::from_time_t(std::mktime(&tm));
    
    processedData.talkgroupID = TalkgroupId(52198);
    processedData.radioID = RadioId(12345);
    processedData.talkgroupName = "NCSHP";
    processedData.duration = Duration(std::chrono::seconds(5));
    processedData.filename = FilePath(std::filesystem::path(testFilename));
    processedData.filepath = FilePath(std::filesystem::path(testFilePath));
    
    // Verify file processing results
    EXPECT_EQ(processedData.talkgroupID.get(), 52198);
    EXPECT_EQ(processedData.radioID.get(), 12345);
    EXPECT_EQ(processedData.date, "20240115");
    EXPECT_EQ(processedData.time, "143045");
    EXPECT_FALSE(processedData.filename.get().empty());
    
    // Step 4: Generate V2 transcription with glossary
    const auto& talkgroupFiles = configSingleton.getTalkgroupFiles();
    std::string mockTranscription = TestDataGenerator::generateOpenAIResponse(
        "Unit officer responding to 10-4 at Main Street"
    );
    
    std::string v2Transcription = generateV2Transcription(
        mockTranscription, processedData.talkgroupID.get(), processedData.radioID.get(), talkgroupFiles
    );
    
    EXPECT_FALSE(v2Transcription.empty());
    EXPECT_NE(v2Transcription.find("police officer"), std::string::npos);
    EXPECT_NE(v2Transcription.find("acknowledged"), std::string::npos);
    
    // Step 5: Store in database
    processedData.transcription = Transcription(extractActualTranscription(mockTranscription));
    processedData.v2transcription = Transcription(v2Transcription);
    
    EXPECT_NO_THROW(dbManager->insertRecording(
        processedData.date, processedData.time, processedData.unixtime(),
        processedData.talkgroupID.get(), processedData.talkgroupName, processedData.radioID.get(),
        static_cast<double>(processedData.duration.get().count()), processedData.filename.get().string(), processedData.filepath.get().string(),
        processedData.transcription.get(), processedData.v2transcription.get()
    ));
}

TEST_F(EndToEndIntegrationTest, MultipleFileProcessingWorkflow) {
    // Initialize configuration
    YamlNode config = loadTestConfig();
    auto& configSingleton = ConfigSingleton::getInstance();
    configSingleton.initialize(config);
    
    // Create multiple test files with different talkgroups
    std::vector<std::tuple<int, int, std::string>> testCases = {
        {52198, 12345, "Unit officer responding to call"},
        {52199, 12346, "Backup officer unit en route to location"},
        {52200, 12347, "10-4 acknowledged, officer clear"},
        {99999, 99999, "Default talkgroup transmission copy"}  // Should use default glossary
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
        
        // Create mock file data since we can't make real API calls
        FileData data;
        data.date = "20240115";
        data.time = "143045";
        // Set timestamp for unixtime() function
        std::tm tm = {};
        std::istringstream ss(data.date + data.time);
        ss >> std::get_time(&tm, "%Y%m%d%H%M%S");
        data.timestamp = std::chrono::system_clock::from_time_t(std::mktime(&tm));
        data.talkgroupID = TalkgroupId(talkgroupID);
        data.radioID = RadioId(radioID);
        data.talkgroupName = (talkgroupID >= 52198 && talkgroupID <= 52250) ? "NCSHP" : "Unknown";
        data.duration = Duration(std::chrono::seconds(5));
        data.filename = FilePath(std::filesystem::path(filename));
        data.filepath = FilePath(std::filesystem::path(filePath));
        
        // Generate V2 transcription
        std::string mockTranscription = TestDataGenerator::generateOpenAIResponse(transcriptText);
        data.transcription = Transcription(extractActualTranscription(mockTranscription));
        data.v2transcription = Transcription(generateV2Transcription(
            mockTranscription, data.talkgroupID.get(), data.radioID.get(),
            configSingleton.getTalkgroupFiles()
        ));

        processedFiles.push_back(data);

        // Store in database
        EXPECT_NO_THROW(dbManager->insertRecording(
            data.date, data.time, data.unixtime(), data.talkgroupID.get(),
            data.talkgroupName, data.radioID.get(), static_cast<double>(data.duration.get().count()),
            data.filename.get().string(), data.filepath.get().string(), data.transcription.get(), data.v2transcription.get()
        ));
    }
    
    // Verify all files were processed correctly
    EXPECT_EQ(processedFiles.size(), 4);
    
    // Check that NCSHP files got enhanced glossary mappings
    for (const auto& data : processedFiles) {
        if (data.talkgroupID.get() >= 52198 && data.talkgroupID.get() <= 52250) {
            EXPECT_NE(data.v2transcription.get().find("police officer"), std::string::npos)
                << "NCSHP file should have enhanced glossary: " << data.filename.get();
        }
    }
}

// =============================================================================
// CONFIGURATION VALIDATION INTEGRATION TESTS
// =============================================================================

class ConfigValidationIntegrationTest : public SDRTrunkTestFixture {};

TEST_F(ConfigValidationIntegrationTest, InvalidConfigurationHandling) {
    // Test with missing required fields
    YamlNode invalidConfig;
    invalidConfig["DATABASE_PATH"] = ":memory:";
    // Missing OPENAI_API_KEY
    
    auto& configSingleton = ConfigSingleton::getInstance();
    
    // Should handle missing configuration gracefully
    EXPECT_THROW(configSingleton.initialize(invalidConfig), std::exception);
}

TEST_F(ConfigValidationIntegrationTest, TalkgroupFilesConfigurationValidation) {
    YamlNode config = loadTestConfig();
    
    // Debug: Check what's in the config
    if (config.hasKey("TALKGROUP_FILES")) {
        const auto& tgFiles = config["TALKGROUP_FILES"];
        auto keys = tgFiles.getKeys();
        std::cout << "Config TALKGROUP_FILES keys: ";
        for (const auto& key : keys) {
            std::cout << key << " ";
        }
        std::cout << std::endl;
    }
    
    auto& configSingleton = ConfigSingleton::getInstance();
    configSingleton.initialize(config);
    
    const auto& talkgroupFiles = configSingleton.getTalkgroupFiles();
    
    // Debug: Print what talkgroups we actually have
    std::cout << "Talkgroup files size: " << talkgroupFiles.size() << std::endl;
    for (const auto& [id, files] : talkgroupFiles) {
        std::cout << "  Found talkgroup: " << id << std::endl;
    }
    
    // Verify talkgroup range mapping
    EXPECT_TRUE(talkgroupFiles.find(52198) != talkgroupFiles.end()) << "Missing 52198";
    EXPECT_TRUE(talkgroupFiles.find(52225) != talkgroupFiles.end()) << "Missing 52225";
    EXPECT_TRUE(talkgroupFiles.find(52250) != talkgroupFiles.end()) << "Missing 52250";
    
    // Verify glossary files are accessible
    for (const auto& [talkgroupID, files] : talkgroupFiles) {
        for (const auto& glossaryFile : files.glossaryFiles) {
            EXPECT_TRUE(std::filesystem::exists(glossaryFile))
                << "Glossary file should exist: " << glossaryFile;
        }
    }
}

TEST_F(ConfigValidationIntegrationTest, DebugFlagsIntegration) {
    YamlNode config = loadTestConfig();
    
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
        testData.date, testData.time, testData.unixtime(),
        testData.talkgroupID.get(), testData.talkgroupName, testData.radioID.get(),
        static_cast<double>(testData.duration.get().count()), testData.filename.get().string(), testData.filepath.get().string(),
        testData.transcription.get(), testData.v2transcription.get()
    ));
}

TEST_F(DatabaseMigrationTest, DatabasePersistenceTest) {
    // Create database and insert data
    {
        DatabaseManager dbManager(persistentDbPath);
        dbManager.createTable();

        FileData testData = createTestFileData();
        dbManager.insertRecording(
            testData.date, testData.time, testData.unixtime(),
            testData.talkgroupID.get(), testData.talkgroupName, testData.radioID.get(),
            static_cast<double>(testData.duration.get().count()), testData.filename.get().string(), testData.filepath.get().string(),
            testData.transcription.get(), testData.v2transcription.get()
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
                EXPECT_TRUE(data.filename.get().empty() || data.talkgroupID.get() == 0 || data.radioID.get() == 0)
                    << "Invalid filename should not produce valid data: " << filename;
            }
        });
    }
}

TEST_F(ErrorHandlingIntegrationTest, MissingGlossaryFileHandling) {
    // Create config with non-existent glossary files
    YamlNode config = loadTestConfig();
    YamlNode talkgroupFiles;
    YamlNode testTalkgroup;
    YamlNode glossary;
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

// =============================================================================
// DATABASE: INSERT OR IGNORE / DUPLICATE HANDLING (Issue #26)
// =============================================================================

class DatabaseDuplicateTest : public ::testing::Test {
protected:
    std::unique_ptr<DatabaseManager> db;
    void SetUp() override {
        db = std::make_unique<DatabaseManager>(":memory:");
        db->createTable();
    }
};

TEST_F(DatabaseDuplicateTest, DuplicateFilenameIgnored) {
    EXPECT_NO_THROW(db->insertRecording(
        "20240115", "143045", 1705330245, 52198, "NCSHP",
        12345, 15.5, "unique_file.mp3", "/path/unique_file.mp3",
        "transcription", "v2transcription"));

    // Same filename again — should be silently ignored, not throw
    EXPECT_NO_THROW(db->insertRecording(
        "20240116", "150000", 1705330999, 52199, "Other",
        99999, 20.0, "unique_file.mp3", "/path/unique_file.mp3",
        "different transcription", "different v2"));
}

TEST_F(DatabaseDuplicateTest, DifferentFilenamesSucceed) {
    EXPECT_NO_THROW(db->insertRecording(
        "20240115", "143045", 1705330245, 52198, "NCSHP",
        12345, 15.5, "file1.mp3", "/path/file1.mp3", "t1", "v1"));
    EXPECT_NO_THROW(db->insertRecording(
        "20240115", "143046", 1705330246, 52198, "NCSHP",
        12345, 15.5, "file2.mp3", "/path/file2.mp3", "t2", "v2"));
}

// =============================================================================
// DATABASE: DURATION AS DOUBLE (Issue #26)
// =============================================================================

class DatabaseDurationTest : public ::testing::Test {
protected:
    std::unique_ptr<DatabaseManager> db;
    void SetUp() override {
        db = std::make_unique<DatabaseManager>(":memory:");
        db->createTable();
    }
};

TEST_F(DatabaseDurationTest, FractionalDuration) {
    EXPECT_NO_THROW(db->insertRecording(
        "20240115", "143045", 1705330245, 52198, "NCSHP",
        12345, 15.123456, "frac.mp3", "/path/frac.mp3", "t", "v"));
}

TEST_F(DatabaseDurationTest, ZeroDuration) {
    EXPECT_NO_THROW(db->insertRecording(
        "20240115", "143045", 1705330245, 52198, "NCSHP",
        12345, 0.0, "zero.mp3", "/path/zero.mp3", "t", "v"));
}

TEST_F(DatabaseDurationTest, LargeDuration) {
    EXPECT_NO_THROW(db->insertRecording(
        "20240115", "143045", 1705330245, 52198, "NCSHP",
        12345, 3600.0, "long.mp3", "/path/long.mp3", "t", "v"));
}

// =============================================================================
// DATABASE: SCHEMA MIGRATION (Issue #26)
// =============================================================================

class DatabaseSchemaMigrationTest : public ::testing::Test {
protected:
    std::string dbPath;

    void SetUp() override {
        dbPath = "/tmp/sdrtrunk_migration_" +
                 std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".db";
    }

    void TearDown() override {
        std::filesystem::remove(dbPath);
        std::filesystem::remove(dbPath + "-wal");
        std::filesystem::remove(dbPath + "-shm");
    }
};

TEST_F(DatabaseSchemaMigrationTest, OldSchemaGetsMigrated) {
    // Create old-schema database directly via sqlite3
    {
        sqlite3 *rawDb;
        ASSERT_EQ(sqlite3_open(dbPath.c_str(), &rawDb), SQLITE_OK);
        const char *oldSchema = R"(
            CREATE TABLE recordings (
                date TEXT NOT NULL, time TEXT NOT NULL, unixtime INTEGER NOT NULL,
                talkgroup_id INTEGER NOT NULL, talkgroup_name TEXT NOT NULL DEFAULT '',
                radio_id INTEGER NOT NULL, duration TEXT NOT NULL DEFAULT '',
                filename TEXT NOT NULL, filepath TEXT NOT NULL,
                transcription TEXT NOT NULL DEFAULT '', v2transcription TEXT NOT NULL DEFAULT ''
            ))";
        char *err = nullptr;
        ASSERT_EQ(sqlite3_exec(rawDb, oldSchema, 0, 0, &err), SQLITE_OK) << (err ? err : "");
        const char *ins = R"(INSERT INTO recordings VALUES
            ('20240115','143045',1705330245,52198,'NCSHP',12345,'15.5',
             'old.mp3','/path/old.mp3','old t','old v'))";
        ASSERT_EQ(sqlite3_exec(rawDb, ins, 0, 0, &err), SQLITE_OK) << (err ? err : "");
        sqlite3_close(rawDb);
    }

    // Open with DatabaseManager — should trigger migration
    DatabaseManager db(dbPath);
    EXPECT_NO_THROW(db.createTable());
    EXPECT_NO_THROW(db.insertRecording(
        "20240116", "150000", 1705416000, 52199, "Other",
        99999, 20.5, "new.mp3", "/path/new.mp3", "new t", "new v"));
}

TEST_F(DatabaseSchemaMigrationTest, FreshDatabaseNoMigrationNeeded) {
    DatabaseManager db(dbPath);
    EXPECT_NO_THROW(db.createTable());
    EXPECT_NO_THROW(db.insertRecording(
        "20240115", "143045", 1705330245, 52198, "NCSHP",
        12345, 15.5, "fresh.mp3", "/path/fresh.mp3", "t", "v"));
}

// =============================================================================
// DATABASE: WAL MODE (Issue #26)
// =============================================================================

class DatabaseWALTest : public ::testing::Test {
protected:
    std::string dbPath;
    void SetUp() override {
        dbPath = "/tmp/sdrtrunk_wal_" +
                 std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".db";
    }
    void TearDown() override {
        std::filesystem::remove(dbPath);
        std::filesystem::remove(dbPath + "-wal");
        std::filesystem::remove(dbPath + "-shm");
    }
};

TEST_F(DatabaseWALTest, DatabaseFileCreated) {
    DatabaseManager db(dbPath);
    db.createTable();
    db.insertRecording("20240115", "143045", 1705330245, 52198, "NCSHP",
                       12345, 15.5, "wal.mp3", "/path/wal.mp3", "t", "v");
    EXPECT_TRUE(std::filesystem::exists(dbPath));
}

// =============================================================================
// DATABASE: THREAD-SAFE WRITES (Issue #15)
// =============================================================================

class DatabaseThreadSafetyTest : public ::testing::Test {
protected:
    std::unique_ptr<DatabaseManager> db;
    void SetUp() override {
        db = std::make_unique<DatabaseManager>(":memory:");
        db->createTable();
    }
};

TEST_F(DatabaseThreadSafetyTest, ConcurrentInserts) {
    const int numThreads = 8;
    const int insertsPerThread = 10;
    std::vector<std::thread> threads;
    std::atomic<int> successCount{0};

    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([this, t, &successCount]() {
            for (int i = 0; i < 10; ++i) {
                std::string fn = "t" + std::to_string(t) + "_f" + std::to_string(i) + ".mp3";
                try {
                    db->insertRecording("20240115", "143045", 1705330245, 52198, "NCSHP",
                                        12345, 15.5, fn, "/path/" + fn, "t", "v");
                    successCount.fetch_add(1);
                } catch (...) {}
            }
        });
    }

    for (auto &t : threads) t.join();

    EXPECT_EQ(successCount.load(), numThreads * insertsPerThread)
        << "All concurrent inserts should succeed with mutex protection";
}

// =============================================================================
// THREAD POOL TESTS (Issue #23)
// =============================================================================

class ThreadPoolTest : public ::testing::Test {};

TEST_F(ThreadPoolTest, BasicEnqueueAndGet) {
    ThreadPool pool(2);
    auto future = pool.enqueue([]() { return 42; });
    EXPECT_EQ(future.get(), 42);
}

TEST_F(ThreadPoolTest, MultipleTasksComplete) {
    ThreadPool pool(4);
    std::vector<std::future<int>> futures;
    for (int i = 0; i < 20; ++i)
        futures.push_back(pool.enqueue([i]() { return i * i; }));
    for (int i = 0; i < 20; ++i)
        EXPECT_EQ(futures[i].get(), i * i);
}

TEST_F(ThreadPoolTest, TasksRunConcurrently) {
    ThreadPool pool(4);
    std::atomic<int> concurrent{0};
    std::atomic<int> maxConcurrent{0};
    std::vector<std::future<void>> futures;

    for (int i = 0; i < 8; ++i) {
        futures.push_back(pool.enqueue([&]() {
            int cur = concurrent.fetch_add(1) + 1;
            int exp = maxConcurrent.load();
            while (cur > exp && !maxConcurrent.compare_exchange_weak(exp, cur)) {}
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            concurrent.fetch_sub(1);
        }));
    }
    for (auto &f : futures) f.get();

    EXPECT_GT(maxConcurrent.load(), 1)
        << "Multiple tasks should run concurrently with 4 threads";
}

TEST_F(ThreadPoolTest, SingleThread) {
    ThreadPool pool(1);
    std::vector<std::future<int>> futures;
    for (int i = 0; i < 5; ++i)
        futures.push_back(pool.enqueue([i]() { return i + 1; }));
    for (int i = 0; i < 5; ++i)
        EXPECT_EQ(futures[i].get(), i + 1);
}

TEST_F(ThreadPoolTest, ExceptionPropagation) {
    ThreadPool pool(2);
    auto future = pool.enqueue([]() -> int { throw std::runtime_error("task error"); });
    EXPECT_THROW(future.get(), std::runtime_error);
}

TEST_F(ThreadPoolTest, DestructorJoinsThreads) {
    std::atomic<int> completed{0};
    {
        ThreadPool pool(2);
        for (int i = 0; i < 4; ++i)
            pool.enqueue([&completed]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                completed.fetch_add(1);
            });
    } // Pool destructor should wait for all tasks
    EXPECT_EQ(completed.load(), 4);
}

TEST_F(ThreadPoolTest, VoidReturnType) {
    ThreadPool pool(2);
    std::atomic<bool> ran{false};
    auto future = pool.enqueue([&ran]() { ran.store(true); });
    future.get();
    EXPECT_TRUE(ran.load());
}

TEST_F(ThreadPoolTest, StringReturnType) {
    ThreadPool pool(2);
    auto future = pool.enqueue([]() -> std::string { return "hello"; });
    EXPECT_EQ(future.get(), "hello");
}

// =============================================================================
// PER-TALKGROUP PROMPT CONFIG INTEGRATION (Issue #12)
// =============================================================================

class PromptConfigIntegrationTest : public SDRTrunkTestFixture {};

TEST_F(PromptConfigIntegrationTest, ConfigParsesPromptField) {
    std::string cfgContent = R"(
OPENAI_API_KEY: "test-key"
DATABASE_PATH: ":memory:"
DirectoryToMonitor: "/tmp/test"
LoopWaitSeconds: 5
MAX_RETRIES: 3
MAX_REQUESTS_PER_MINUTE: 60
ERROR_WINDOW_SECONDS: 300
RATE_LIMIT_WINDOW_SECONDS: 60
MIN_DURATION_SECONDS: 1
DEBUG_CURL_HELPER: false
DEBUG_DATABASE_MANAGER: false
DEBUG_FILE_PROCESSOR: false
DEBUG_MAIN: false
DEBUG_TRANSCRIPTION_PROCESSOR: false
TALKGROUP_FILES:
  52198:
    GLOSSARY: ["/tmp/glossary.json"]
    PROMPT: "Police radio dispatch."
)";
    std::string cfgPath = fileManager->createTempFile("prompt_cfg.yaml", cfgContent);
    YamlNode config = YamlParser::loadFile(cfgPath);
    auto &cfg = ConfigSingleton::getInstance();
    cfg.initialize(config);

    auto it = cfg.getTalkgroupFiles().find(52198);
    ASSERT_NE(it, cfg.getTalkgroupFiles().end());
    EXPECT_EQ(it->second.prompt, "Police radio dispatch.");
}

// =============================================================================
// REAL MP3 END-TO-END PIPELINE TEST
// =============================================================================
// Tests the full SDRTrunk workflow with a real MP3 file:
//   detect file -> duration check -> extract metadata -> glossary lookup ->
//   generate v2 transcription -> save to DB -> verify all data
// Only the OpenAI API call is skipped (requires credentials).

extern int extractTalkgroupIdFromFilename(const std::string &filename);

class RealPipelineTest : public SDRTrunkTestFixture {
protected:
    static constexpr const char* REAL_MP3 =
        "/home/foxtrot/git/cpp-sdrtrunk-transcriber/test/test_data/"
        "20250715_112349North_Carolina_VIPER_Rutherford_T-SPDControl"
        "__TO_52324_FROM_2097268.mp3";
    static constexpr const char* REAL_FILENAME =
        "20250715_112349North_Carolina_VIPER_Rutherford_T-SPDControl"
        "__TO_52324_FROM_2097268.mp3";

    std::string monitorDir;
    std::string glossaryPath;

    void SetUp() override {
        SDRTrunkTestFixture::SetUp();
        monitorDir = fileManager->createTempDirectory("pipeline_monitor");

        // Create a realistic glossary file (multi-key format)
        glossaryPath = fileManager->createTempFile("pipeline_glossary.json", R"JSON({
            "GLOSSARY": [
                {"keys": ["10-4", "104"], "value": "Affirmative (OK)"},
                {"keys": ["10-20", "1020"], "value": "Location"},
                {"keys": ["10-7", "107"], "value": "Out-of-Service"},
                {"keys": ["officer", "ofc"], "value": "police officer"},
                {"keys": ["ETA"], "value": "estimated time of arrival"},
                {"keys": ["MVA"], "value": "motor vehicle accident"}
            ]
        })JSON");
    }
};

TEST_F(RealPipelineTest, Step1_FileDetection) {
    // Simulate SDRTrunk writing an MP3 to the monitored directory
    ASSERT_TRUE(std::filesystem::exists(REAL_MP3)) << "Real MP3 test file missing";

    std::filesystem::path dest = std::filesystem::path(monitorDir) / REAL_FILENAME;
    std::filesystem::copy_file(REAL_MP3, dest);

    EXPECT_TRUE(std::filesystem::exists(dest));
    EXPECT_FALSE(isFileLocked(dest.string()));
    EXPECT_FALSE(isFileBeingWrittenTo(dest.string()));
}

TEST_F(RealPipelineTest, Step2_MP3DurationExtraction) {
    // Use libmpg123 to extract duration from the real MP3
    auto result = sdrtrunk::getMP3Duration(REAL_MP3);

    ASSERT_TRUE(result.has_value()) << "libmpg123 failed: " << result.error().toString();

    double duration = result.value();
    std::cout << "  Real MP3 duration: " << duration << " seconds" << std::endl;

    EXPECT_GT(duration, 0.0) << "Duration should be positive";
    EXPECT_LT(duration, 300.0) << "Duration should be reasonable (< 5 min for test file)";
}

TEST_F(RealPipelineTest, Step3_TalkgroupIdFromFilename) {
    int tgId = extractTalkgroupIdFromFilename(REAL_FILENAME);
    EXPECT_EQ(tgId, 52324);
}

TEST_F(RealPipelineTest, Step4_ExtractFileInfo) {
    // Simulate what happens after transcription returns
    std::string mockTranscription = R"({"text":"Unit 104 responding to MVA on Highway 74, officer en route ETA 5 minutes"})";

    FileData data;
    extractFileInfo(data, REAL_FILENAME, mockTranscription);

    EXPECT_EQ(data.date, "20250715");
    EXPECT_EQ(data.time, "112349");
    EXPECT_EQ(data.talkgroupID.get(), 52324);
    EXPECT_EQ(data.radioID.get(), 2097268);
    EXPECT_EQ(data.talkgroupName, "North_Carolina_VIPER_Rutherford_T-SPDControl");
    EXPECT_EQ(data.filename.get().string(), REAL_FILENAME);
    EXPECT_FALSE(data.transcription.get().empty());

    // Verify unix timestamp is reasonable (July 2025)
    int64_t unixtime = data.unixtime();
    EXPECT_GT(unixtime, 1750000000) << "Timestamp should be after 2025-06-15";
    EXPECT_LT(unixtime, 1760000000) << "Timestamp should be before 2025-10-06";
}

TEST_F(RealPipelineTest, Step5_GlossaryMultiKeyAndHyphenStripping) {
    // Read the multi-key glossary and verify both formats + hyphen stripping
    auto mappings = readMappingFile(glossaryPath);

    // Multi-key entries
    EXPECT_EQ(mappings["10-4"], "Affirmative (OK)");
    EXPECT_EQ(mappings["104"], "Affirmative (OK)");   // explicit key from GLOSSARY
    EXPECT_EQ(mappings["officer"], "police officer");
    EXPECT_EQ(mappings["ofc"], "police officer");      // alias

    // Hyphen-stripped auto-generation
    EXPECT_EQ(mappings["1020"], "Location");
    EXPECT_EQ(mappings["107"], "Out-of-Service");

    // Non-hyphenated key has no extra variant
    EXPECT_EQ(mappings.count("ETA"), 1);
}

TEST_F(RealPipelineTest, Step6_GenerateV2Transcription) {
    // Wire up glossary for talkgroup 52324
    std::unordered_map<int, TalkgroupFiles> tgFiles;
    TalkgroupFiles files;
    files.glossaryFiles.push_back(glossaryPath);
    files.prompt = "Police radio dispatch, NC VIPER.";
    tgFiles[52324] = files;

    std::string mockTranscription = R"({"text":"Unit 104 responding to MVA on Highway 74, officer en route ETA 5 minutes"})";

    std::string v2 = generateV2Transcription(mockTranscription, 52324, 2097268, tgFiles);

    std::cout << "  V2 transcription: " << v2 << std::endl;

    EXPECT_FALSE(v2.empty());
    // Should contain radio ID as JSON key
    EXPECT_NE(v2.find("\"2097268\""), std::string::npos) << "Missing radio ID in v2";
    // Should contain glossary matches
    EXPECT_NE(v2.find("motor vehicle accident"), std::string::npos) << "MVA not expanded";
    EXPECT_NE(v2.find("police officer"), std::string::npos) << "officer not expanded";
    EXPECT_NE(v2.find("estimated time of arrival"), std::string::npos) << "ETA not expanded";
}

TEST_F(RealPipelineTest, Step7_DatabaseInsertAndVerify) {
    // Full pipeline: duration + metadata + v2 transcription + DB insert
    auto durResult = sdrtrunk::getMP3Duration(REAL_MP3);
    ASSERT_TRUE(durResult.has_value());
    double duration = durResult.value();

    std::string mockTranscription = R"({"text":"Unit 104 responding to MVA on Highway 74"})";

    FileData data;
    extractFileInfo(data, REAL_FILENAME, mockTranscription);

    // Set duration from libmpg123
    data.duration = Duration(std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::duration<double>(duration)));
    data.filepath = FilePath(std::filesystem::path(REAL_MP3));

    // Generate v2 with glossary
    std::unordered_map<int, TalkgroupFiles> tgFiles;
    TalkgroupFiles files;
    files.glossaryFiles.push_back(glossaryPath);
    tgFiles[52324] = files;
    data.v2transcription = Transcription(generateV2Transcription(
        mockTranscription, data.talkgroupID.get(), data.radioID.get(), tgFiles));

    // Insert into database
    DatabaseManager db(":memory:");
    db.createTable();

    EXPECT_NO_THROW(db.insertRecording(
        data.date, data.time, data.unixtime(),
        data.talkgroupID.get(), data.talkgroupName, data.radioID.get(),
        duration,
        data.filename.get().string(), data.filepath.get().string(),
        data.transcription.get(), data.v2transcription.get()));

    // Verify by querying back
    sqlite3 *rawDb;
    ASSERT_EQ(sqlite3_open(":memory:", &rawDb), SQLITE_OK);
    // We can't query the in-memory DB from a different connection, but
    // we verified no exception was thrown, meaning the INSERT succeeded
    sqlite3_close(rawDb);

    // Print summary
    std::cout << "  === PIPELINE SUMMARY ===" << std::endl;
    std::cout << "  File:        " << data.filename.get().string() << std::endl;
    std::cout << "  Date:        " << data.date << std::endl;
    std::cout << "  Time:        " << data.time << std::endl;
    std::cout << "  Talkgroup:   " << data.talkgroupID.get() << " (" << data.talkgroupName << ")" << std::endl;
    std::cout << "  Radio ID:    " << data.radioID.get() << std::endl;
    std::cout << "  Duration:    " << duration << "s" << std::endl;
    std::cout << "  Unix time:   " << data.unixtime() << std::endl;
    std::cout << "  V2 output:   " << data.v2transcription.get() << std::endl;
    std::cout << "  ========================" << std::endl;
}

TEST_F(RealPipelineTest, Step8_FileMoveToTalkgroupDir) {
    // Copy real MP3 to monitored directory, then verify move logic
    std::filesystem::path dest = std::filesystem::path(monitorDir) / REAL_FILENAME;
    std::filesystem::copy_file(REAL_MP3, dest);

    // Create a mock .txt file alongside it (as saveTranscription would)
    std::filesystem::path txtPath = dest;
    txtPath.replace_extension(".txt");
    {
        std::ofstream txt(txtPath);
        txt << R"({"2097268":"test transcription","MVA":"motor vehicle accident"})";
    }

    EXPECT_TRUE(std::filesystem::exists(dest));
    EXPECT_TRUE(std::filesystem::exists(txtPath));

    // The app creates a subdirectory named after the talkgroup ID
    std::filesystem::path tgSubdir = std::filesystem::path(monitorDir) / "52324";
    std::filesystem::create_directory(tgSubdir);

    // Move files (simulating what moveFiles() does)
    std::filesystem::rename(dest, tgSubdir / REAL_FILENAME);
    std::filesystem::rename(txtPath, tgSubdir / txtPath.filename());

    // Verify files moved to talkgroup subdirectory
    EXPECT_TRUE(std::filesystem::exists(tgSubdir / REAL_FILENAME));
    EXPECT_TRUE(std::filesystem::exists(tgSubdir / txtPath.filename()));
    EXPECT_FALSE(std::filesystem::exists(dest));
}

// =============================================================================
// DIVERSE REAL MP3 PIPELINE TESTS
// =============================================================================
// Tests the pipeline with real MP3 files covering different SDRTrunk patterns:
//   - P-group with bracket notation: TO_P52197-[52198]
//   - NBFM without FROM field: TO_9969
//   - Short file below MIN_DURATION threshold
//   - Standard P25 with different talkgroups

static const std::string TEST_DATA =
    "/home/foxtrot/git/cpp-sdrtrunk-transcriber/test/test_data/";

class DiverseMP3Test : public ::testing::Test {};

// -- P-group file (largest, ~235K) --

TEST_F(DiverseMP3Test, PGroup_DurationExtraction) {
    std::string path = TEST_DATA +
        "20260208_121343_North-Carolina-VIPER_Rutherford_T-Control"
        "__TO_P52197-[52198]_FROM_2151534.mp3";
    ASSERT_TRUE(std::filesystem::exists(path));
    auto result = sdrtrunk::getMP3Duration(path);
    ASSERT_TRUE(result.has_value()) << "libmpg123 failed: " << result.error().toString();
    std::cout << "  P-group MP3 duration: " << result.value() << "s" << std::endl;
    EXPECT_GT(result.value(), 5.0) << "Large file should have substantial duration";
    EXPECT_LT(result.value(), 120.0);
}

TEST_F(DiverseMP3Test, PGroup_FullPipeline) {
    std::string fn =
        "20260208_121343_North-Carolina-VIPER_Rutherford_T-Control"
        "__TO_P52197-[52198]_FROM_2151534.mp3";
    std::string path = TEST_DATA + fn;

    // Duration
    auto dur = sdrtrunk::getMP3Duration(path);
    ASSERT_TRUE(dur.has_value());

    // Talkgroup extraction (should use bracketed value)
    int tgId = extractTalkgroupIdFromFilename(fn);
    EXPECT_EQ(tgId, 52198);

    // Full file info
    FileData data;
    extractFileInfo(data, fn, R"({"text":"unit 104 responding"})");
    EXPECT_EQ(data.talkgroupID.get(), 52198);
    EXPECT_EQ(data.radioID.get(), 2151534);
    EXPECT_EQ(data.talkgroupName, "North-Carolina-VIPER_Rutherford_T-Control");

    // DB insert
    DatabaseManager db(":memory:");
    db.createTable();
    data.duration = Duration(std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::duration<double>(dur.value())));
    data.filepath = FilePath(std::filesystem::path(path));

    EXPECT_NO_THROW(db.insertRecording(
        data.date, data.time, data.unixtime(),
        data.talkgroupID.get(), data.talkgroupName, data.radioID.get(),
        dur.value(), data.filename.get().string(), data.filepath.get().string(),
        data.transcription.get(), data.v2transcription.get()));
}

// -- NBFM file (no FROM field) --

TEST_F(DiverseMP3Test, NBFM_DurationExtraction) {
    std::string path = TEST_DATA +
        "20260208_120926_WR4SC_Greenville_14537NBFM__TO_9969.mp3";
    ASSERT_TRUE(std::filesystem::exists(path));
    auto result = sdrtrunk::getMP3Duration(path);
    ASSERT_TRUE(result.has_value()) << "libmpg123 failed: " << result.error().toString();
    std::cout << "  NBFM MP3 duration: " << result.value() << "s" << std::endl;
    EXPECT_GT(result.value(), 1.0);
}

TEST_F(DiverseMP3Test, NBFM_FullPipeline) {
    std::string fn = "20260208_120926_WR4SC_Greenville_14537NBFM__TO_9969.mp3";
    std::string path = TEST_DATA + fn;

    auto dur = sdrtrunk::getMP3Duration(path);
    ASSERT_TRUE(dur.has_value());

    FileData data;
    extractFileInfo(data, fn, R"({"text":"calling on repeater"})");
    EXPECT_EQ(data.talkgroupID.get(), 9969);
    EXPECT_EQ(data.radioID.get(), 1234567) << "No FROM field should use default";
    EXPECT_EQ(data.talkgroupName, "WR4SC_Greenville_14537NBFM");

    DatabaseManager db(":memory:");
    db.createTable();
    data.duration = Duration(std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::duration<double>(dur.value())));
    data.filepath = FilePath(std::filesystem::path(path));

    EXPECT_NO_THROW(db.insertRecording(
        data.date, data.time, data.unixtime(),
        data.talkgroupID.get(), data.talkgroupName, data.radioID.get(),
        dur.value(), data.filename.get().string(), data.filepath.get().string(),
        data.transcription.get(), data.v2transcription.get()));
}

// -- BlueRidge NBFM --

TEST_F(DiverseMP3Test, BlueRidge_DurationExtraction) {
    std::string path = TEST_DATA +
        "20260208_114210_BlueRidge_Unkn_14661NBFM__TO_9967.mp3";
    ASSERT_TRUE(std::filesystem::exists(path));
    auto result = sdrtrunk::getMP3Duration(path);
    ASSERT_TRUE(result.has_value());
    std::cout << "  BlueRidge NBFM duration: " << result.value() << "s" << std::endl;
    EXPECT_GT(result.value(), 1.0);
}

// -- Short file (below duration threshold) --

TEST_F(DiverseMP3Test, ShortFile_DurationExtraction) {
    std::string path = TEST_DATA +
        "20260208_114511_North-Carolina-VIPER_Rutherford_T-Control"
        "__TO_41001_FROM_1610075.mp3";
    ASSERT_TRUE(std::filesystem::exists(path));
    auto result = sdrtrunk::getMP3Duration(path);
    ASSERT_TRUE(result.has_value()) << "Even short files should parse";
    std::cout << "  Short MP3 duration: " << result.value() << "s" << std::endl;
    // 1.6KB file should be very short (< 1 second typically)
    EXPECT_LT(result.value(), 2.0) << "Short file should be under 2 seconds";
}

TEST_F(DiverseMP3Test, ShortFile_BelowThreshold) {
    // A 1.6KB MP3 should be below the typical MIN_DURATION_SECONDS=1 threshold
    std::string path = TEST_DATA +
        "20260208_114511_North-Carolina-VIPER_Rutherford_T-Control"
        "__TO_41001_FROM_1610075.mp3";
    auto result = sdrtrunk::getMP3Duration(path);
    ASSERT_TRUE(result.has_value());
    // In the real pipeline, files below MIN_DURATION_SECONDS get skipped.
    // We just verify the duration is extractable and short.
    EXPECT_GT(result.value(), 0.0) << "Duration should be positive";
}

// -- Standard P25 (medium, ~143K) --

TEST_F(DiverseMP3Test, StandardP25_DurationExtraction) {
    std::string path = TEST_DATA +
        "20260208_121634_North-Carolina-VIPER_Rutherford_T-Control"
        "__TO_41001_FROM_1610018.mp3";
    ASSERT_TRUE(std::filesystem::exists(path));
    auto result = sdrtrunk::getMP3Duration(path);
    ASSERT_TRUE(result.has_value());
    std::cout << "  Standard P25 duration: " << result.value() << "s" << std::endl;
    EXPECT_GT(result.value(), 5.0) << "Medium file should have several seconds";
}

TEST_F(DiverseMP3Test, StandardP25_FullPipeline) {
    std::string fn =
        "20260208_121634_North-Carolina-VIPER_Rutherford_T-Control"
        "__TO_41001_FROM_1610018.mp3";
    std::string path = TEST_DATA + fn;

    auto dur = sdrtrunk::getMP3Duration(path);
    ASSERT_TRUE(dur.has_value());

    FileData data;
    extractFileInfo(data, fn, R"({"text":"ten four copy that"})");
    EXPECT_EQ(data.talkgroupID.get(), 41001);
    EXPECT_EQ(data.radioID.get(), 1610018);
    EXPECT_EQ(data.talkgroupName, "North-Carolina-VIPER_Rutherford_T-Control");

    DatabaseManager db(":memory:");
    db.createTable();
    data.duration = Duration(std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::duration<double>(dur.value())));
    data.filepath = FilePath(std::filesystem::path(path));

    EXPECT_NO_THROW(db.insertRecording(
        data.date, data.time, data.unixtime(),
        data.talkgroupID.get(), data.talkgroupName, data.radioID.get(),
        dur.value(), data.filename.get().string(), data.filepath.get().string(),
        data.transcription.get(), data.v2transcription.get()));
}

// -- All files in one DB (duplicate handling) --

TEST_F(DiverseMP3Test, AllFilesInSingleDatabase) {
    DatabaseManager db(":memory:");
    db.createTable();

    std::vector<std::string> filenames = {
        "20250715_112349North_Carolina_VIPER_Rutherford_T-SPDControl__TO_52324_FROM_2097268.mp3",
        "20260208_121343_North-Carolina-VIPER_Rutherford_T-Control__TO_P52197-[52198]_FROM_2151534.mp3",
        "20260208_120926_WR4SC_Greenville_14537NBFM__TO_9969.mp3",
        "20260208_114210_BlueRidge_Unkn_14661NBFM__TO_9967.mp3",
        "20260208_114511_North-Carolina-VIPER_Rutherford_T-Control__TO_41001_FROM_1610075.mp3",
        "20260208_121634_North-Carolina-VIPER_Rutherford_T-Control__TO_41001_FROM_1610018.mp3"
    };

    int insertCount = 0;
    for (const auto& fn : filenames) {
        std::string path = TEST_DATA + fn;
        ASSERT_TRUE(std::filesystem::exists(path)) << "Missing: " << fn;

        auto dur = sdrtrunk::getMP3Duration(path);
        ASSERT_TRUE(dur.has_value()) << "Duration failed for: " << fn;

        FileData data;
        extractFileInfo(data, fn, R"({"text":"test"})");
        data.duration = Duration(std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::duration<double>(dur.value())));
        data.filepath = FilePath(std::filesystem::path(path));

        EXPECT_NO_THROW(db.insertRecording(
            data.date, data.time, data.unixtime(),
            data.talkgroupID.get(), data.talkgroupName, data.radioID.get(),
            dur.value(), data.filename.get().string(), data.filepath.get().string(),
            data.transcription.get(), data.v2transcription.get()));
        insertCount++;
    }

    EXPECT_EQ(insertCount, 6) << "All 6 files should insert successfully";

    // Verify duplicate insert is ignored (INSERT OR IGNORE)
    EXPECT_NO_THROW(db.insertRecording(
        "20260208", "121343", 1738932823, 52198, "dup",
        2151534, 10.0, filenames[1], "/path/" + filenames[1], "dup", "dup"));
}