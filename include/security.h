#pragma once

#include <string>
#include <filesystem>
#include <vector>

namespace Security {
    /**
     * Escapes a string for safe use in shell commands on Unix-like systems
     * Uses single quotes and handles embedded single quotes
     */
    std::string escapeShellArgUnix(const std::string& arg);
    
    /**
     * Escapes a string for safe use in shell commands on Windows
     * Uses double quotes and handles embedded quotes and special characters
     */
    std::string escapeShellArgWindows(const std::string& arg);
    
    /**
     * Cross-platform shell argument escaping
     * Automatically selects the appropriate escaping method based on the platform
     */
    std::string escapeShellArg(const std::string& arg);
    
    /**
     * Validates that a path is within the allowed base directory
     * Prevents directory traversal attacks
     */
    bool isPathSafe(const std::filesystem::path& path, const std::filesystem::path& baseDir);
    
    /**
     * Validates file extension against a whitelist
     */
    bool isFileExtensionAllowed(const std::filesystem::path& path, const std::vector<std::string>& allowedExtensions);
    
    /**
     * Validates file size is within acceptable limits
     */
    bool isFileSizeAcceptable(const std::filesystem::path& path, size_t maxSize);
    
    /**
     * Sanitizes a filename by removing dangerous characters
     * Keeps only alphanumeric, dots, hyphens, and underscores
     */
    std::string sanitizeFilename(const std::string& filename);
    
    /**
     * Validates that a string contains only safe characters for a file path
     */
    bool containsOnlySafeCharacters(const std::string& str);
}