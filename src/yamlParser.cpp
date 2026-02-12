#include "../include/yamlParser.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <stack>
#include <iostream>

YamlNode YamlNode::emptyNode;

template<> std::string YamlNode::as<std::string>() const {
    if (std::holds_alternative<std::string>(value_)) {
        return std::get<std::string>(value_);
    }
    if (std::holds_alternative<int>(value_)) {
        return std::to_string(std::get<int>(value_));
    }
    if (std::holds_alternative<bool>(value_)) {
        return std::get<bool>(value_) ? "true" : "false";
    }
    throw std::runtime_error("Cannot convert to string");
}

template<> int YamlNode::as<int>() const {
    if (std::holds_alternative<int>(value_)) {
        return std::get<int>(value_);
    }
    if (std::holds_alternative<std::string>(value_)) {
        return std::stoi(std::get<std::string>(value_));
    }
    throw std::runtime_error("Cannot convert to int");
}

template<> bool YamlNode::as<bool>() const {
    if (std::holds_alternative<bool>(value_)) {
        return std::get<bool>(value_);
    }
    if (std::holds_alternative<std::string>(value_)) {
        std::string val = std::get<std::string>(value_);
        return (val == "true" || val == "1" || val == "yes");
    }
    if (std::holds_alternative<int>(value_)) {
        return std::get<int>(value_) != 0;
    }
    throw std::runtime_error("Cannot convert to bool");
}

YamlNode& YamlNode::operator[](const std::string& key) {
    if (!std::holds_alternative<std::unordered_map<std::string, std::shared_ptr<YamlNode>>>(value_)) {
        value_ = std::unordered_map<std::string, std::shared_ptr<YamlNode>>();
    }
    auto& map = std::get<std::unordered_map<std::string, std::shared_ptr<YamlNode>>>(value_);
    if (map.find(key) == map.end()) {
        map[key] = std::make_shared<YamlNode>();
    }
    return *map[key];
}

const YamlNode& YamlNode::operator[](const std::string& key) const {
    if (std::holds_alternative<std::unordered_map<std::string, std::shared_ptr<YamlNode>>>(value_)) {
        const auto& map = std::get<std::unordered_map<std::string, std::shared_ptr<YamlNode>>>(value_);
        auto it = map.find(key);
        if (it != map.end()) {
            return *(it->second);
        }
    }
    return emptyNode;
}

bool YamlNode::hasKey(const std::string& key) const {
    if (std::holds_alternative<std::unordered_map<std::string, std::shared_ptr<YamlNode>>>(value_)) {
        const auto& map = std::get<std::unordered_map<std::string, std::shared_ptr<YamlNode>>>(value_);
        return map.find(key) != map.end();
    }
    return false;
}

std::vector<std::string> YamlNode::getKeys() const {
    std::vector<std::string> keys;
    if (std::holds_alternative<std::unordered_map<std::string, std::shared_ptr<YamlNode>>>(value_)) {
        const auto& map = std::get<std::unordered_map<std::string, std::shared_ptr<YamlNode>>>(value_);
        for (const auto& [key, _] : map) {
            keys.push_back(key);
        }
    }
    return keys;
}

void YamlNode::push_back(const std::string& item) {
    if (!std::holds_alternative<std::vector<std::string>>(value_)) {
        value_ = std::vector<std::string>();
    }
    auto& vec = std::get<std::vector<std::string>>(value_);
    vec.push_back(item);
}

std::string YamlNode::toString() const {
    return toStringWithIndent(0);
}

std::string YamlNode::toStringWithIndent(int indent) const {
    return toStringWithIndentInternal(indent, true);
}

std::string YamlNode::toStringWithIndentInternal(int indent, bool isFirst) const {
    std::stringstream ss;
    std::string indentStr(indent * 2, ' ');
    
    if (std::holds_alternative<std::string>(value_)) {
        ss << std::get<std::string>(value_);
    } else if (std::holds_alternative<int>(value_)) {
        ss << std::get<int>(value_);
    } else if (std::holds_alternative<bool>(value_)) {
        ss << (std::get<bool>(value_) ? "true" : "false");
    } else if (std::holds_alternative<std::vector<std::string>>(value_)) {
        const auto& vec = std::get<std::vector<std::string>>(value_);
        ss << "[";
        for (size_t i = 0; i < vec.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << "\"" << vec[i] << "\"";
        }
        ss << "]";
    } else if (std::holds_alternative<std::unordered_map<std::string, std::shared_ptr<YamlNode>>>(value_)) {
        const auto& map = std::get<std::unordered_map<std::string, std::shared_ptr<YamlNode>>>(value_);
        size_t count = 0;
        for (const auto& [key, node] : map) {
            // Add newline before all entries except when this is root level first entry
            if (count > 0 || (indent > 0 && !isFirst)) {
                ss << "\n";
            }
            
            // Add indentation for non-root entries
            if (indent > 0) {
                ss << indentStr;
            }
            
            ss << key << ":";
            
            // Check if the node is a simple value or needs indentation
            if (node->isMap()) {
                ss << "\n";
                // Recursively get the content with proper indentation
                ss << node->toStringWithIndentInternal(indent + 1, false);
            } else {
                ss << " " << node->toStringWithIndentInternal(0, true);
            }
            
            count++;
        }
    }
    
    return ss.str();
}

bool YamlNode::isMap() const {
    return std::holds_alternative<std::unordered_map<std::string, std::shared_ptr<YamlNode>>>(value_);
}

std::string YamlParser::trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, (last - first + 1));
}

int YamlParser::getIndentLevel(const std::string& line) {
    int count = 0;
    for (char c : line) {
        if (c == ' ') count++;
        else if (c == '\t') count += 4;  // Treat tab as 4 spaces
        else break;
    }
    return count;
}

std::pair<std::string, std::string> YamlParser::parseKeyValue(const std::string& line) {
    size_t colonPos = line.find(':');
    if (colonPos == std::string::npos) {
        return {"", ""};
    }
    
    std::string key = trim(line.substr(0, colonPos));
    std::string value = "";
    
    if (colonPos + 1 < line.length()) {
        value = trim(line.substr(colonPos + 1));
        // Remove quotes if present
        if (value.length() >= 2 && value[0] == '"' && value.back() == '"') {
            value = value.substr(1, value.length() - 2);
        }
    }
    
    return {key, value};
}

bool YamlParser::isList(const std::string& trimmedLine) {
    return !trimmedLine.empty() && trimmedLine[0] == '[';
}

std::vector<std::string> YamlParser::parseList(const std::string& value) {
    std::vector<std::string> result;
    
    // Remove brackets
    std::string content = value;
    if (!content.empty() && content[0] == '[' && content.back() == ']') {
        content = content.substr(1, content.length() - 2);
    }
    
    // Parse comma-separated values
    std::stringstream ss(content);
    std::string item;
    while (std::getline(ss, item, ',')) {
        item = trim(item);
        // Remove quotes if present
        if (item.length() >= 2 && item[0] == '"' && item.back() == '"') {
            item = item.substr(1, item.length() - 2);
        }
        if (!item.empty()) {
            result.push_back(item);
        }
    }
    
    return result;
}

YamlNode YamlParser::loadFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open YAML file: " + filePath);
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return parseString(buffer.str());
}

YamlNode YamlParser::parseString(const std::string& yamlStr) {
    std::vector<std::string> lines;
    std::stringstream ss(yamlStr);
    std::string line;
    while (std::getline(ss, line)) {
        // Skip comments and empty lines
        std::string trimmedLine = trim(line);
        if (!trimmedLine.empty() && trimmedLine[0] != '#') {
            lines.push_back(line);
        }
    }
    return parse(lines);
}

YamlNode YamlParser::parse(const std::vector<std::string>& lines) {
    YamlNode root;
    std::unordered_map<std::string, std::shared_ptr<YamlNode>> rootMap;
    
    struct StackItem {
        YamlNode* node;
        int indent;
        std::string key;
    };
    
    std::stack<StackItem> nodeStack;
    nodeStack.push({&root, -1, ""});
    
    for (size_t i = 0; i < lines.size(); ++i) {
        const std::string& line = lines[i];
        std::string trimmedLine = trim(line);
        if (trimmedLine.empty()) continue;
        
        int indent = getIndentLevel(line);
        auto [key, value] = parseKeyValue(trimmedLine);
        
        if (key.empty()) continue;
        
        // Pop stack until we find parent with lower indent
        while (!nodeStack.empty() && nodeStack.top().indent >= indent) {
            nodeStack.pop();
        }
        
        if (nodeStack.empty()) {
            throw std::runtime_error("Invalid YAML structure");
        }
        
        YamlNode* parent = nodeStack.top().node;
        
        // Check if this is a list value
        if (!value.empty() && isList(value)) {
            auto listValues = parseList(value);
            (*parent)[key].setValue(listValues);
        }
        // Check if value is empty (node will have children)
        else if (value.empty()) {
            // Look ahead to see if next line is a list
            if (i + 1 < lines.size()) {
                std::string nextLine = trim(lines[i + 1]);
                int nextIndent = getIndentLevel(lines[i + 1]);
                
                // Check for GLOSSARY pattern (special case for list)
                if (!nextLine.empty() && nextLine[0] == '[' && nextIndent > indent) {
                    // This is a GLOSSARY-style list
                    auto listValues = parseList(nextLine);
                    YamlNode& childNode = (*parent)[key];
                    childNode["GLOSSARY"].setValue(listValues);
                    i++; // Skip the next line since we processed it
                } else {
                    // Regular nested structure
                    nodeStack.push({&(*parent)[key], indent, key});
                }
            } else {
                nodeStack.push({&(*parent)[key], indent, key});
            }
        }
        // Regular key-value pair
        else {
            // Try to parse as int
            try {
                size_t pos;
                int intVal = std::stoi(value, &pos);
                if (pos == value.length()) {
                    (*parent)[key].setValue(intVal);
                } else {
                    (*parent)[key].setValue(value);
                }
            } catch (...) {
                // Try to parse as bool
                if (value == "true" || value == "false") {
                    (*parent)[key].setValue(value == "true");
                } else {
                    (*parent)[key].setValue(value);
                }
            }
        }
    }
    
    return root;
}