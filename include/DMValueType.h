#pragma once

#include <cstdint>
#include <string>

namespace DMCompiler {

/// <summary>
/// DMValueType - Type Flags for DM Values
/// 
/// Stores explicit casting done via the "as" keyword. Also stores compiler hints for DMStandard.
/// This is a flags enum because it's possible for something to have multiple values.
/// 
/// IMPORTANT: This must be kept in sync with OpenDreamShared.Dream.DreamValueType
/// </summary>
enum class DMValueType : uint32_t {
    Anything = 0x0,
    Null = 0x1,
    Text = 0x2,
    Obj = 0x4,
    Mob = 0x8,
    Turf = 0x10,
    Num = 0x20,
    Message = 0x40,
    Area = 0x80,
    Color = 0x100,
    File = 0x200,
    CommandText = 0x400,
    Sound = 0x800,
    Icon = 0x1000,
    Path = 0x2000, // For proc return types

    // Special flags
    Unimplemented = 0x4000,        // Marks that a method or property is not implemented
    CompiletimeReadonly = 0x8000,  // Marks that a property can only ever be read from
    NoConstFold = 0x10000,         // Marks that a const var cannot be const-folded
    Unsupported = 0x20000,         // Marks that a method or property will not be implemented
};

// Bitwise operators for DMValueType flags
inline DMValueType operator|(DMValueType a, DMValueType b) {
    return static_cast<DMValueType>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline DMValueType operator&(DMValueType a, DMValueType b) {
    return static_cast<DMValueType>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline DMValueType operator~(DMValueType a) {
    return static_cast<DMValueType>(~static_cast<uint32_t>(a));
}

inline DMValueType& operator|=(DMValueType& a, DMValueType b) {
    a = a | b;
    return a;
}

inline bool HasFlag(DMValueType value, DMValueType flag) {
    return (static_cast<uint32_t>(value) & static_cast<uint32_t>(flag)) != 0;
}

/// Convert DMValueType to string representation
std::string DMValueTypeToString(DMValueType type);

/// Parse type flags from a string (e.g., "num", "text", "num|text")
/// Returns DMValueType::Anything if the string is empty or invalid
DMValueType ParseTypeFlags(const std::string& typeStr);

} // namespace DMCompiler
