#include <gtest/gtest.h>
#include "security.h"
#include <filesystem>

class SecurityTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// Test shell argument escaping for Unix
TEST_F(SecurityTest, EscapeShellArgUnix) {
    // Test normal string
    EXPECT_EQ(Security::escapeShellArgUnix("hello"), "'hello'");
    
    // Test string with spaces
    EXPECT_EQ(Security::escapeShellArgUnix("hello world"), "'hello world'");
    
    // Test string with single quotes (command injection attempt)
    EXPECT_EQ(Security::escapeShellArgUnix("file'; rm -rf /"), "'file'\\''; rm -rf /'");
    
    // Test string with backticks (command substitution attempt)
    EXPECT_EQ(Security::escapeShellArgUnix("file`whoami`"), "'file`whoami`'");
    
    // Test string with dollar signs (variable expansion attempt)
    EXPECT_EQ(Security::escapeShellArgUnix("file$PATH"), "'file$PATH'");
    
    // Test string with semicolon (command separator)
    EXPECT_EQ(Security::escapeShellArgUnix("file.mp3; cat /etc/passwd"), "'file.mp3; cat /etc/passwd'");
}

// Test shell argument escaping for Windows
TEST_F(SecurityTest, EscapeShellArgWindows) {
    // Test normal string
    EXPECT_EQ(Security::escapeShellArgWindows("hello"), "\"hello\"");
    
    // Test string with spaces
    EXPECT_EQ(Security::escapeShellArgWindows("hello world"), "\"hello world\"");
    
    // Test string with quotes (command injection attempt)
    EXPECT_EQ(Security::escapeShellArgWindows("file\" & del /f /q C:\\*"), "\"file\\\" & del /f /q C:\\*\"");
    
    // Test string with backslashes before quote
    EXPECT_EQ(Security::escapeShellArgWindows("C:\\path\\file\"name"), "\"C:\\path\\file\\\"name\"");
    
    // Test trailing backslashes
    EXPECT_EQ(Security::escapeShellArgWindows("C:\\path\\"), "\"C:\\path\\\\\"");
}

// Test path safety validation
TEST_F(SecurityTest, IsPathSafe) {
    std::filesystem::path baseDir = "/home/user/audio";
    
    // Test safe paths
    EXPECT_TRUE(Security::isPathSafe("/home/user/audio/file.mp3", baseDir));
    EXPECT_TRUE(Security::isPathSafe("/home/user/audio/subdir/file.mp3", baseDir));
    
    // Test unsafe paths (path traversal attempts)
    EXPECT_FALSE(Security::isPathSafe("/home/user/audio/../../../etc/passwd", baseDir));
    EXPECT_FALSE(Security::isPathSafe("/etc/passwd", baseDir));
    EXPECT_FALSE(Security::isPathSafe("/home/user/other/file.mp3", baseDir));
    
    // Test with relative paths
    std::filesystem::path currentDir = std::filesystem::current_path();
    EXPECT_TRUE(Security::isPathSafe(currentDir / "test.mp3", currentDir));
}

// Test file extension validation
TEST_F(SecurityTest, IsFileExtensionAllowed) {
    std::vector<std::string> allowedExtensions = {".mp3", ".wav", ".m4a"};
    
    // Test allowed extensions
    EXPECT_TRUE(Security::isFileExtensionAllowed("audio.mp3", allowedExtensions));
    EXPECT_TRUE(Security::isFileExtensionAllowed("audio.wav", allowedExtensions));
    EXPECT_TRUE(Security::isFileExtensionAllowed("audio.m4a", allowedExtensions));
    
    // Test case insensitivity
    EXPECT_TRUE(Security::isFileExtensionAllowed("audio.MP3", allowedExtensions));
    EXPECT_TRUE(Security::isFileExtensionAllowed("audio.WaV", allowedExtensions));
    
    // Test disallowed extensions
    EXPECT_FALSE(Security::isFileExtensionAllowed("script.sh", allowedExtensions));
    EXPECT_FALSE(Security::isFileExtensionAllowed("document.pdf", allowedExtensions));
    EXPECT_FALSE(Security::isFileExtensionAllowed("executable.exe", allowedExtensions));
    
    // Test files without extension
    EXPECT_FALSE(Security::isFileExtensionAllowed("noextension", allowedExtensions));
}

// Test filename sanitization
TEST_F(SecurityTest, SanitizeFilename) {
    // Test normal filename
    EXPECT_EQ(Security::sanitizeFilename("audio_file.mp3"), "audio_file.mp3");
    
    // Test filename with spaces and special characters
    EXPECT_EQ(Security::sanitizeFilename("audio file!@#$.mp3"), "audio_file____.mp3");
    
    // Test filename with path traversal attempt
    EXPECT_EQ(Security::sanitizeFilename("../../../etc/passwd"), "______etc_passwd");
    
    // Test filename with shell metacharacters
    EXPECT_EQ(Security::sanitizeFilename("file;rm -rf /.mp3"), "file_rm_-rf__.mp3");
    
    // Test hidden file attempt
    EXPECT_EQ(Security::sanitizeFilename(".hidden"), "_hidden");
}

// Test safe character validation
TEST_F(SecurityTest, ContainsOnlySafeCharacters) {
    // Test safe strings
    EXPECT_TRUE(Security::containsOnlySafeCharacters("audio_file.mp3"));
    EXPECT_TRUE(Security::containsOnlySafeCharacters("/home/user/audio/file.mp3"));
    EXPECT_TRUE(Security::containsOnlySafeCharacters("file-name_123.wav"));
    
    // Test unsafe strings with shell metacharacters
    EXPECT_FALSE(Security::containsOnlySafeCharacters("file;command"));
    EXPECT_FALSE(Security::containsOnlySafeCharacters("file&command"));
    EXPECT_FALSE(Security::containsOnlySafeCharacters("file|command"));
    EXPECT_FALSE(Security::containsOnlySafeCharacters("file>output"));
    EXPECT_FALSE(Security::containsOnlySafeCharacters("file<input"));
    EXPECT_FALSE(Security::containsOnlySafeCharacters("file`command`"));
    EXPECT_FALSE(Security::containsOnlySafeCharacters("file$(command)"));
    EXPECT_FALSE(Security::containsOnlySafeCharacters("file'quote"));
    EXPECT_FALSE(Security::containsOnlySafeCharacters("file\"quote"));
}