#pragma once

#include <expected>
#include <string>
#include <system_error>
#include <variant>

// C++23 std::expected-based Result type for error handling
// This replaces exceptions and error codes with a monadic Result type

namespace sdrtrunk {

// Error types we might encounter
enum class ErrorCode {
    FileNotFound,
    InvalidPath,
    InvalidFormat,
    TranscriptionFailed,
    DatabaseError,
    NetworkError,
    ConfigError,
    SystemError,
    PermissionDenied,
    ResourceExhausted,
    Timeout,
    Unknown
};

// Convert ErrorCode to string for debugging
inline std::string errorCodeToString(ErrorCode code) {
    switch (code) {
        case ErrorCode::FileNotFound: return "File not found";
        case ErrorCode::InvalidPath: return "Invalid path";
        case ErrorCode::InvalidFormat: return "Invalid format";
        case ErrorCode::TranscriptionFailed: return "Transcription failed";
        case ErrorCode::DatabaseError: return "Database error";
        case ErrorCode::NetworkError: return "Network error";
        case ErrorCode::ConfigError: return "Configuration error";
        case ErrorCode::SystemError: return "System error";
        case ErrorCode::PermissionDenied: return "Permission denied";
        case ErrorCode::ResourceExhausted: return "Resource exhausted";
        case ErrorCode::Timeout: return "Operation timed out";
        case ErrorCode::Unknown: return "Unknown error";
        default: return "Unspecified error";
    }
}

// Error information with code and message
struct Error {
    ErrorCode code;
    std::string message;
    std::string context;  // Optional context about where the error occurred
    
    Error(ErrorCode c, const std::string& msg = "", const std::string& ctx = "")
        : code(c), message(msg.empty() ? errorCodeToString(c) : msg), context(ctx) {}
    
    std::string toString() const {
        std::string result = "[" + errorCodeToString(code) + "] " + message;
        if (!context.empty()) {
            result += " (at " + context + ")";
        }
        return result;
    }
};

// Main Result type using std::expected
template<typename T>
using Result = std::expected<T, Error>;

// Convenience factory functions
template<typename T>
Result<typename std::remove_cvref_t<T>> Ok(T&& value) {
    using ValueType = typename std::remove_cvref_t<T>;
    return Result<ValueType>(std::forward<T>(value));
}

template<typename T>
Result<T> Err(ErrorCode code, const std::string& message = "", const std::string& context = "") {
    return std::unexpected(Error(code, message, context));
}

// Monadic operations helpers (C++23 provides these, but let's add convenience versions)

// Map a function over a successful result
template<typename T, typename F>
auto map(const Result<T>& result, F&& func) -> Result<decltype(func(std::declval<T>()))> {
    if (result.has_value()) {
        return Ok(func(result.value()));
    }
    return std::unexpected(result.error());
}

// FlatMap (and_then) - chain operations that return Results
template<typename T, typename F>
auto andThen(const Result<T>& result, F&& func) -> decltype(func(std::declval<T>())) {
    if (result.has_value()) {
        return func(result.value());
    }
    return std::unexpected(result.error());
}

// Provide a default value if error
template<typename T>
T orElse(const Result<T>& result, T&& defaultValue) {
    if (result.has_value()) {
        return result.value();
    }
    return std::forward<T>(defaultValue);
}

// Execute a function if error (for logging, etc.)
template<typename T, typename F>
const Result<T>& onError(const Result<T>& result, F&& errorHandler) {
    if (!result.has_value()) {
        errorHandler(result.error());
    }
    return result;
}

// TRY macro for early return on error (similar to Rust's ? operator)
#define TRY(expr) \
    do { \
        auto _result = (expr); \
        if (!_result.has_value()) { \
            return std::unexpected(_result.error()); \
        } \
    } while(0)

// TRY with value extraction
#define TRY_ASSIGN(var, expr) \
    auto _##var##_result = (expr); \
    if (!_##var##_result.has_value()) { \
        return std::unexpected(_##var##_result.error()); \
    } \
    auto var = std::move(_##var##_result.value())

} // namespace sdrtrunk