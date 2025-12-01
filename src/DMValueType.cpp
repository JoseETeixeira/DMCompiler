#include "DMValueType.h"
#include <sstream>
#include <algorithm>
#include <cctype>

namespace DMCompiler {

/// Parse type flags from a string (e.g., "num", "text", "num|text")
/// Returns DMValueType::Anything if the string is empty or invalid
DMValueType ParseTypeFlags(const std::string& typeStr) {
    if (typeStr.empty()) {
        return DMValueType::Anything;
    }
    
    DMValueType result = DMValueType::Anything;
    bool hasAnyFlag = false;
    
    // Split by '|' to handle multiple types
    std::string current;
    for (size_t i = 0; i <= typeStr.length(); ++i) {
        if (i == typeStr.length() || typeStr[i] == '|') {
            if (!current.empty()) {
                // Trim whitespace and convert to lowercase
                std::string trimmed = current;
                trimmed.erase(0, trimmed.find_first_not_of(" \t\r\n"));
                trimmed.erase(trimmed.find_last_not_of(" \t\r\n") + 1);
                
                std::string lower = trimmed;
                std::transform(lower.begin(), lower.end(), lower.begin(),
                             [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                
                // Map type name to DMValueType flag
                DMValueType flag = DMValueType::Anything;
                if (lower == "null") {
                    flag = DMValueType::Null;
                } else if (lower == "text") {
                    flag = DMValueType::Text;
                } else if (lower == "obj") {
                    flag = DMValueType::Obj;
                } else if (lower == "mob") {
                    flag = DMValueType::Mob;
                } else if (lower == "turf") {
                    flag = DMValueType::Turf;
                } else if (lower == "num") {
                    flag = DMValueType::Num;
                } else if (lower == "message") {
                    flag = DMValueType::Message;
                } else if (lower == "area") {
                    flag = DMValueType::Area;
                } else if (lower == "color") {
                    flag = DMValueType::Color;
                } else if (lower == "file") {
                    flag = DMValueType::File;
                } else if (lower == "command_text") {
                    flag = DMValueType::CommandText;
                } else if (lower == "sound") {
                    flag = DMValueType::Sound;
                } else if (lower == "icon") {
                    flag = DMValueType::Icon;
                } else if (lower == "path") {
                    flag = DMValueType::Path;
                } else if (lower == "anything") {
                    // Explicit "anything" - keep as Anything
                    flag = DMValueType::Anything;
                }
                
                // Combine flags with bitwise OR
                if (static_cast<uint32_t>(flag) != 0 || lower == "anything") {
                    if (!hasAnyFlag) {
                        result = flag;
                        hasAnyFlag = true;
                    } else {
                        result = result | flag;
                    }
                }
                
                current.clear();
            }
        } else {
            current += typeStr[i];
        }
    }
    
    return hasAnyFlag ? result : DMValueType::Anything;
}

std::string DMValueTypeToString(DMValueType type) {
    std::ostringstream ss;
    bool first = true;
    
    auto addFlag = [&](DMValueType flag, const char* name) {
        if (HasFlag(type, flag)) {
            if (!first) ss << ", ";
            ss << name;
            first = false;
        }
    };
    
    // Check if it's just Anything (no flags set)
    if (static_cast<uint32_t>(type) == 0) {
        return "anything";
    }
    
    // Add each flag that's set
    addFlag(DMValueType::Null, "null");
    addFlag(DMValueType::Text, "text");
    addFlag(DMValueType::Obj, "obj");
    addFlag(DMValueType::Mob, "mob");
    addFlag(DMValueType::Turf, "turf");
    addFlag(DMValueType::Num, "num");
    addFlag(DMValueType::Message, "message");
    addFlag(DMValueType::Area, "area");
    addFlag(DMValueType::Color, "color");
    addFlag(DMValueType::File, "file");
    addFlag(DMValueType::CommandText, "command_text");
    addFlag(DMValueType::Sound, "sound");
    addFlag(DMValueType::Icon, "icon");
    addFlag(DMValueType::Path, "path");
    
    // Special flags (usually not printed)
    addFlag(DMValueType::Unimplemented, "unimplemented");
    addFlag(DMValueType::CompiletimeReadonly, "compiletime_readonly");
    addFlag(DMValueType::NoConstFold, "no_const_fold");
    addFlag(DMValueType::Unsupported, "unsupported");
    
    return ss.str();
}

} // namespace DMCompiler
