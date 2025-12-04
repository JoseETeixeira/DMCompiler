#pragma once

#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <variant>

namespace DMCompiler {

// Resource reference with ID (serialized as {"type": "resource", "id": N})
struct ResourceRef {
    int id;
    explicit ResourceRef(int id) : id(id) {}
};

// JsonValue type for representing JSON values from expressions
using JsonValue = std::variant<
    std::nullptr_t,
    bool,
    int64_t,
    double,
    std::string,
    std::unordered_map<std::string, std::string>,
    ResourceRef
>;

/// Simple JSON writer helper for serializing compiler output
class JsonWriter {
public:
    JsonWriter() : indent_(0) {}
    
    void BeginObject();
    void EndObject();
    void BeginArray();
    void EndArray();
    
    void WriteKey(const std::string& key);
    void WriteString(const std::string& value);
    void WriteInt(int value);
    void WriteInt64(int64_t value);
    void WriteDouble(double value);
    void WriteBool(bool value);
    void WriteNull();
    void WriteValue(const JsonValue& value);
    
    void WriteKeyValue(const std::string& key, const std::string& value);
    void WriteKeyValue(const std::string& key, int value);
    void WriteKeyValue(const std::string& key, bool value);
    
    void WriteByteArray(const std::vector<uint8_t>& bytes);
    
    std::string ToString() const { return ss_.str(); }
    
private:
    std::stringstream ss_;
    int indent_;
    bool needsComma_ = false;
    
    void WriteIndent();
    void WriteComma();
    std::string EscapeString(const std::string& str);
};

} // namespace DMCompiler