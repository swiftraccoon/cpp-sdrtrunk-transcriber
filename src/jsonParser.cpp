#include "../include/jsonParser.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

JsonParser::JsonObject JsonParser::parseFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open JSON file: " + filePath);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return parseString(buffer.str());
}

JsonParser::JsonObject JsonParser::parseString(const std::string& jsonStr) {
    return parse(jsonStr);
}

std::string JsonParser::trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, (last - first + 1));
}

void JsonParser::skipWhitespace(const std::string& json, size_t& pos) {
    while (pos < json.length() && std::isspace(json[pos])) {
        pos++;
    }
}

std::string JsonParser::parseStringValue(const std::string& json, size_t& pos) {
    std::string result;
    pos++; // Skip opening quote

    while (pos < json.length() && json[pos] != '"') {
        if (json[pos] == '\\' && pos + 1 < json.length()) {
            // Handle escape sequences
            pos++;
            switch (json[pos]) {
                case 'n': result += '\n'; break;
                case 't': result += '\t'; break;
                case 'r': result += '\r'; break;
                case '\\': result += '\\'; break;
                case '"': result += '"'; break;
                default: result += json[pos]; break;
            }
        } else {
            result += json[pos];
        }
        pos++;
    }

    if (pos < json.length()) pos++; // Skip closing quote
    return result;
}

JsonParser::JsonValue JsonParser::parseValue(const std::string& json, size_t& pos) {
    skipWhitespace(json, pos);

    if (pos >= json.length()) {
        throw std::runtime_error("Unexpected end of JSON");
    }

    // String value
    if (json[pos] == '"') {
        return parseStringValue(json, pos);
    }

    // Boolean true
    if (json.substr(pos, 4) == "true") {
        pos += 4;
        return true;
    }

    // Boolean false
    if (json.substr(pos, 5) == "false") {
        pos += 5;
        return false;
    }

    // Null
    if (json.substr(pos, 4) == "null") {
        pos += 4;
        return nullptr;
    }

    // Number
    if (json[pos] == '-' || std::isdigit(json[pos])) {
        size_t start = pos;
        if (json[pos] == '-') pos++;

        while (pos < json.length() && std::isdigit(json[pos])) pos++;

        if (pos < json.length() && json[pos] == '.') {
            pos++;
            while (pos < json.length() && std::isdigit(json[pos])) pos++;
        }

        if (pos < json.length() && (json[pos] == 'e' || json[pos] == 'E')) {
            pos++;
            if (pos < json.length() && (json[pos] == '+' || json[pos] == '-')) pos++;
            while (pos < json.length() && std::isdigit(json[pos])) pos++;
        }

        return std::stod(json.substr(start, pos - start));
    }

    // Array (treated as string for simplicity in glossary use case)
    if (json[pos] == '[') {
        size_t start = pos;
        int depth = 1;
        pos++;
        while (pos < json.length() && depth > 0) {
            if (json[pos] == '[') depth++;
            else if (json[pos] == ']') depth--;
            pos++;
        }
        return json.substr(start, pos - start);
    }

    // Object (treated as string for simplicity in glossary use case)
    if (json[pos] == '{') {
        size_t start = pos;
        int depth = 1;
        pos++;
        while (pos < json.length() && depth > 0) {
            if (json[pos] == '{') depth++;
            else if (json[pos] == '}') depth--;
            pos++;
        }
        return json.substr(start, pos - start);
    }

    throw std::runtime_error("Invalid JSON value at position " + std::to_string(pos));
}

std::vector<std::string> JsonParser::parseStringArray(const std::string& json, size_t& pos) {
    std::vector<std::string> result;

    if (pos >= json.length() || json[pos] != '[') {
        throw std::runtime_error("Expected '[' at position " + std::to_string(pos));
    }

    pos++; // Skip '['
    skipWhitespace(json, pos);

    // Empty array
    if (pos < json.length() && json[pos] == ']') {
        pos++;
        return result;
    }

    while (pos < json.length()) {
        skipWhitespace(json, pos);

        if (json[pos] == '"') {
            result.push_back(parseStringValue(json, pos));
        } else {
            throw std::runtime_error("Expected string in array at position " + std::to_string(pos));
        }

        skipWhitespace(json, pos);

        if (pos < json.length() && json[pos] == ',') {
            pos++;
        } else if (pos < json.length() && json[pos] == ']') {
            pos++;
            break;
        } else {
            throw std::runtime_error("Expected ',' or ']' in array at position " + std::to_string(pos));
        }
    }

    return result;
}

JsonParser::JsonObject JsonParser::parseObject(const std::string& json, size_t& pos) {
    JsonObject result;

    if (pos >= json.length() || json[pos] != '{') {
        throw std::runtime_error("Expected '{' at position " + std::to_string(pos));
    }

    pos++; // Skip '{'
    skipWhitespace(json, pos);

    if (pos < json.length() && json[pos] == '}') {
        pos++;
        return result;
    }

    while (pos < json.length()) {
        skipWhitespace(json, pos);

        if (json[pos] == '}') {
            pos++;
            break;
        }

        if (json[pos] != '"') {
            throw std::runtime_error("Expected string key at position " + std::to_string(pos));
        }

        std::string key = parseStringValue(json, pos);

        skipWhitespace(json, pos);

        if (pos >= json.length() || json[pos] != ':') {
            throw std::runtime_error("Expected ':' after key at position " + std::to_string(pos));
        }

        pos++; // Skip ':'
        skipWhitespace(json, pos);

        result[key] = parseValue(json, pos);

        skipWhitespace(json, pos);

        if (pos < json.length()) {
            if (json[pos] == ',') {
                pos++;
            } else if (json[pos] != '}') {
                throw std::runtime_error("Expected ',' or '}' at position " + std::to_string(pos));
            }
        }
    }

    return result;
}

std::vector<GlossaryEntry> JsonParser::parseGlossaryFile(const std::string& filePath) {
    std::vector<GlossaryEntry> entries;

    std::ifstream file(filePath);
    if (!file.is_open()) {
        return entries;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string json = buffer.str();

    size_t pos = 0;
    skipWhitespace(json, pos);

    if (pos >= json.length() || json[pos] != '{') {
        return entries;
    }

    pos++; // Skip '{'
    skipWhitespace(json, pos);

    // Look for "GLOSSARY" key
    bool foundGlossary = false;
    while (pos < json.length() && json[pos] != '}') {
        skipWhitespace(json, pos);

        if (json[pos] != '"') break;

        std::string key = parseStringValue(json, pos);
        skipWhitespace(json, pos);

        if (pos >= json.length() || json[pos] != ':') break;
        pos++; // Skip ':'
        skipWhitespace(json, pos);

        if (key == "GLOSSARY" && pos < json.length() && json[pos] == '[') {
            foundGlossary = true;
            pos++; // Skip '['
            skipWhitespace(json, pos);

            // Parse array of glossary entry objects
            while (pos < json.length() && json[pos] != ']') {
                skipWhitespace(json, pos);

                if (json[pos] == '{') {
                    // Parse individual entry object
                    pos++; // Skip '{'
                    skipWhitespace(json, pos);

                    GlossaryEntry entry;

                    while (pos < json.length() && json[pos] != '}') {
                        skipWhitespace(json, pos);

                        if (json[pos] != '"') break;

                        std::string entryKey = parseStringValue(json, pos);
                        skipWhitespace(json, pos);

                        if (pos >= json.length() || json[pos] != ':') break;
                        pos++; // Skip ':'
                        skipWhitespace(json, pos);

                        if (entryKey == "keys" && json[pos] == '[') {
                            entry.keys = parseStringArray(json, pos);
                        } else if (entryKey == "value" && json[pos] == '"') {
                            entry.value = parseStringValue(json, pos);
                        } else {
                            // Skip unknown value
                            parseValue(json, pos);
                        }

                        skipWhitespace(json, pos);
                        if (pos < json.length() && json[pos] == ',') pos++;
                    }

                    if (pos < json.length() && json[pos] == '}') pos++; // Skip '}'

                    if (!entry.keys.empty() && !entry.value.empty()) {
                        entries.push_back(std::move(entry));
                    }
                }

                skipWhitespace(json, pos);
                if (pos < json.length() && json[pos] == ',') pos++;
            }

            if (pos < json.length() && json[pos] == ']') pos++; // Skip ']'
        } else {
            // Skip this value
            parseValue(json, pos);
        }

        skipWhitespace(json, pos);
        if (pos < json.length() && json[pos] == ',') pos++;
    }

    if (!foundGlossary) {
        entries.clear(); // Signal to caller to use old flat format
    }

    return entries;
}

JsonParser::JsonObject JsonParser::parse(const std::string& json) {
    JsonObject result;
    size_t pos = 0;

    skipWhitespace(json, pos);

    if (pos >= json.length() || json[pos] != '{') {
        throw std::runtime_error("JSON must start with '{'");
    }

    pos++; // Skip opening brace
    skipWhitespace(json, pos);

    // Empty object
    if (pos < json.length() && json[pos] == '}') {
        return result;
    }

    while (pos < json.length()) {
        skipWhitespace(json, pos);

        // Check for end of object
        if (json[pos] == '}') {
            break;
        }

        // Parse key
        if (json[pos] != '"') {
            throw std::runtime_error("Expected string key at position " + std::to_string(pos));
        }

        std::string key = parseStringValue(json, pos);

        skipWhitespace(json, pos);

        // Expect colon
        if (pos >= json.length() || json[pos] != ':') {
            throw std::runtime_error("Expected ':' after key at position " + std::to_string(pos));
        }

        pos++; // Skip colon
        skipWhitespace(json, pos);

        // Parse value
        result[key] = parseValue(json, pos);

        skipWhitespace(json, pos);

        // Check for comma or end of object
        if (pos < json.length()) {
            if (json[pos] == ',') {
                pos++;
            } else if (json[pos] != '}') {
                throw std::runtime_error("Expected ',' or '}' at position " + std::to_string(pos));
            }
        }
    }

    return result;
}
