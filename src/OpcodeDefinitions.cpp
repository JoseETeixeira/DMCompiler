#include "OpcodeDefinitions.h"
#include <unordered_map>

namespace DMCompiler {

static std::unordered_map<DreamProcOpcode, OpcodeMetadata> opcodeMetadataMap;

static void InitializeOpcodeMetadata() {
    if (!opcodeMetadataMap.empty()) return;
    
    opcodeMetadataMap[DreamProcOpcode::BitShiftLeft] = OpcodeMetadata(-1);
    opcodeMetadataMap[DreamProcOpcode::PushType] = OpcodeMetadata(1, OpcodeArgType::TypeId);
    opcodeMetadataMap[DreamProcOpcode::PushString] = OpcodeMetadata(1, OpcodeArgType::String);
    opcodeMetadataMap[DreamProcOpcode::FormatString] = OpcodeMetadata(0, OpcodeArgType::String, OpcodeArgType::FormatCount);
    opcodeMetadataMap[DreamProcOpcode::SwitchCaseRange] = OpcodeMetadata(-2, OpcodeArgType::Label);
    opcodeMetadataMap[DreamProcOpcode::PushReferenceValue] = OpcodeMetadata(1, OpcodeArgType::Reference);
    opcodeMetadataMap[DreamProcOpcode::Rgb] = OpcodeMetadata(0, OpcodeArgType::ArgType, OpcodeArgType::StackDelta);
    opcodeMetadataMap[DreamProcOpcode::Add] = OpcodeMetadata(-1);
    opcodeMetadataMap[DreamProcOpcode::Assign] = OpcodeMetadata(0, OpcodeArgType::Reference);
    opcodeMetadataMap[DreamProcOpcode::Call] = OpcodeMetadata(0, OpcodeArgType::Reference, OpcodeArgType::ArgType, OpcodeArgType::StackDelta);
    opcodeMetadataMap[DreamProcOpcode::MultiplyReference] = OpcodeMetadata(0, OpcodeArgType::Reference);
    opcodeMetadataMap[DreamProcOpcode::JumpIfFalse] = OpcodeMetadata(-1, OpcodeArgType::Label);
    opcodeMetadataMap[DreamProcOpcode::CreateStrictAssociativeList] = OpcodeMetadata(0, OpcodeArgType::ListSize);
    opcodeMetadataMap[DreamProcOpcode::Jump] = OpcodeMetadata(0, OpcodeArgType::Label);
    opcodeMetadataMap[DreamProcOpcode::CompareEquals] = OpcodeMetadata(-1);
    opcodeMetadataMap[DreamProcOpcode::Return] = OpcodeMetadata(-1);
    opcodeMetadataMap[DreamProcOpcode::PushNull] = OpcodeMetadata(1);
    opcodeMetadataMap[DreamProcOpcode::Subtract] = OpcodeMetadata(-1);
    opcodeMetadataMap[DreamProcOpcode::CompareLessThan] = OpcodeMetadata(-1);
    opcodeMetadataMap[DreamProcOpcode::CompareGreaterThan] = OpcodeMetadata(-1);
    opcodeMetadataMap[DreamProcOpcode::BooleanAnd] = OpcodeMetadata(-1, OpcodeArgType::Label);
    opcodeMetadataMap[DreamProcOpcode::BooleanNot] = OpcodeMetadata(0);
    opcodeMetadataMap[DreamProcOpcode::DivideReference] = OpcodeMetadata(0, OpcodeArgType::Reference);
    opcodeMetadataMap[DreamProcOpcode::Negate] = OpcodeMetadata(0);
    opcodeMetadataMap[DreamProcOpcode::Modulus] = OpcodeMetadata(-1);
    opcodeMetadataMap[DreamProcOpcode::Append] = OpcodeMetadata(0, OpcodeArgType::Reference);
    opcodeMetadataMap[DreamProcOpcode::CreateRangeEnumerator] = OpcodeMetadata(-3, OpcodeArgType::EnumeratorId);
    opcodeMetadataMap[DreamProcOpcode::Input] = OpcodeMetadata(0, OpcodeArgType::Reference, OpcodeArgType::Reference);
    opcodeMetadataMap[DreamProcOpcode::CompareLessThanOrEqual] = OpcodeMetadata(-1);
    opcodeMetadataMap[DreamProcOpcode::CreateAssociativeList] = OpcodeMetadata(0, OpcodeArgType::ListSize);
    opcodeMetadataMap[DreamProcOpcode::Remove] = OpcodeMetadata(0, OpcodeArgType::Reference);
    opcodeMetadataMap[DreamProcOpcode::DeleteObject] = OpcodeMetadata(-1);
    opcodeMetadataMap[DreamProcOpcode::PushResource] = OpcodeMetadata(1, OpcodeArgType::Resource);
    opcodeMetadataMap[DreamProcOpcode::CreateList] = OpcodeMetadata(0, OpcodeArgType::ListSize);
    opcodeMetadataMap[DreamProcOpcode::CallStatement] = OpcodeMetadata(0, OpcodeArgType::ArgType, OpcodeArgType::StackDelta);
    opcodeMetadataMap[DreamProcOpcode::BitAnd] = OpcodeMetadata(-1);
    opcodeMetadataMap[DreamProcOpcode::CompareNotEquals] = OpcodeMetadata(-1);
    opcodeMetadataMap[DreamProcOpcode::PushProc] = OpcodeMetadata(1, OpcodeArgType::ProcId);
    opcodeMetadataMap[DreamProcOpcode::Divide] = OpcodeMetadata(-1);
    opcodeMetadataMap[DreamProcOpcode::Multiply] = OpcodeMetadata(-1);
    opcodeMetadataMap[DreamProcOpcode::BitXorReference] = OpcodeMetadata(0, OpcodeArgType::Reference);
    opcodeMetadataMap[DreamProcOpcode::BitXor] = OpcodeMetadata(-1);
    opcodeMetadataMap[DreamProcOpcode::BitOr] = OpcodeMetadata(-1);
    opcodeMetadataMap[DreamProcOpcode::BitNot] = OpcodeMetadata(0);
    opcodeMetadataMap[DreamProcOpcode::Combine] = OpcodeMetadata(0, OpcodeArgType::Reference);
    opcodeMetadataMap[DreamProcOpcode::CreateObject] = OpcodeMetadata(0, OpcodeArgType::ArgType, OpcodeArgType::StackDelta);
    opcodeMetadataMap[DreamProcOpcode::BooleanOr] = OpcodeMetadata(-1, OpcodeArgType::Label);
    opcodeMetadataMap[DreamProcOpcode::CreateMultidimensionalList] = OpcodeMetadata(0, OpcodeArgType::ListSize);
    opcodeMetadataMap[DreamProcOpcode::CompareGreaterThanOrEqual] = OpcodeMetadata(-1);
    opcodeMetadataMap[DreamProcOpcode::SwitchCase] = OpcodeMetadata(-1, OpcodeArgType::Label);
    opcodeMetadataMap[DreamProcOpcode::Mask] = OpcodeMetadata(0, OpcodeArgType::Reference);
    opcodeMetadataMap[DreamProcOpcode::Error] = OpcodeMetadata(0);
    opcodeMetadataMap[DreamProcOpcode::IsInList] = OpcodeMetadata(-1);
    opcodeMetadataMap[DreamProcOpcode::PushFloat] = OpcodeMetadata(1, OpcodeArgType::Float);
    opcodeMetadataMap[DreamProcOpcode::ModulusReference] = OpcodeMetadata(0, OpcodeArgType::Reference);
    opcodeMetadataMap[DreamProcOpcode::CreateListEnumerator] = OpcodeMetadata(-1, OpcodeArgType::EnumeratorId);
    opcodeMetadataMap[DreamProcOpcode::Enumerate] = OpcodeMetadata(0, OpcodeArgType::EnumeratorId, OpcodeArgType::Reference, OpcodeArgType::Label);
    opcodeMetadataMap[DreamProcOpcode::DestroyEnumerator] = OpcodeMetadata(0, OpcodeArgType::EnumeratorId);
    opcodeMetadataMap[DreamProcOpcode::Browse] = OpcodeMetadata(-3);
    opcodeMetadataMap[DreamProcOpcode::BrowseResource] = OpcodeMetadata(-3);
    opcodeMetadataMap[DreamProcOpcode::OutputControl] = OpcodeMetadata(-3);
    opcodeMetadataMap[DreamProcOpcode::BitShiftRight] = OpcodeMetadata(-1);
    opcodeMetadataMap[DreamProcOpcode::CreateFilteredListEnumerator] = OpcodeMetadata(-1, OpcodeArgType::EnumeratorId, OpcodeArgType::FilterId);
    opcodeMetadataMap[DreamProcOpcode::Power] = OpcodeMetadata(-1);
    opcodeMetadataMap[DreamProcOpcode::EnumerateAssoc] = OpcodeMetadata(0, OpcodeArgType::EnumeratorId, OpcodeArgType::Reference, OpcodeArgType::Reference, OpcodeArgType::Label);
    opcodeMetadataMap[DreamProcOpcode::Link] = OpcodeMetadata(-2);
    opcodeMetadataMap[DreamProcOpcode::Prompt] = OpcodeMetadata(-3, OpcodeArgType::TypeId);
    opcodeMetadataMap[DreamProcOpcode::Ftp] = OpcodeMetadata(-3);
    opcodeMetadataMap[DreamProcOpcode::Initial] = OpcodeMetadata(-1);
    opcodeMetadataMap[DreamProcOpcode::AsType] = OpcodeMetadata(-1);
    opcodeMetadataMap[DreamProcOpcode::IsType] = OpcodeMetadata(-1);
    opcodeMetadataMap[DreamProcOpcode::LocateCoord] = OpcodeMetadata(-2);
    opcodeMetadataMap[DreamProcOpcode::Locate] = OpcodeMetadata(-1);
    opcodeMetadataMap[DreamProcOpcode::IsNull] = OpcodeMetadata(0);
    opcodeMetadataMap[DreamProcOpcode::Spawn] = OpcodeMetadata(-1, OpcodeArgType::Label);
    opcodeMetadataMap[DreamProcOpcode::OutputReference] = OpcodeMetadata(-1, OpcodeArgType::Reference);
    opcodeMetadataMap[DreamProcOpcode::Output] = OpcodeMetadata(-2);
    opcodeMetadataMap[DreamProcOpcode::Pop] = OpcodeMetadata(-1);
    opcodeMetadataMap[DreamProcOpcode::Prob] = OpcodeMetadata(0);
    opcodeMetadataMap[DreamProcOpcode::IsSaved] = OpcodeMetadata(-1);
    opcodeMetadataMap[DreamProcOpcode::PickUnweighted] = OpcodeMetadata(0, OpcodeArgType::PickCount);
    opcodeMetadataMap[DreamProcOpcode::PickWeighted] = OpcodeMetadata(0, OpcodeArgType::PickCount);
    opcodeMetadataMap[DreamProcOpcode::Increment] = OpcodeMetadata(1, OpcodeArgType::Reference);
    opcodeMetadataMap[DreamProcOpcode::Decrement] = OpcodeMetadata(1, OpcodeArgType::Reference);
    opcodeMetadataMap[DreamProcOpcode::CompareEquivalent] = OpcodeMetadata(-1);
    opcodeMetadataMap[DreamProcOpcode::CompareNotEquivalent] = OpcodeMetadata(-1);
    opcodeMetadataMap[DreamProcOpcode::Throw] = OpcodeMetadata(0);
    opcodeMetadataMap[DreamProcOpcode::IsInRange] = OpcodeMetadata(-2);
    opcodeMetadataMap[DreamProcOpcode::MassConcatenation] = OpcodeMetadata(0, OpcodeArgType::ConcatCount);
    opcodeMetadataMap[DreamProcOpcode::CreateTypeEnumerator] = OpcodeMetadata(-1, OpcodeArgType::EnumeratorId);
    opcodeMetadataMap[DreamProcOpcode::PushGlobalVars] = OpcodeMetadata(1);
    opcodeMetadataMap[DreamProcOpcode::ModulusModulus] = OpcodeMetadata(-1);
    opcodeMetadataMap[DreamProcOpcode::ModulusModulusReference] = OpcodeMetadata(0, OpcodeArgType::Reference);
    opcodeMetadataMap[DreamProcOpcode::JumpIfNull] = OpcodeMetadata(0, OpcodeArgType::Label);
    opcodeMetadataMap[DreamProcOpcode::JumpIfNullNoPop] = OpcodeMetadata(0, OpcodeArgType::Label);
    opcodeMetadataMap[DreamProcOpcode::JumpIfTrueReference] = OpcodeMetadata(0, OpcodeArgType::Reference, OpcodeArgType::Label);
    opcodeMetadataMap[DreamProcOpcode::JumpIfFalseReference] = OpcodeMetadata(0, OpcodeArgType::Reference, OpcodeArgType::Label);
    opcodeMetadataMap[DreamProcOpcode::DereferenceField] = OpcodeMetadata(0, OpcodeArgType::String);
    opcodeMetadataMap[DreamProcOpcode::DereferenceIndex] = OpcodeMetadata(-1);
    opcodeMetadataMap[DreamProcOpcode::DereferenceCall] = OpcodeMetadata(0, OpcodeArgType::String, OpcodeArgType::ArgType, OpcodeArgType::StackDelta);
    opcodeMetadataMap[DreamProcOpcode::PopReference] = OpcodeMetadata(0, OpcodeArgType::Reference);
    opcodeMetadataMap[DreamProcOpcode::BitShiftLeftReference] = OpcodeMetadata(0, OpcodeArgType::Reference);
    opcodeMetadataMap[DreamProcOpcode::BitShiftRightReference] = OpcodeMetadata(0, OpcodeArgType::Reference);
    opcodeMetadataMap[DreamProcOpcode::Try] = OpcodeMetadata(0, OpcodeArgType::Label, OpcodeArgType::Reference);
    opcodeMetadataMap[DreamProcOpcode::TryNoValue] = OpcodeMetadata(0, OpcodeArgType::Label);
    opcodeMetadataMap[DreamProcOpcode::EndTry] = OpcodeMetadata(0);
    opcodeMetadataMap[DreamProcOpcode::EnumerateNoAssign] = OpcodeMetadata(0, OpcodeArgType::EnumeratorId, OpcodeArgType::Label);
    opcodeMetadataMap[DreamProcOpcode::Gradient] = OpcodeMetadata(0, OpcodeArgType::ArgType, OpcodeArgType::StackDelta);
    opcodeMetadataMap[DreamProcOpcode::AssignInto] = OpcodeMetadata(0, OpcodeArgType::Reference);
    opcodeMetadataMap[DreamProcOpcode::GetStep] = OpcodeMetadata(-1);
    opcodeMetadataMap[DreamProcOpcode::Length] = OpcodeMetadata(0);
    opcodeMetadataMap[DreamProcOpcode::GetDir] = OpcodeMetadata(-1);
    opcodeMetadataMap[DreamProcOpcode::DebuggerBreakpoint] = OpcodeMetadata(0);
    opcodeMetadataMap[DreamProcOpcode::Sin] = OpcodeMetadata(0);
    opcodeMetadataMap[DreamProcOpcode::Cos] = OpcodeMetadata(0);
    opcodeMetadataMap[DreamProcOpcode::Tan] = OpcodeMetadata(0);
    opcodeMetadataMap[DreamProcOpcode::ArcSin] = OpcodeMetadata(0);
    opcodeMetadataMap[DreamProcOpcode::ArcCos] = OpcodeMetadata(0);
    opcodeMetadataMap[DreamProcOpcode::ArcTan] = OpcodeMetadata(0);
    opcodeMetadataMap[DreamProcOpcode::ArcTan2] = OpcodeMetadata(-1);
    opcodeMetadataMap[DreamProcOpcode::Sqrt] = OpcodeMetadata(0);
    opcodeMetadataMap[DreamProcOpcode::Log] = OpcodeMetadata(-1);
    opcodeMetadataMap[DreamProcOpcode::LogE] = OpcodeMetadata(0);
    opcodeMetadataMap[DreamProcOpcode::Abs] = OpcodeMetadata(0);
    // Peephole optimization opcodes
    opcodeMetadataMap[DreamProcOpcode::AppendNoPush] = OpcodeMetadata(-1, OpcodeArgType::Reference);
    opcodeMetadataMap[DreamProcOpcode::AssignNoPush] = OpcodeMetadata(-1, OpcodeArgType::Reference);
    opcodeMetadataMap[DreamProcOpcode::PushRefAndDereferenceField] = OpcodeMetadata(1, OpcodeArgType::Reference, OpcodeArgType::String);
    opcodeMetadataMap[DreamProcOpcode::PushNRefs] = OpcodeMetadata(0, OpcodeArgType::Int); // true, 0 in C#
    opcodeMetadataMap[DreamProcOpcode::PushNFloats] = OpcodeMetadata(0, OpcodeArgType::Int);
    opcodeMetadataMap[DreamProcOpcode::PushNResources] = OpcodeMetadata(0, OpcodeArgType::Int);
    opcodeMetadataMap[DreamProcOpcode::PushStringFloat] = OpcodeMetadata(2, OpcodeArgType::String, OpcodeArgType::Float);
    opcodeMetadataMap[DreamProcOpcode::JumpIfReferenceFalse] = OpcodeMetadata(0, OpcodeArgType::Reference, OpcodeArgType::Label);
    opcodeMetadataMap[DreamProcOpcode::PushNStrings] = OpcodeMetadata(0, OpcodeArgType::Int);
    opcodeMetadataMap[DreamProcOpcode::SwitchOnFloat] = OpcodeMetadata(0, OpcodeArgType::Float, OpcodeArgType::Label);
    opcodeMetadataMap[DreamProcOpcode::PushNOfStringFloats] = OpcodeMetadata(0, OpcodeArgType::Int);
    opcodeMetadataMap[DreamProcOpcode::CreateListNFloats] = OpcodeMetadata(1, OpcodeArgType::Int);
    opcodeMetadataMap[DreamProcOpcode::CreateListNStrings] = OpcodeMetadata(1, OpcodeArgType::Int);
    opcodeMetadataMap[DreamProcOpcode::CreateListNRefs] = OpcodeMetadata(1, OpcodeArgType::Int);
    opcodeMetadataMap[DreamProcOpcode::CreateListNResources] = OpcodeMetadata(1, OpcodeArgType::Int);
    opcodeMetadataMap[DreamProcOpcode::SwitchOnString] = OpcodeMetadata(0, OpcodeArgType::String, OpcodeArgType::Label);
    opcodeMetadataMap[DreamProcOpcode::IsTypeDirect] = OpcodeMetadata(0, OpcodeArgType::TypeId);
    opcodeMetadataMap[DreamProcOpcode::NullRef] = OpcodeMetadata(0, OpcodeArgType::Reference);
    opcodeMetadataMap[DreamProcOpcode::ReturnReferenceValue] = OpcodeMetadata(0, OpcodeArgType::Reference);
    opcodeMetadataMap[DreamProcOpcode::ReturnFloat] = OpcodeMetadata(0, OpcodeArgType::Float);
    opcodeMetadataMap[DreamProcOpcode::IndexRefWithString] = OpcodeMetadata(1, OpcodeArgType::Reference, OpcodeArgType::String);
    opcodeMetadataMap[DreamProcOpcode::PushFloatAssign] = OpcodeMetadata(2, OpcodeArgType::Float, OpcodeArgType::Reference);
    opcodeMetadataMap[DreamProcOpcode::NPushFloatAssign] = OpcodeMetadata(0, OpcodeArgType::Int);
}

const OpcodeMetadata& GetOpcodeMetadata(DreamProcOpcode opcode) {
    InitializeOpcodeMetadata();
    
    auto it = opcodeMetadataMap.find(opcode);
    if (it != opcodeMetadataMap.end()) {
        return it->second;
    }
    
    static OpcodeMetadata defaultMetadata;
    return defaultMetadata;
}

} // namespace DMCompiler
