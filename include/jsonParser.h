#ifndef JSON_PARSER_H
#define JSON_PARSER_H

#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
#include <stdexcept>

struct GlossaryEntry
{
    std::vector<std::string> keys;
    std::string value;
};

class JsonParser {
public:
    using JsonValue = std::variant<std::string, double, bool, std::nullptr_t>;
    using JsonObject = std::unordered_map<std::string, JsonValue>;

    // Parse a JSON file and return a map of key-value pairs
    // Only supports simple objects with string, number, boolean, and null values
    static JsonObject parseFile(const std::string& filePath);

    // Parse a JSON string
    static JsonObject parseString(const std::string& jsonStr);

    // Parse a glossary file supporting both old flat format and new multi-key format
    // Returns empty vector if file is old flat format (caller should fall back)
    static std::vector<GlossaryEntry> parseGlossaryFile(const std::string& filePath);

private:
    static JsonObject parse(const std::string& json);
    static std::string trim(const std::string& str);
    static std::string parseStringValue(const std::string& json, size_t& pos);
    static JsonValue parseValue(const std::string& json, size_t& pos);
    static void skipWhitespace(const std::string& json, size_t& pos);
    static std::vector<std::string> parseStringArray(const std::string& json, size_t& pos);
    static JsonObject parseObject(const std::string& json, size_t& pos);
};

#endif // JSON_PARSER_H