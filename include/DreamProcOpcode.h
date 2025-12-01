#pragma once

#include <cstdint>

namespace DMCompiler {

/// <summary>
/// DM procedure bytecode operation codes.
/// These opcodes form the instruction set for compiled DM procedures.
/// Ported from C# DMCompiler/Bytecode/DreamProcOpcode.cs
/// </summary>
enum class DreamProcOpcode : uint8_t {
    // Arithmetic and bitwise operations
    BitShiftLeft = 0x01,
    Add = 0x08,
    Subtract = 0x12,
    Multiply = 0x28,
    Divide = 0x27,
    Modulus = 0x19,
    Power = 0x42,
    Negate = 0x18,
    
    // Bitwise operations
    BitAnd = 0x24,
    BitOr = 0x2B,
    BitXor = 0x2A,
    BitNot = 0x2C,
    BitShiftRight = 0x40,
    
    // Comparison operations
    CompareEquals = 0x0F,
    CompareNotEquals = 0x25,
    CompareLessThan = 0x13,
    CompareGreaterThan = 0x14,
    CompareLessThanOrEqual = 0x1D,
    CompareGreaterThanOrEqual = 0x31,
    CompareEquivalent = 0x58,
    CompareNotEquivalent = 0x59,
    
    // Logical operations
    BooleanAnd = 0x15,
    BooleanOr = 0x2F,
    BooleanNot = 0x16,
    
    // Stack operations
    PushType = 0x02,
    PushString = 0x03,
    PushFloat = 0x38,
    PushNull = 0x11,
    PushProc = 0x26,
    PushResource = 0x21,
    PushGlobalVars = 0x5F,
    PushReferenceValue = 0x06,
    Pop = 0x51,
    
    // Assignment operations
    Assign = 0x09,
    AssignInto = 0x74,
    AssignNoPush = 0x85,  // Peephole optimization
    
    // Reference operations
    MultiplyReference = 0x0B,
    DivideReference = 0x17,
    ModulusReference = 0x39,
    BitXorReference = 0x29,
    ModulusModulusReference = 0x61,
    BitShiftLeftReference = 0x6D,
    BitShiftRightReference = 0x6E,
    PopReference = 0x6B,
    NullRef = 0x96,
    ReturnReferenceValue = 0x97,
    
    // Control flow
    Jump = 0x0E,
    JumpIfFalse = 0x0C,
    JumpIfNull = 0x64,
    JumpIfNullNoPop = 0x65,
    JumpIfTrueReference = 0x66,
    JumpIfFalseReference = 0x67,
    JumpIfReferenceFalse = 0x8B,
    Return = 0x10,
    ReturnFloat = 0x98,
    
    // Procedure calls
    Call = 0x0A,
    CallStatement = 0x23,
    DereferenceCall = 0x6A,
    
    // List operations
    CreateList = 0x22,
    CreateAssociativeList = 0x1E,
    CreateStrictAssociativeList = 0x0D,
    CreateMultidimensionalList = 0x30,
    Append = 0x1A,
    AppendNoPush = 0x84,  // Peephole optimization
    Remove = 0x1F,
    Combine = 0x2D,
    Mask = 0x33,
    IsInList = 0x36,
    
    // Object operations
    CreateObject = 0x2E,
    DeleteObject = 0x20,
    
    // Enumerators
    CreateListEnumerator = 0x3A,
    CreateFilteredListEnumerator = 0x41,
    CreateRangeEnumerator = 0x1B,
    CreateTypeEnumerator = 0x5D,
    Enumerate = 0x3B,
    EnumerateAssoc = 0x43,
    EnumerateNoAssign = 0x72,
    DestroyEnumerator = 0x3C,
    
    // Field/Index operations
    DereferenceField = 0x68,
    DereferenceIndex = 0x69,
    IndexRefWithString = 0x99,
    PushRefAndDereferenceField = 0x86,  // Peephole optimization
    
    // Switch statement
    SwitchCase = 0x32,
    SwitchCaseRange = 0x05,
    SwitchOnFloat = 0x8D,
    SwitchOnString = 0x93,
    
    // String operations
    FormatString = 0x04,
    MassConcatenation = 0x5C,
    
    // Type operations
    AsType = 0x48,
    IsType = 0x49,
    IsTypeDirect = 0x95,
    IsNull = 0x4C,
    
    // Increment/Decrement
    Increment = 0x56,
    Decrement = 0x57,
    
    // Special operations
    Input = 0x1C,
    Output = 0x4F,
    OutputReference = 0x4E,
    OutputControl = 0x3F,
    Browse = 0x3D,
    BrowseResource = 0x3E,
    Link = 0x44,
    Ftp = 0x46,
    Spawn = 0x4D,
    Error = 0x35,
    Throw = 0x5A,
    
    // Exception handling
    Try = 0x6F,
    TryNoValue = 0x70,
    EndTry = 0x71,
    
    // Pick operations
    PickWeighted = 0x55,
    PickUnweighted = 0x54,
    Prob = 0x52,
    
    // Math operations
    Rgb = 0x07,
    Gradient = 0x73,
    Sin = 0x79,
    Cos = 0x7A,
    Tan = 0x7B,
    ArcSin = 0x7C,
    ArcCos = 0x7D,
    ArcTan = 0x7E,
    ArcTan2 = 0x7F,
    Sqrt = 0x80,
    Log = 0x81,
    LogE = 0x82,
    Abs = 0x83,
    
    // World operations
    Locate = 0x4B,
    LocateCoord = 0x4A,
    GetStep = 0x75,
    GetDir = 0x77,
    Length = 0x76,
    
    // Misc operations
    Initial = 0x47,
    IsSaved = 0x53,
    IsInRange = 0x5B,
    ModulusModulus = 0x60,
    Prompt = 0x45,
    
    // Debugging
    DebuggerBreakpoint = 0x78,
    
    // Peephole optimizations (batch operations)
    PushNRefs = 0x87,
    PushNFloats = 0x88,
    PushNResources = 0x89,
    PushStringFloat = 0x8A,
    PushNStrings = 0x8C,
    PushNOfStringFloats = 0x8E,
    CreateListNFloats = 0x8F,
    CreateListNStrings = 0x90,
    CreateListNRefs = 0x91,
    CreateListNResources = 0x92,
    NPushFloatAssign = 0x9B,
    PushFloatAssign = 0x9A
};

/// <summary>
/// Specifies where procedure call arguments come from.
/// </summary>
enum class DMCallArgumentsType : uint8_t {
    /// <summary>
    /// No arguments provided.
    /// </summary>
    None = 0,
    
    /// <summary>
    /// Arguments are on the stack.
    /// </summary>
    FromStack = 1,
    
    /// <summary>
    /// Arguments are on the stack with associated keys (named arguments).
    /// </summary>
    FromStackKeyed = 2,
    
    /// <summary>
    /// Arguments come from a list on the stack (arglist()).
    /// </summary>
    FromArgumentList = 3,
    
    /// <summary>
    /// Use the arguments from the calling procedure (implicit ..()).
    /// </summary>
    FromProcArguments = 4
};

} // namespace DMCompiler
