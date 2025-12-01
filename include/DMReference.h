#pragma once

#include <cstdint>
#include <string>
#include <stdexcept>

namespace DMCompiler {

/// <summary>
/// Represents a reference to a location where a value can be read from or written to.
/// References are used in bytecode to specify where operations should read/write values.
/// Ported from C# DMCompiler/Bytecode/DMReference.cs
/// </summary>
struct DMReference {
    enum class Type : uint8_t {
        /// References nothing, reads return null and writes are no-op
        NoRef,
        
        Src,
        Self,
        Usr,
        Args,
        World,
        SuperProc,
        ListIndex,
        Argument,
        Local,
        Global,
        GlobalProc,
        Field,
        SrcField,
        SrcProc,
        Callee,
        Caller,
        
        /// Something went wrong in the creation of this DMReference, and so this reference is not valid
        /// Be sure to emit a compiler error before creating
        Invalid
    };
    
    Type RefType = Type::NoRef;
    
    // Used for: Argument, Local, Global, GlobalProc, Field (as string ID), SrcField (as string ID)
    int Index = 0;
    
    // Used for: Field, SrcField, SrcProc (raw name before string table conversion)
    std::string Name;
    
    // ===== Static Factory Methods =====
    
    static DMReference Src() {
        return DMReference{Type::Src, 0, ""};
    }
    
    static DMReference Self() {
        return DMReference{Type::Self, 0, ""};
    }
    
    static DMReference Usr() {
        return DMReference{Type::Usr, 0, ""};
    }
    
    static DMReference Args() {
        return DMReference{Type::Args, 0, ""};
    }
    
    static DMReference World() {
        return DMReference{Type::World, 0, ""};
    }
    
    static DMReference SuperProc() {
        return DMReference{Type::SuperProc, 0, ""};
    }
    
    static DMReference ListIndex() {
        return DMReference{Type::ListIndex, 0, ""};
    }
    
    static DMReference Callee() {
        return DMReference{Type::Callee, 0, ""};
    }
    
    static DMReference Caller() {
        return DMReference{Type::Caller, 0, ""};
    }
    
    static DMReference Invalid() {
        return DMReference{Type::Invalid, 0, ""};
    }
    
    static DMReference CreateArgument(int argId) {
        if (argId > 255) {
            throw std::runtime_error("Argument id is greater than the maximum of 255");
        }
        return DMReference{Type::Argument, argId, ""};
    }
    
    static DMReference CreateLocal(int localId) {
        if (localId > 255) {
            throw std::runtime_error("Local variable id is greater than the maximum of 255");
        }
        return DMReference{Type::Local, localId, ""};
    }
    
    static DMReference CreateGlobal(int globalId) {
        return DMReference{Type::Global, globalId, ""};
    }
    
    static DMReference CreateField(const std::string& fieldName) {
        return DMReference{Type::Field, 0, fieldName};
    }
    
    static DMReference CreateSrcField(const std::string& fieldName) {
        return DMReference{Type::SrcField, 0, fieldName};
    }
    
    static DMReference CreateGlobalProc(int procId) {
        return DMReference{Type::GlobalProc, procId, ""};
    }
    
    static DMReference CreateSrcProc(const std::string& procName) {
        return DMReference{Type::SrcProc, 0, procName};
    }
};

} // namespace DMCompiler
