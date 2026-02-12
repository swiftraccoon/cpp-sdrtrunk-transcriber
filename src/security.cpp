#include "security.h"
#include <algorithm>
#include <ranges>
#include <regex>
#include <filesystem>

namespace Security {
    
    std::string escapeShellArgUnix(const std::string& arg) {
        // Use single quotes to escape most special characters
        // Handle embedded single quotes by ending quote, adding escaped quote, starting new quote
        std::string escaped = "'";
        for (char c : arg) {
            if (c == '\'') {
                escaped += "'\\''";  // End quote, escaped quote, start quote
            } else {
                escaped += c;
            }
        }
        escaped += "'";
        return escaped;
    }
    
    std::string escapeShellArgWindows(const std::string& arg) {
        // Windows command line escaping is complex
        // We need to handle quotes and backslashes specially
        std::string escaped = "\"";
        int backslashCount = 0;
        
        for (char c : arg) {
            if (c == '\\') {
                backslashCount++;
            } else if (c == '"') {
                // Escape all backslashes and the quote
                escaped.append(backslashCount * 2 + 1, '\\');
                escaped += '"';
                backslashCount = 0;
            } else {
                // Output accumulated backslashes
                escaped.append(backslashCount, '\\');
                escaped += c;
                backslashCount = 0;
            }
        }
        
        // Handle trailing backslashes
        escaped.append(backslashCount * 2, '\\');
        escaped += "\"";
        return escaped;
    }
    
    std::string escapeShellArg(const std::string& arg) {
#ifdef _WIN32
        return escapeShellArgWindows(arg);
#else
        return escapeShellArgUnix(arg);
#endif
    }
    
    bool isPathSafe(const std::filesystem::path& path, const std::filesystem::path& baseDir) {
        try {
            // Get canonical paths to resolve symlinks and .. references
            auto normalizedPath = std::filesystem::weakly_canonical(path);
            auto normalizedBase = std::filesystem::weakly_canonical(baseDir);
            
            // Convert to string for comparison
            std::string pathStr = normalizedPath.string();
            std::string baseStr = normalizedBase.string();
            
            // Ensure base ends with separator for accurate comparison
            if (!baseStr.empty() && baseStr.back() != std::filesystem::path::preferred_separator) {
                baseStr += std::filesystem::path::preferred_separator;
            }
            
            // Check if path starts with base directory
            return pathStr.find(baseStr) == 0;
        } catch (const std::filesystem::filesystem_error&) {
            // If we can't resolve the path, consider it unsafe
            return false;
        }
    }
    
    bool isFileExtensionAllowed(const std::filesystem::path& path, const std::vector<std::string>& allowedExtensions) {
        std::string extension = path.extension().string();
        
        // Convert to lowercase for case-insensitive comparison
        std::ranges::transform(extension, extension.begin(), 
                               [](unsigned char c) { return std::tolower(c); });
        
        for (const auto& allowed : allowedExtensions) {
            std::string allowedLower = allowed;
            std::ranges::transform(allowedLower, allowedLower.begin(),
                                  [](unsigned char c) { return std::tolower(c); });
            
            // Add dot if not present in allowed extension
            if (!allowedLower.empty() && allowedLower[0] != '.') {
                allowedLower = "." + allowedLower;
            }
            
            if (extension == allowedLower) {
                return true;
            }
        }
        
        return false;
    }
    
    bool isFileSizeAcceptable(const std::filesystem::path& path, size_t maxSize) {
        try {
            if (!std::filesystem::exists(path)) {
                return false;
            }
            
            auto fileSize = std::filesystem::file_size(path);
            return fileSize <= maxSize;
        } catch (const std::filesystem::filesystem_error&) {
            return false;
        }
    }
    
    std::string sanitizeFilename(const std::string& filename) {
        std::string sanitized;
        
        for (char c : filename) {
            // Allow alphanumeric, dots, hyphens, and underscores
            if (std::isalnum(c) || c == '.' || c == '-' || c == '_') {
                sanitized += c;
            } else {
                // Replace invalid characters with underscore
                sanitized += '_';
            }
        }
        
        // Prevent double dots (parent directory reference)
        std::regex doubleDot("\\.\\.");
        sanitized = std::regex_replace(sanitized, doubleDot, "_");
        
        // Prevent leading dots (hidden files on Unix)
        if (!sanitized.empty() && sanitized[0] == '.') {
            sanitized[0] = '_';
        }
        
        return sanitized;
    }
    
    bool containsOnlySafeCharacters(const std::string& str) {
        // Define safe characters: alphanumeric, common punctuation, spaces
        // Exclude shell metacharacters: ; & | > < ` $ ( ) { } [ ] ! \ " '
        std::regex safePattern("^[a-zA-Z0-9._/ -]+$");
        return std::regex_match(str, safePattern);
    }
}