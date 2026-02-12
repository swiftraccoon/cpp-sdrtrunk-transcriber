#pragma once

#include <chrono>
#include <filesystem>
#include <string>
#include <compare>

namespace sdrtrunk::domain {

// Strong type wrapper template
template<typename T, typename Tag>
class StrongType {
public:
    StrongType() : value_{} {}
    explicit StrongType(T value) : value_(std::move(value)) {}

    const T& get() const { return value_; }
    T& get() { return value_; }
    
    // Enable comparison operators for C++20
    auto operator<=>(const StrongType&) const = default;
    
    // Conversion operator for convenience (use carefully)
    explicit operator const T&() const { return value_; }
    explicit operator T&() { return value_; }
    
private:
    T value_;
};

// Domain-specific type aliases
struct TalkgroupIdTag {};
using TalkgroupId = StrongType<int, TalkgroupIdTag>;

struct RadioIdTag {};
using RadioId = StrongType<int, RadioIdTag>;

struct DurationTag {};
using Duration = StrongType<std::chrono::seconds, DurationTag>;

struct FilePathTag {};
using FilePath = StrongType<std::filesystem::path, FilePathTag>;

struct TranscriptionTag {};
using Transcription = StrongType<std::string, TranscriptionTag>;

// Helper functions for domain types
inline TalkgroupId makeTalkgroupId(int id) {
    return TalkgroupId(id);
}

inline RadioId makeRadioId(int id) {
    return RadioId(id);
}

inline Duration makeDuration(std::chrono::seconds secs) {
    return Duration(secs);
}

inline FilePath makeFilePath(const std::filesystem::path& path) {
    return FilePath(path);
}

inline Transcription makeTranscription(const std::string& text) {
    return Transcription(text);
}

} // namespace sdrtrunk::domain