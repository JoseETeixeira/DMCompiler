#include "JsonWriter.h"
#include <iomanip>

namespace DMCompiler {

void JsonWriter::BeginObject() {
    WriteComma();
    ss_ << "{\n";
    indent_++;
    needsComma_ = false;
}

void JsonWriter::EndObject() {
    ss_ << "\n";
    indent_--;
    WriteIndent();
    ss_ << "}";
    needsComma_ = true;
}

void JsonWriter::BeginArray() {
    WriteComma();
    ss_ << "[\n";
    indent_++;
    needsComma_ = false;
}

void JsonWriter::EndArray() {
    ss_ << "\n";
    indent_--;
    WriteIndent();
    ss_ << "]";
    needsComma_ = true;
}

void JsonWriter::WriteKey(const std::string& key) {
    WriteComma();
    WriteIndent();
    ss_ << "\"" << EscapeString(key) << "\": ";
    needsComma_ = false;
}

void JsonWriter::WriteString(const std::string& value) {
    WriteComma();
    ss_ << "\"" << EscapeString(value) << "\"";
    needsComma_ = true;
}

void JsonWriter::WriteInt(int value) {
    WriteComma();
    ss_ << value;
    needsComma_ = true;
}

void JsonWriter::WriteBool(bool value) {
    WriteComma();
    ss_ << (value ? "true" : "false");
    needsComma_ = true;
}

void JsonWriter::WriteNull() {
    WriteComma();
    ss_ << "null";
    needsComma_ = true;
}

void JsonWriter::WriteInt64(int64_t value) {
    WriteComma();
    ss_ << value;
    needsComma_ = true;
}

void JsonWriter::WriteDouble(double value) {
    WriteComma();
    ss_ << std::fixed << std::setprecision(6) << value;
    needsComma_ = true;
}

void JsonWriter::WriteValue(const JsonValue& value) {
    std::visit([this](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, std::nullptr_t>) {
            WriteNull();
        } else if constexpr (std::is_same_v<T, bool>) {
            WriteBool(arg);
        } else if constexpr (std::is_same_v<T, int64_t>) {
            WriteInt64(arg);
        } else if constexpr (std::is_same_v<T, double>) {
            WriteDouble(arg);
        } else if constexpr (std::is_same_v<T, std::string>) {
            WriteString(arg);
        } else if constexpr (std::is_same_v<T, std::unordered_map<std::string, std::string>>) {
            // Handle special map case (e.g., {"type": "float_infinity"})
            BeginObject();
            for (const auto& [key, val] : arg) {
                WriteKeyValue(key, val);
            }
            EndObject();
        } else if constexpr (std::is_same_v<T, ResourceRef>) {
            // Handle resource references: {"type": "resource", "id": N}
            BeginObject();
            WriteKeyValue("type", std::string("resource"));
            WriteKeyValue("id", arg.id);
            EndObject();
        }
    }, value);
}

void JsonWriter::WriteKeyValue(const std::string& key, const std::string& value) {
    WriteKey(key);
    WriteString(value);
}

void JsonWriter::WriteKeyValue(const std::string& key, int value) {
    WriteKey(key);
    WriteInt(value);
}

void JsonWriter::WriteKeyValue(const std::string& key, bool value) {
    WriteKey(key);
    WriteBool(value);
}

void JsonWriter::WriteByteArray(const std::vector<uint8_t>& bytes) {
    WriteComma();
    ss_ << "[";
    for (size_t i = 0; i < bytes.size(); ++i) {
        if (i > 0) ss_ << ",";
        ss_ << static_cast<int>(bytes[i]);
    }
    ss_ << "]";
    needsComma_ = true;
}

void JsonWriter::WriteIndent() {
    for (int i = 0; i < indent_; ++i) {
        ss_ << "  ";
    }
}

void JsonWriter::WriteComma() {
    if (needsComma_) {
        ss_ << ",\n";
    }
}

std::string JsonWriter::EscapeString(const std::string& str) {
    std::string result;
    result.reserve(str.size());
    
    for (char c : str) {
        switch (c) {
            case '"':  result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\b': result += "\\b"; break;
            case '\f': result += "\\f"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default:
                if (c < 32) {
                    // Escape control characters
                    char buf[7];
                    snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
                    result += buf;
                } else {
                    result += c;
                }
                break;
        }
    }
    
    return result;
}

} // namespace DMCompiler