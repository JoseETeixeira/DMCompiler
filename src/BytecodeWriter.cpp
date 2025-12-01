#include "BytecodeWriter.h"
#include <cstring>
#include <stdexcept>
#include <iostream>

namespace DMCompiler {

BytecodeWriter::BytecodeWriter()
    : NextLabelId_(0)
    , CurrentStackSize_(0)
    , MaxStackSize_(0) {
}

void BytecodeWriter::Emit(DreamProcOpcode opcode) {
    WriteByte(static_cast<uint8_t>(opcode));
}

void BytecodeWriter::EmitByte(DreamProcOpcode opcode, uint8_t value) {
    WriteByte(static_cast<uint8_t>(opcode));
    WriteByte(value);
}

void BytecodeWriter::EmitShort(DreamProcOpcode opcode, uint16_t value) {
    WriteByte(static_cast<uint8_t>(opcode));
    WriteShort(value);
}

void BytecodeWriter::EmitInt(DreamProcOpcode opcode, int32_t value) {
    WriteByte(static_cast<uint8_t>(opcode));
    WriteInt(value);
}

void BytecodeWriter::EmitFloat(DreamProcOpcode opcode, float value) {
    WriteByte(static_cast<uint8_t>(opcode));
    WriteFloat(value);
}

void BytecodeWriter::EmitString(DreamProcOpcode opcode, const std::string& value) {
    WriteByte(static_cast<uint8_t>(opcode));
    int stringId = GetStringId(value);
    WriteInt(stringId);
}

void BytecodeWriter::EmitMulti(DreamProcOpcode opcode, const std::vector<uint8_t>& operands) {
    WriteByte(static_cast<uint8_t>(opcode));
    for (uint8_t operand : operands) {
        WriteByte(operand);
    }
}

int BytecodeWriter::CreateLabel() {
    return NextLabelId_++;
}

void BytecodeWriter::MarkLabel(int labelId) {
    LabelPositions_[labelId] = Bytecode_.size();
}

void BytecodeWriter::EmitJump(DreamProcOpcode opcode, int labelId) {
    WriteByte(static_cast<uint8_t>(opcode));
    
    // Reserve space for the jump offset (4 bytes for int32)
    size_t jumpPosition = Bytecode_.size();
    WriteInt(0);  // Placeholder, will be fixed up in Finalize()
    
    // Record this jump for later fixup
    PendingJumps_.push_back({jumpPosition, labelId, opcode});
}

void BytecodeWriter::EmitJumpWithReference(DreamProcOpcode opcode, const std::vector<uint8_t>& refBytes, int labelId) {
    WriteByte(static_cast<uint8_t>(opcode));
    
    // Emit reference bytes
    for (uint8_t byte : refBytes) {
        WriteByte(byte);
    }
    
    // Reserve space for the jump offset (4 bytes for int32)
    size_t jumpPosition = Bytecode_.size();
    WriteInt(0);  // Placeholder, will be fixed up in Finalize()
    
    // Record this jump for later fixup
    PendingJumps_.push_back({jumpPosition, labelId, opcode});
}

void BytecodeWriter::Finalize() {
    // Fix up all pending jumps
    for (const auto& jump : PendingJumps_) {
        auto labelIt = LabelPositions_.find(jump.TargetLabel);
        if (labelIt == LabelPositions_.end()) {
            throw std::runtime_error("Jump to undefined label " + std::to_string(jump.TargetLabel));
        }
        
        // Calculate the offset from the jump instruction to the label
        // The offset is relative to the position AFTER the jump instruction
        int32_t offset = static_cast<int32_t>(labelIt->second - (jump.BytecodePosition + 4));
        
        // Write the offset at the jump position
        Bytecode_[jump.BytecodePosition] = offset & 0xFF;
        Bytecode_[jump.BytecodePosition + 1] = (offset >> 8) & 0xFF;
        Bytecode_[jump.BytecodePosition + 2] = (offset >> 16) & 0xFF;
        Bytecode_[jump.BytecodePosition + 3] = (offset >> 24) & 0xFF;
    }
    
    // Clear pending jumps after fixup
    PendingJumps_.clear();
}

int BytecodeWriter::GetStringId(const std::string& str) {
    auto it = StringTable_.find(str);
    if (it != StringTable_.end()) {
        return it->second;
    }
    
    // Add new string to table
    int id = static_cast<int>(Strings_.size());
    StringTable_[str] = id;
    Strings_.push_back(str);
    return id;
}

void BytecodeWriter::Reset() {
    Bytecode_.clear();
    StringTable_.clear();
    Strings_.clear();
    LabelPositions_.clear();
    PendingJumps_.clear();
    NextLabelId_ = 0;
    CurrentStackSize_ = 0;
    MaxStackSize_ = 0;
}

void BytecodeWriter::ResizeStack(int sizeDelta) {
    CurrentStackSize_ += sizeDelta;
    if (CurrentStackSize_ > MaxStackSize_) {
        MaxStackSize_ = CurrentStackSize_;
    }
    
    // Detect stack underflow (debugging aid)
    if (CurrentStackSize_ < 0) {
        std::cerr << "Warning: Stack underflow detected at bytecode position " 
                  << Bytecode_.size() << " (stack size: " << CurrentStackSize_ << ")" << std::endl;
        // Don't throw - allow compilation to continue for better error reporting
        // Reset to 0 to prevent cascading errors
        CurrentStackSize_ = 0;
    }
}

void BytecodeWriter::WriteByte(uint8_t value) {
    Bytecode_.push_back(value);
}

void BytecodeWriter::WriteShort(uint16_t value) {
    // Little-endian encoding
    WriteByte(value & 0xFF);
    WriteByte((value >> 8) & 0xFF);
}

void BytecodeWriter::WriteInt(int32_t value) {
    // Little-endian encoding
    WriteByte(value & 0xFF);
    WriteByte((value >> 8) & 0xFF);
    WriteByte((value >> 16) & 0xFF);
    WriteByte((value >> 24) & 0xFF);
}

void BytecodeWriter::WriteFloat(float value) {
    // Write float in IEEE 754 format
    uint32_t bits;
    std::memcpy(&bits, &value, sizeof(float));
    WriteInt(static_cast<int32_t>(bits));
}

void BytecodeWriter::AppendByte(uint8_t value) {
    WriteByte(value);
}

void BytecodeWriter::AppendInt(int32_t value) {
    WriteInt(value);
}

void BytecodeWriter::EmitGlobalProcReference(int procId) {
    // DMReference.Type.GlobalProc = 11 (0x0B)
    WriteByte(11);
    WriteInt(procId);
}

void BytecodeWriter::WriteReference(const DMReference& reference) {
    // Write the reference type byte
    WriteByte(static_cast<uint8_t>(reference.RefType));
    
    // Write additional data based on reference type
    switch (reference.RefType) {
        case DMReference::Type::Argument:
        case DMReference::Type::Local:
            // Single byte for argument/local index (0-255)
            WriteByte(static_cast<uint8_t>(reference.Index));
            break;
            
        case DMReference::Type::Global:
        case DMReference::Type::GlobalProc:
            // 32-bit int for global variable/proc ID
            WriteInt(reference.Index);
            break;
            
        case DMReference::Type::Field:
        case DMReference::Type::SrcField:
        case DMReference::Type::SrcProc:
            // 32-bit int for string table index
            // For now, use Index directly (assumes caller has already converted to string ID)
            WriteInt(reference.Index);
            break;
            
        case DMReference::Type::ListIndex:
        case DMReference::Type::SuperProc:
        case DMReference::Type::Src:
        case DMReference::Type::Self:
        case DMReference::Type::Args:
        case DMReference::Type::World:
        case DMReference::Type::Usr:
        case DMReference::Type::Callee:
        case DMReference::Type::Caller:
        case DMReference::Type::NoRef:
        case DMReference::Type::Invalid:
            // No additional data needed
            break;
    }
}

void BytecodeWriter::CreateListEnumerator(int enumeratorId) {
    Emit(DreamProcOpcode::CreateListEnumerator);
    WriteInt(enumeratorId);
}

void BytecodeWriter::CreateFilteredListEnumerator(int enumeratorId, int filterTypeId) {
    Emit(DreamProcOpcode::CreateFilteredListEnumerator);
    WriteInt(enumeratorId);
    WriteInt(filterTypeId);
    // TODO: Also need to write DreamPath for the filter type
}

void BytecodeWriter::Enumerate(int enumeratorId, const DMReference& reference, int endLabel) {
    Emit(DreamProcOpcode::Enumerate);
    WriteInt(enumeratorId);
    WriteReference(reference);
    // Write a placeholder for the jump offset, will be fixed up in Finalize
    size_t jumpPos = Bytecode_.size();
    WriteInt(0); // Placeholder for jump offset
    PendingJumps_.push_back({jumpPos, endLabel, DreamProcOpcode::Enumerate});
}

void BytecodeWriter::EnumerateAssoc(int enumeratorId, const DMReference& assocRef, const DMReference& outputRef, int endLabel) {
    Emit(DreamProcOpcode::EnumerateAssoc);
    WriteInt(enumeratorId);
    WriteReference(assocRef);
    WriteReference(outputRef);
    // Write a placeholder for the jump offset
    size_t jumpPos = Bytecode_.size();
    WriteInt(0);
    PendingJumps_.push_back({jumpPos, endLabel, DreamProcOpcode::EnumerateAssoc});
}

void BytecodeWriter::EnumerateNoAssign(int enumeratorId, int endLabel) {
    Emit(DreamProcOpcode::EnumerateNoAssign);
    WriteInt(enumeratorId);
    // Write a placeholder for the jump offset
    size_t jumpPos = Bytecode_.size();
    WriteInt(0);
    PendingJumps_.push_back({jumpPos, endLabel, DreamProcOpcode::EnumerateNoAssign});
}

void BytecodeWriter::DestroyEnumerator(int enumeratorId) {
    Emit(DreamProcOpcode::DestroyEnumerator);
    WriteInt(enumeratorId);
}


} // namespace DMCompiler
