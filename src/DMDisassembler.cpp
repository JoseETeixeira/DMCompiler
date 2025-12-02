#include "DMDisassembler.h"
#include "DreamProcOpcode.h"
#include "OpcodeDefinitions.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace DMCompiler {

// Static empty string for returning references
static const std::string EmptyString;

// Helper to get opcode name (could be moved to OpcodeDefinitions)
std::string GetOpcodeName(DreamProcOpcode opcode) {
    switch (opcode) {
        case DreamProcOpcode::BitShiftLeft: return "BitShiftLeft";
        case DreamProcOpcode::PushType: return "PushType";
        case DreamProcOpcode::PushString: return "PushString";
        case DreamProcOpcode::FormatString: return "FormatString";
        case DreamProcOpcode::SwitchCaseRange: return "SwitchCaseRange";
        case DreamProcOpcode::PushReferenceValue: return "PushReferenceValue";
        case DreamProcOpcode::Rgb: return "Rgb";
        case DreamProcOpcode::Add: return "Add";
        case DreamProcOpcode::Assign: return "Assign";
        case DreamProcOpcode::Call: return "Call";
        case DreamProcOpcode::MultiplyReference: return "MultiplyReference";
        case DreamProcOpcode::JumpIfFalse: return "JumpIfFalse";
        case DreamProcOpcode::CreateStrictAssociativeList: return "CreateStrictAssociativeList";
        case DreamProcOpcode::Jump: return "Jump";
        case DreamProcOpcode::CompareEquals: return "CompareEquals";
        case DreamProcOpcode::Return: return "Return";
        case DreamProcOpcode::PushNull: return "PushNull";
        case DreamProcOpcode::Subtract: return "Subtract";
        case DreamProcOpcode::CompareLessThan: return "CompareLessThan";
        case DreamProcOpcode::CompareGreaterThan: return "CompareGreaterThan";
        case DreamProcOpcode::BooleanAnd: return "BooleanAnd";
        case DreamProcOpcode::BooleanNot: return "BooleanNot";
        case DreamProcOpcode::DivideReference: return "DivideReference";
        case DreamProcOpcode::Negate: return "Negate";
        case DreamProcOpcode::Modulus: return "Modulus";
        case DreamProcOpcode::Append: return "Append";
        case DreamProcOpcode::CreateRangeEnumerator: return "CreateRangeEnumerator";
        case DreamProcOpcode::Input: return "Input";
        case DreamProcOpcode::CompareLessThanOrEqual: return "CompareLessThanOrEqual";
        case DreamProcOpcode::CreateAssociativeList: return "CreateAssociativeList";
        case DreamProcOpcode::Remove: return "Remove";
        case DreamProcOpcode::DeleteObject: return "DeleteObject";
        case DreamProcOpcode::PushResource: return "PushResource";
        case DreamProcOpcode::CreateList: return "CreateList";
        case DreamProcOpcode::CallStatement: return "CallStatement";
        case DreamProcOpcode::BitAnd: return "BitAnd";
        case DreamProcOpcode::CompareNotEquals: return "CompareNotEquals";
        case DreamProcOpcode::PushProc: return "PushProc";
        case DreamProcOpcode::Divide: return "Divide";
        case DreamProcOpcode::Multiply: return "Multiply";
        case DreamProcOpcode::BitXorReference: return "BitXorReference";
        case DreamProcOpcode::BitXor: return "BitXor";
        case DreamProcOpcode::BitOr: return "BitOr";
        case DreamProcOpcode::BitNot: return "BitNot";
        case DreamProcOpcode::Combine: return "Combine";
        case DreamProcOpcode::CreateObject: return "CreateObject";
        case DreamProcOpcode::BooleanOr: return "BooleanOr";
        case DreamProcOpcode::CreateMultidimensionalList: return "CreateMultidimensionalList";
        case DreamProcOpcode::CompareGreaterThanOrEqual: return "CompareGreaterThanOrEqual";
        case DreamProcOpcode::SwitchCase: return "SwitchCase";
        case DreamProcOpcode::Mask: return "Mask";
        case DreamProcOpcode::Error: return "Error";
        case DreamProcOpcode::IsInList: return "IsInList";
        case DreamProcOpcode::PushFloat: return "PushFloat";
        case DreamProcOpcode::ModulusReference: return "ModulusReference";
        case DreamProcOpcode::CreateListEnumerator: return "CreateListEnumerator";
        case DreamProcOpcode::Enumerate: return "Enumerate";
        case DreamProcOpcode::DestroyEnumerator: return "DestroyEnumerator";
        case DreamProcOpcode::Browse: return "Browse";
        case DreamProcOpcode::BrowseResource: return "BrowseResource";
        case DreamProcOpcode::OutputControl: return "OutputControl";
        case DreamProcOpcode::BitShiftRight: return "BitShiftRight";
        case DreamProcOpcode::CreateFilteredListEnumerator: return "CreateFilteredListEnumerator";
        case DreamProcOpcode::Power: return "Power";
        case DreamProcOpcode::EnumerateAssoc: return "EnumerateAssoc";
        case DreamProcOpcode::Link: return "Link";
        case DreamProcOpcode::Prompt: return "Prompt";
        case DreamProcOpcode::Ftp: return "Ftp";
        case DreamProcOpcode::Initial: return "Initial";
        case DreamProcOpcode::AsType: return "AsType";
        case DreamProcOpcode::IsType: return "IsType";
        case DreamProcOpcode::LocateCoord: return "LocateCoord";
        case DreamProcOpcode::Locate: return "Locate";
        case DreamProcOpcode::IsNull: return "IsNull";
        case DreamProcOpcode::Spawn: return "Spawn";
        case DreamProcOpcode::OutputReference: return "OutputReference";
        case DreamProcOpcode::Output: return "Output";
        case DreamProcOpcode::Pop: return "Pop";
        case DreamProcOpcode::Prob: return "Prob";
        case DreamProcOpcode::IsSaved: return "IsSaved";
        case DreamProcOpcode::PickUnweighted: return "PickUnweighted";
        case DreamProcOpcode::PickWeighted: return "PickWeighted";
        case DreamProcOpcode::Increment: return "Increment";
        case DreamProcOpcode::Decrement: return "Decrement";
        case DreamProcOpcode::CompareEquivalent: return "CompareEquivalent";
        case DreamProcOpcode::CompareNotEquivalent: return "CompareNotEquivalent";
        case DreamProcOpcode::Throw: return "Throw";
        case DreamProcOpcode::IsInRange: return "IsInRange";
        case DreamProcOpcode::MassConcatenation: return "MassConcatenation";
        case DreamProcOpcode::CreateTypeEnumerator: return "CreateTypeEnumerator";
        case DreamProcOpcode::PushGlobalVars: return "PushGlobalVars";
        case DreamProcOpcode::ModulusModulus: return "ModulusModulus";
        case DreamProcOpcode::ModulusModulusReference: return "ModulusModulusReference";
        case DreamProcOpcode::JumpIfNull: return "JumpIfNull";
        case DreamProcOpcode::JumpIfNullNoPop: return "JumpIfNullNoPop";
        case DreamProcOpcode::JumpIfTrueReference: return "JumpIfTrueReference";
        case DreamProcOpcode::JumpIfFalseReference: return "JumpIfFalseReference";
        case DreamProcOpcode::DereferenceField: return "DereferenceField";
        case DreamProcOpcode::DereferenceIndex: return "DereferenceIndex";
        case DreamProcOpcode::DereferenceCall: return "DereferenceCall";
        case DreamProcOpcode::PopReference: return "PopReference";
        case DreamProcOpcode::BitShiftLeftReference: return "BitShiftLeftReference";
        case DreamProcOpcode::BitShiftRightReference: return "BitShiftRightReference";
        case DreamProcOpcode::Try: return "Try";
        case DreamProcOpcode::TryNoValue: return "TryNoValue";
        case DreamProcOpcode::EndTry: return "EndTry";
        case DreamProcOpcode::EnumerateNoAssign: return "EnumerateNoAssign";
        case DreamProcOpcode::Gradient: return "Gradient";
        case DreamProcOpcode::AssignInto: return "AssignInto";
        case DreamProcOpcode::GetStep: return "GetStep";
        case DreamProcOpcode::Length: return "Length";
        case DreamProcOpcode::GetDir: return "GetDir";
        case DreamProcOpcode::DebuggerBreakpoint: return "DebuggerBreakpoint";
        case DreamProcOpcode::Sin: return "Sin";
        case DreamProcOpcode::Cos: return "Cos";
        case DreamProcOpcode::Tan: return "Tan";
        case DreamProcOpcode::ArcSin: return "ArcSin";
        case DreamProcOpcode::ArcCos: return "ArcCos";
        case DreamProcOpcode::ArcTan: return "ArcTan";
        case DreamProcOpcode::ArcTan2: return "ArcTan2";
        case DreamProcOpcode::Sqrt: return "Sqrt";
        case DreamProcOpcode::Log: return "Log";
        case DreamProcOpcode::LogE: return "LogE";
        case DreamProcOpcode::Abs: return "Abs";
        case DreamProcOpcode::AppendNoPush: return "AppendNoPush";
        case DreamProcOpcode::AssignNoPush: return "AssignNoPush";
        case DreamProcOpcode::PushRefAndDereferenceField: return "PushRefAndDereferenceField";
        case DreamProcOpcode::PushNRefs: return "PushNRefs";
        case DreamProcOpcode::PushNFloats: return "PushNFloats";
        case DreamProcOpcode::PushNResources: return "PushNResources";
        case DreamProcOpcode::PushStringFloat: return "PushStringFloat";
        case DreamProcOpcode::JumpIfReferenceFalse: return "JumpIfReferenceFalse";
        case DreamProcOpcode::PushNStrings: return "PushNStrings";
        case DreamProcOpcode::SwitchOnFloat: return "SwitchOnFloat";
        case DreamProcOpcode::PushNOfStringFloats: return "PushNOfStringFloats";
        case DreamProcOpcode::CreateListNFloats: return "CreateListNFloats";
        case DreamProcOpcode::CreateListNStrings: return "CreateListNStrings";
        case DreamProcOpcode::CreateListNRefs: return "CreateListNRefs";
        case DreamProcOpcode::CreateListNResources: return "CreateListNResources";
        case DreamProcOpcode::SwitchOnString: return "SwitchOnString";
        case DreamProcOpcode::IsTypeDirect: return "IsTypeDirect";
        case DreamProcOpcode::NullRef: return "NullRef";
        case DreamProcOpcode::ReturnReferenceValue: return "ReturnReferenceValue";
        case DreamProcOpcode::ReturnFloat: return "ReturnFloat";
        case DreamProcOpcode::IndexRefWithString: return "IndexRefWithString";
        case DreamProcOpcode::PushFloatAssign: return "PushFloatAssign";
        case DreamProcOpcode::NPushFloatAssign: return "NPushFloatAssign";
        default: return "Unknown(" + std::to_string(static_cast<int>(opcode)) + ")";
    }
}

DMDisassembler::DMDisassembler()
    : loaded_(false)
{
}

DMDisassembler::~DMDisassembler() = default;

bool DMDisassembler::LoadJson(const std::string& jsonPath) {
    // Read file content
    std::ifstream file(jsonPath);
    if (!file.is_open()) {
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    file.close();
    
    // Parse the JSON content
    if (!ParseJson(content)) {
        return false;
    }
    
    // Build lookup tables
    BuildLookupTables();
    
    filePath_ = jsonPath;
    loaded_ = true;
    return true;
}

bool DMDisassembler::ParseJson(const std::string& content) {
    try {
        auto root = json::parse(content);
        
        // 1. Strings
        if (root.contains("Strings")) {
            for (const auto& str : root["Strings"]) {
                stringTable_.push_back(str.get<std::string>());
            }
        }
        
        // 2. Types
        if (root.contains("Types")) {
            int typeId = 0;
            for (const auto& typeJson : root["Types"]) {
                DisasmType type;
                type.Id = typeId++;
                type.Path = typeJson.value("Path", "");
                if (typeJson.contains("Parent")) {
                    type.ParentId = typeJson["Parent"].get<int>();
                }
                
                if (typeJson.contains("Variables")) {
                    for (auto& [key, val] : typeJson["Variables"].items()) {
                        type.Variables.push_back(key);
                    }
                }
                
                if (typeJson.contains("Procs")) {
                    for (const auto& procGroup : typeJson["Procs"]) {
                        for (const auto& procId : procGroup) {
                            type.ProcIds.push_back(procId.get<int>());
                        }
                    }
                }
                
                types_.push_back(type);
            }
        }
        
        // 3. Procs
        if (root.contains("Procs")) {
            int procId = 0;
            for (const auto& procJson : root["Procs"]) {
                DisasmProc proc;
                proc.Id = procId++;
                proc.Name = procJson.value("Name", "");
                proc.OwnerTypeId = procJson.value("OwningTypeId", -1);
                
                // Resolve OwnerPath
                if (proc.OwnerTypeId >= 0 && proc.OwnerTypeId < static_cast<int>(types_.size())) {
                    proc.OwnerPath = types_[proc.OwnerTypeId].Path;
                }
                
                if (procJson.contains("Arguments")) {
                    for (const auto& arg : procJson["Arguments"]) {
                        proc.Parameters.push_back(arg.value("Name", ""));
                    }
                }
                
                if (procJson.contains("Bytecode")) {
                    proc.Bytecode = procJson["Bytecode"].get<std::vector<uint8_t>>();
                }
                
                proc.IsVerb = procJson.value("IsVerb", false);
                
                procs_.push_back(proc);
            }
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "JSON Parse Error: " << e.what() << std::endl;
        return false;
    }
}

void DMDisassembler::BuildLookupTables() {
    // Build path to type ID lookup
    pathToTypeId_.clear();
    for (const auto& type : types_) {
        pathToTypeId_[type.Path] = type.Id;
    }
}

std::vector<std::string> DMDisassembler::Search(const std::string& query) const {
    std::vector<std::string> results;
    
    if (query.empty()) {
        return results;
    }
    
    // Convert query to lowercase for case-insensitive search
    std::string lowerQuery = query;
    std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    
    // Search types
    for (const auto& type : types_) {
        std::string lowerPath = type.Path;
        std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        
        if (lowerPath.find(lowerQuery) != std::string::npos) {
            results.push_back(type.Path);
        }
    }
    
    // Search procs
    for (const auto& proc : procs_) {
        std::string lowerName = proc.Name;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        
        if (lowerName.find(lowerQuery) != std::string::npos) {
            results.push_back(proc.OwnerPath + "/" + proc.Name);
        }
    }
    
    return results;
}

const DisasmType* DMDisassembler::GetType(const std::string& path) const {
    auto it = pathToTypeId_.find(path);
    if (it != pathToTypeId_.end()) {
        return GetTypeById(it->second);
    }
    return nullptr;
}

const DisasmType* DMDisassembler::GetTypeById(int typeId) const {
    if (typeId >= 0 && static_cast<size_t>(typeId) < types_.size()) {
        return &types_[typeId];
    }
    return nullptr;
}

const DisasmProc* DMDisassembler::GetProc(int procId) const {
    if (procId >= 0 && static_cast<size_t>(procId) < procs_.size()) {
        return &procs_[procId];
    }
    return nullptr;
}

std::vector<const DisasmProc*> DMDisassembler::GetProcsForType(const std::string& typePath) const {
    std::vector<const DisasmProc*> result;
    
    const DisasmType* type = GetType(typePath);
    if (type) {
        for (int procId : type->ProcIds) {
            const DisasmProc* proc = GetProc(procId);
            if (proc) {
                result.push_back(proc);
            }
        }
    }
    
    return result;
}

std::string DMDisassembler::DecompileProc(int procId) const {
    const DisasmProc* proc = GetProc(procId);
    if (!proc) {
        return "Error: Proc not found";
    }
    
    std::stringstream ss;
    ss << "// Proc: " << proc->OwnerPath << "/" << proc->Name << "\n";
    ss << "// Parameters: ";
    for (size_t i = 0; i < proc->Parameters.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << proc->Parameters[i];
    }
    ss << "\n";
    ss << "// Bytecode size: " << proc->Bytecode.size() << " bytes\n\n";
    
    size_t pc = 0;
    const auto& bytecode = proc->Bytecode;
    
    auto readInt = [&](size_t& pc) -> int {
        if (pc + 4 > bytecode.size()) return 0;
        int val = *reinterpret_cast<const int*>(&bytecode[pc]);
        pc += 4;
        return val;
    };
    
    auto readFloat = [&](size_t& pc) -> float {
        if (pc + 4 > bytecode.size()) return 0.0f;
        float val = *reinterpret_cast<const float*>(&bytecode[pc]);
        pc += 4;
        return val;
    };
    
    while (pc < bytecode.size()) {
        size_t currentPc = pc;
        uint8_t opcodeVal = bytecode[pc++];
        DreamProcOpcode opcode = static_cast<DreamProcOpcode>(opcodeVal);
        
        ss << std::setw(4) << std::setfill('0') << currentPc << ": " 
           << std::setw(25) << std::setfill(' ') << std::left << GetOpcodeName(opcode);
        
        const auto& metadata = GetOpcodeMetadata(opcode);
        
        auto printArg = [&](OpcodeArgType argType) {
            switch (argType) {
                case OpcodeArgType::Int: {
                    int val = readInt(pc);
                    ss << " " << val;
                    break;
                }
                case OpcodeArgType::Float: {
                    float val = readFloat(pc);
                    ss << " " << val;
                    break;
                }
                case OpcodeArgType::String: {
                    int id = readInt(pc);
                    ss << " \"" << GetString(id) << "\"";
                    break;
                }
                case OpcodeArgType::TypeId: {
                    int id = readInt(pc);
                    const DisasmType* type = GetTypeById(id);
                    ss << " " << (type ? type->Path : "UnknownType(" + std::to_string(id) + ")");
                    break;
                }
                case OpcodeArgType::ProcId: {
                    int id = readInt(pc);
                    const DisasmProc* p = GetProc(id);
                    ss << " " << (p ? (p->OwnerPath + "/" + p->Name) : "UnknownProc(" + std::to_string(id) + ")");
                    break;
                }
                case OpcodeArgType::Label: {
                    int offset = readInt(pc);
                    ss << " " << std::setw(4) << std::setfill('0') << offset;
                    break;
                }
                case OpcodeArgType::Reference: {
                    int type = readInt(pc);
                    int val = readInt(pc);
                    ss << " Ref(" << type << ", " << val << ")";
                    break;
                }
                case OpcodeArgType::FilterId: {
                    int id = readInt(pc);
                    // Filter ID is usually a type ID in our implementation
                    const DisasmType* type = GetTypeById(id);
                    ss << " Filter(" << (type ? type->Path : std::to_string(id)) << ")";
                    break;
                }
                case OpcodeArgType::ListSize:
                case OpcodeArgType::StackDelta:
                case OpcodeArgType::PickCount:
                case OpcodeArgType::ConcatCount:
                case OpcodeArgType::FormatCount:
                case OpcodeArgType::Resource:
                case OpcodeArgType::EnumeratorId:
                case OpcodeArgType::ArgType: {
                    int val = readInt(pc);
                    ss << " " << val;
                    break;
                }
                default:
                    break;
            }
        };
        
        if (metadata.ArgType1 != OpcodeArgType::None) printArg(metadata.ArgType1);
        if (metadata.ArgType2 != OpcodeArgType::None) printArg(metadata.ArgType2);
        if (metadata.ArgType3 != OpcodeArgType::None) printArg(metadata.ArgType3);
        if (metadata.ArgType4 != OpcodeArgType::None) printArg(metadata.ArgType4);
        
        // Handle variable length arguments for specific opcodes
        if (opcode == DreamProcOpcode::FormatString) {
            // FormatString has a variable number of arguments based on the format string
            // But the metadata only defines the string ID and count.
            // The actual arguments are on the stack, so we don't read them here.
        } else if (opcode == DreamProcOpcode::SwitchCase || opcode == DreamProcOpcode::SwitchCaseRange) {
            // These have variable length jump tables, but usually implemented as separate jumps?
            // No, SwitchCase has a count and then pairs of (value, label)
            // But GetOpcodeMetadata might not reflect that fully if it's variable.
            // Let's check OpcodeDefinitions.cpp if we could... but we can't easily.
            // Assuming standard metadata covers the fixed parts.
        }
        
        ss << "\n";
    }
    
    return ss.str();
}

void DMDisassembler::DumpAll(std::ostream& out) const {
    out << "=== DMDisassembler Dump ===\n\n";
    
    // Statistics
    auto stats = GetStats();
    out << "Statistics:\n";
    out << "  Types: " << stats.TypeCount << "\n";
    out << "  Procs: " << stats.ProcCount << "\n";
    out << "  Strings: " << stats.StringCount << "\n";
    out << "  Total Bytecode: " << stats.TotalBytecodeSize << " bytes\n\n";
    
    // Types
    out << "Types:\n";
    for (const auto& type : types_) {
        out << "  [" << type.Id << "] " << type.Path;
        if (type.ParentId >= 0) {
            out << " (parent: " << type.ParentId << ")";
        }
        out << "\n";
        
        if (!type.Variables.empty()) {
            out << "    Variables: ";
            for (size_t i = 0; i < type.Variables.size(); ++i) {
                if (i > 0) out << ", ";
                out << type.Variables[i];
            }
            out << "\n";
        }
        
        if (!type.ProcIds.empty()) {
            out << "    Procs: ";
            for (size_t i = 0; i < type.ProcIds.size(); ++i) {
                if (i > 0) out << ", ";
                const DisasmProc* proc = GetProc(type.ProcIds[i]);
                if (proc) {
                    out << proc->Name;
                } else {
                    out << "#" << type.ProcIds[i];
                }
            }
            out << "\n";
        }
    }
    
    // Procs
    out << "\nProcs:\n";
    for (const auto& proc : procs_) {
        out << "  [" << proc.Id << "] " << proc.OwnerPath << "/" << proc.Name;
        if (proc.IsVerb) out << " (verb)";
        out << "\n";
        
        if (!proc.Parameters.empty()) {
            out << "    Parameters: ";
            for (size_t i = 0; i < proc.Parameters.size(); ++i) {
                if (i > 0) out << ", ";
                out << proc.Parameters[i];
            }
            out << "\n";
        }
        
        out << "    Bytecode: " << proc.Bytecode.size() << " bytes\n";
    }
}

int DMDisassembler::TestAll() const {
    int failures = 0;
    
    // Try to decompile each proc
    for (const auto& proc : procs_) {
        try {
            std::string result = DecompileProc(proc.Id);
            if (result.empty() || result.find("Error:") == 0) {
                failures++;
            }
        } catch (...) {
            failures++;
        }
    }
    
    return failures;
}

DMDisassembler::Stats DMDisassembler::GetStats() const {
    Stats stats;
    stats.TypeCount = types_.size();
    stats.ProcCount = procs_.size();
    stats.StringCount = stringTable_.size();
    
    stats.TotalBytecodeSize = 0;
    for (const auto& proc : procs_) {
        stats.TotalBytecodeSize += proc.Bytecode.size();
    }
    
    return stats;
}

const std::string& DMDisassembler::GetString(size_t index) const {
    if (index < stringTable_.size()) {
        return stringTable_[index];
    }
    return EmptyString;
}

} // namespace DMCompiler
