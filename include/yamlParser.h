#ifndef YAML_PARSER_H
#define YAML_PARSER_H

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <variant>
#include <iostream>

class YamlNode {
public:
    using ValueType = std::variant<
        std::string,
        int,
        bool,
        std::vector<std::string>,
        std::unordered_map<std::string, std::shared_ptr<YamlNode>>
    >;
    
    YamlNode() = default;
    explicit YamlNode(const std::string& value) : value_(value) {}
    explicit YamlNode(int value) : value_(value) {}
    explicit YamlNode(bool value) : value_(value) {}
    explicit YamlNode(const std::vector<std::string>& value) : value_(value) {}
    
    // Assignment operators for convenience
    YamlNode& operator=(const std::string& value) { value_ = value; return *this; }
    YamlNode& operator=(const char* value) { value_ = std::string(value); return *this; }
    YamlNode& operator=(int value) { value_ = value; return *this; }
    YamlNode& operator=(bool value) { value_ = value; return *this; }
    YamlNode& operator=(const std::vector<std::string>& value) { value_ = value; return *this; }
    
    // Template for as<T>() conversions
    template<typename T>
    T as() const;
    
    // Access child nodes for maps
    YamlNode& operator[](const std::string& key);
    const YamlNode& operator[](const std::string& key) const;
    
    // Check if a key exists
    bool hasKey(const std::string& key) const;
    
    // Get all keys for iteration
    std::vector<std::string> getKeys() const;
    
    // Add item to vector (for list nodes)
    void push_back(const std::string& item);
    
    // Convert to string representation (for serialization)
    std::string toString() const;
    std::string toStringWithIndent(int indent) const;
    std::string toStringWithIndentInternal(int indent, bool isFirst) const;
    bool isMap() const;
    
    // Set value (for internal use)
    void setValue(const ValueType& value) { value_ = value; }
    ValueType& getValue() { return value_; }
    const ValueType& getValue() const { return value_; }
    
private:
    ValueType value_;
    static YamlNode emptyNode;
};

// Template specializations
template<> std::string YamlNode::as<std::string>() const;
template<> int YamlNode::as<int>() const;
template<> bool YamlNode::as<bool>() const;

// Stream operator for YamlNode
inline std::ostream& operator<<(std::ostream& os, const YamlNode& node) {
    os << node.toString();
    return os;
}

class YamlParser {
public:
    static YamlNode loadFile(const std::string& filePath);
    static YamlNode parseString(const std::string& yamlStr);
    
private:
    static YamlNode parse(const std::vector<std::string>& lines);
    static std::string trim(const std::string& str);
    static int getIndentLevel(const std::string& line);
    static std::pair<std::string, std::string> parseKeyValue(const std::string& line);
    static std::vector<std::string> parseList(const std::string& value);
    static bool isList(const std::string& trimmedLine);
};

#endif // YAML_PARSER_H