#pragma once

// Third-Party Library Headers
#ifdef GMOCK_AVAILABLE
#include <gmock/gmock.h>
#endif
#include <curl/curl.h>
#include <sqlite3.h>

// Project-Specific Headers
#include "DatabaseManager.h"
#include "curlHelper.h"

// =============================================================================
// MOCK CURL INTERFACE
// =============================================================================

class MockCurlInterface {
public:
    virtual ~MockCurlInterface() = default;
    virtual CURL* curl_easy_init() = 0;
    virtual void curl_easy_cleanup(CURL* curl) = 0;
    virtual CURLcode curl_easy_perform(CURL* curl) = 0;
    virtual CURLcode curl_easy_setopt(CURL* curl, CURLoption option, ...) = 0;
    virtual struct curl_slist* curl_slist_append(struct curl_slist* list, const char* string) = 0;
    virtual void curl_slist_free_all(struct curl_slist* list) = 0;
    virtual curl_mime* curl_mime_init(CURL* easy) = 0;
    virtual void curl_mime_free(curl_mime* mime) = 0;
    virtual CURLcode curl_mime_addpart(curl_mime* mime, curl_mimepart** part) = 0;
    virtual const char* curl_easy_strerror(CURLcode code) = 0;
};

#ifdef GMOCK_AVAILABLE
class MockCurl : public MockCurlInterface {
public:
    MOCK_METHOD(CURL*, curl_easy_init, (), (override));
    MOCK_METHOD(void, curl_easy_cleanup, (CURL* curl), (override));
    MOCK_METHOD(CURLcode, curl_easy_perform, (CURL* curl), (override));
    MOCK_METHOD(CURLcode, curl_easy_setopt, (CURL* curl, CURLoption option, ...), (override));
    MOCK_METHOD(struct curl_slist*, curl_slist_append, (struct curl_slist* list, const char* string), (override));
    MOCK_METHOD(void, curl_slist_free_all, (struct curl_slist* list), (override));
    MOCK_METHOD(curl_mime*, curl_mime_init, (CURL* easy), (override));
    MOCK_METHOD(void, curl_mime_free, (curl_mime* mime), (override));
    MOCK_METHOD(CURLcode, curl_mime_addpart, (curl_mime* mime, curl_mimepart** part), (override));
    MOCK_METHOD(const char*, curl_easy_strerror, (CURLcode code), (override));
};
#endif

// =============================================================================
// MOCK HTTP RESPONSES
// =============================================================================

class MockHttpResponse {
public:
    static std::string successfulTranscription() {
        return R"({
            "text": "This is a test transcription from OpenAI Whisper API."
        })";
    }
    
    static std::string errorResponse() {
        return R"({
            "error": {
                "message": "Invalid API key",
                "type": "invalid_request_error"
            }
        })";
    }
    
    static std::string rateLimitResponse() {
        return R"({
            "error": {
                "message": "Rate limit exceeded",
                "type": "requests",
                "code": "rate_limit_exceeded"
            }
        })";
    }
    
    static std::string serverErrorResponse() {
        return R"({
            "error": {
                "message": "Internal server error",
                "type": "server_error"
            }
        })";
    }
};

// =============================================================================
// MOCK DATABASE INTERFACE
// =============================================================================

class MockDatabaseInterface {
public:
    virtual ~MockDatabaseInterface() = default;
    virtual int sqlite3_open(const char* filename, sqlite3** ppDb) = 0;
    virtual int sqlite3_close(sqlite3* db) = 0;
    virtual int sqlite3_exec(sqlite3* db, const char* sql, 
                            int (*callback)(void*,int,char**,char**), 
                            void* arg, char** errmsg) = 0;
    virtual int sqlite3_prepare_v2(sqlite3* db, const char* zSql, int nByte, 
                                  sqlite3_stmt** ppStmt, const char** pzTail) = 0;
    virtual int sqlite3_bind_text(sqlite3_stmt* stmt, int index, const char* text, 
                                 int length, void(*destructor)(void*)) = 0;
    virtual int sqlite3_bind_int(sqlite3_stmt* stmt, int index, int value) = 0;
    virtual int sqlite3_step(sqlite3_stmt* stmt) = 0;
    virtual int sqlite3_finalize(sqlite3_stmt* stmt) = 0;
    virtual const char* sqlite3_errmsg(sqlite3* db) = 0;
};

#ifdef GMOCK_AVAILABLE
class MockDatabase : public MockDatabaseInterface {
public:
    MOCK_METHOD(int, sqlite3_open, (const char* filename, sqlite3** ppDb), (override));
    MOCK_METHOD(int, sqlite3_close, (sqlite3* db), (override));
    MOCK_METHOD(int, sqlite3_exec, (sqlite3* db, const char* sql, 
                                   int (*callback)(void*,int,char**,char**), 
                                   void* arg, char** errmsg), (override));
    MOCK_METHOD(int, sqlite3_prepare_v2, (sqlite3* db, const char* zSql, int nByte, 
                                         sqlite3_stmt** ppStmt, const char** pzTail), (override));
    MOCK_METHOD(int, sqlite3_bind_text, (sqlite3_stmt* stmt, int index, const char* text, 
                                        int length, void(*destructor)(void*)), (override));
    MOCK_METHOD(int, sqlite3_bind_int, (sqlite3_stmt* stmt, int index, int value), (override));
    MOCK_METHOD(int, sqlite3_step, (sqlite3_stmt* stmt), (override));
    MOCK_METHOD(int, sqlite3_finalize, (sqlite3_stmt* stmt), (override));
    MOCK_METHOD(const char*, sqlite3_errmsg, (sqlite3* db), (override));
};
#endif

// =============================================================================
// MOCK FILESYSTEM OPERATIONS  
// =============================================================================

class MockFilesystemInterface {
public:
    virtual ~MockFilesystemInterface() = default;
    virtual bool exists(const std::filesystem::path& path) = 0;
    virtual bool is_regular_file(const std::filesystem::path& path) = 0;
    virtual bool is_directory(const std::filesystem::path& path) = 0;
    virtual std::uintmax_t file_size(const std::filesystem::path& path) = 0;
    virtual std::filesystem::file_time_type last_write_time(const std::filesystem::path& path) = 0;
    virtual bool create_directories(const std::filesystem::path& path) = 0;
    virtual bool remove(const std::filesystem::path& path) = 0;
    virtual bool remove_all(const std::filesystem::path& path) = 0;
};

#ifdef GMOCK_AVAILABLE
class MockFilesystem : public MockFilesystemInterface {
public:
    MOCK_METHOD(bool, exists, (const std::filesystem::path& path), (override));
    MOCK_METHOD(bool, is_regular_file, (const std::filesystem::path& path), (override));
    MOCK_METHOD(bool, is_directory, (const std::filesystem::path& path), (override));
    MOCK_METHOD(std::uintmax_t, file_size, (const std::filesystem::path& path), (override));
    MOCK_METHOD(std::filesystem::file_time_type, last_write_time, (const std::filesystem::path& path), (override));
    MOCK_METHOD(bool, create_directories, (const std::filesystem::path& path), (override));
    MOCK_METHOD(bool, remove, (const std::filesystem::path& path), (override));
    MOCK_METHOD(bool, remove_all, (const std::filesystem::path& path), (override));
};
#endif

// =============================================================================
// MOCK PYTHON EXECUTION
// =============================================================================

// Mock interface for testing Python execution - NOT for production use
// These methods are mocked for unit testing and do not execute actual commands
class MockPythonInterface {
public:
    virtual ~MockPythonInterface() = default;
    
    // Mock system call - does not execute actual commands in tests
    virtual int system(const char* command) = 0;
    
    // Mock popen - does not execute actual commands in tests
    // NOSONAR: This is a test mock interface, not actual command execution
    // CodeFactor-ignore: CWE-78 - This is a mock for testing only
    virtual FILE* popen(const char* command, const char* type) = 0;  // NOLINT(cert-env33-c)
    
    virtual int pclose(FILE* stream) = 0;
};

#ifdef GMOCK_AVAILABLE
class MockPython : public MockPythonInterface {
public:
    MOCK_METHOD(int, system, (const char* command), (override));
    
    // Mock implementation - does not execute actual commands
    // NOSONAR: This is a test mock, not actual command execution
    MOCK_METHOD(FILE*, popen, (const char* command, const char* type), (override));  // NOLINT(cert-env33-c)
    
    MOCK_METHOD(int, pclose, (FILE* stream), (override));
};
#endif

// =============================================================================
// TEST HELPERS FOR DEPENDENCY INJECTION
// =============================================================================

class TestDependencyInjector {
public:
    static TestDependencyInjector& getInstance() {
        static TestDependencyInjector instance;
        return instance;
    }
    
    void setCurlMock(std::shared_ptr<MockCurlInterface> mock) {
        curlMock = mock;
    }
    
    void setDatabaseMock(std::shared_ptr<MockDatabaseInterface> mock) {
        databaseMock = mock;
    }
    
    void setFilesystemMock(std::shared_ptr<MockFilesystemInterface> mock) {
        filesystemMock = mock;
    }
    
    void setPythonMock(std::shared_ptr<MockPythonInterface> mock) {
        pythonMock = mock;
    }
    
    std::shared_ptr<MockCurlInterface> getCurlMock() { return curlMock; }
    std::shared_ptr<MockDatabaseInterface> getDatabaseMock() { return databaseMock; }
    std::shared_ptr<MockFilesystemInterface> getFilesystemMock() { return filesystemMock; }
    std::shared_ptr<MockPythonInterface> getPythonMock() { return pythonMock; }
    
    void reset() {
        curlMock.reset();
        databaseMock.reset();
        filesystemMock.reset();
        pythonMock.reset();
    }

private:
    std::shared_ptr<MockCurlInterface> curlMock;
    std::shared_ptr<MockDatabaseInterface> databaseMock;
    std::shared_ptr<MockFilesystemInterface> filesystemMock;
    std::shared_ptr<MockPythonInterface> pythonMock;
};