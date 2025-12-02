#pragma once

#include <cstdint>
#include "DreamProcOpcode.h"

namespace DMCompiler {

/// <summary>
/// Opcode argument types
/// </summary>
enum class OpcodeArgType {
    None,
    TypeId,
    String,
    Label,
    Reference,
    StackDelta,
    ArgType,
    FormatCount,
    ListSize,
    Resource,
    ProcId,
    EnumeratorId,
    FilterId,
    PickCount,
    ConcatCount,
    Int,
    Float
};

/// <summary>
/// Metadata about each opcode
/// </summary>
struct OpcodeMetadata {
    int StackDelta;  // Change in stack size (-1 for pops, +1 for pushes, etc.)
    OpcodeArgType ArgType1;
    OpcodeArgType ArgType2;
    OpcodeArgType ArgType3;
    OpcodeArgType ArgType4;

    OpcodeMetadata()
        : StackDelta(0)
        , ArgType1(OpcodeArgType::None)
        , ArgType2(OpcodeArgType::None)
        , ArgType3(OpcodeArgType::None)
        , ArgType4(OpcodeArgType::None)
    {}

    OpcodeMetadata(int delta, OpcodeArgType arg1 = OpcodeArgType::None,
                   OpcodeArgType arg2 = OpcodeArgType::None,
                   OpcodeArgType arg3 = OpcodeArgType::None,
                   OpcodeArgType arg4 = OpcodeArgType::None)
        : StackDelta(delta)
        , ArgType1(arg1)
        , ArgType2(arg2)
        , ArgType3(arg3)
        , ArgType4(arg4)
    {}
};

// Get metadata for an opcode
const OpcodeMetadata& GetOpcodeMetadata(DreamProcOpcode opcode);

} // namespace DMCompiler
