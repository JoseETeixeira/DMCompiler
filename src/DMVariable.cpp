#include "DMVariable.h"
#include <utility>

namespace DMCompiler {

DMVariable::DMVariable()
    : Type(std::nullopt)
    , Name("")
    , IsGlobal(false)
    , IsConst(false)
    , IsFinal(false)
    , IsTmp(false)
    , Value(nullptr)
    , ValType(DMComplexValueType::Anything())
{
}

DMVariable::DMVariable(
    std::optional<DreamPath> type,
    std::string name,
    bool isGlobal,
    bool isConst,
    bool isFinal,
    bool isTmp,
    DMComplexValueType valType
)
    : Type(std::move(type))
    , Name(std::move(name))
    , IsGlobal(isGlobal)
    , IsConst(isConst)
    , IsFinal(isFinal)
    , IsTmp(isTmp)
    , Value(nullptr)
    , ValType(std::move(valType))
{
}

DMVariable::DMVariable(const DMVariable& other)
    : Type(other.Type)
    , Name(other.Name)
    , IsGlobal(other.IsGlobal)
    , IsConst(other.IsConst)
    , IsFinal(other.IsFinal)
    , IsTmp(other.IsTmp)
    , Value(other.Value)
    , ValType(other.ValType)
{
}

DMVariable::DMVariable(DMVariable&& other) noexcept
    : Type(std::move(other.Type))
    , Name(std::move(other.Name))
    , IsGlobal(other.IsGlobal)
    , IsConst(other.IsConst)
    , IsFinal(other.IsFinal)
    , IsTmp(other.IsTmp)
    , Value(other.Value)
    , ValType(std::move(other.ValType))
{
    other.Value = nullptr;
}

DMVariable& DMVariable::operator=(const DMVariable& other) {
    if (this != &other) {
        Type = other.Type;
        Name = other.Name;
        IsGlobal = other.IsGlobal;
        IsConst = other.IsConst;
        IsFinal = other.IsFinal;
        IsTmp = other.IsTmp;
        Value = other.Value;
        ValType = other.ValType;
    }
    return *this;
}

DMVariable& DMVariable::operator=(DMVariable&& other) noexcept {
    if (this != &other) {
        Type = std::move(other.Type);
        Name = std::move(other.Name);
        IsGlobal = other.IsGlobal;
        IsConst = other.IsConst;
        IsFinal = other.IsFinal;
        IsTmp = other.IsTmp;
        Value = other.Value;
        ValType = std::move(other.ValType);
        other.Value = nullptr;
    }
    return *this;
}

} // namespace DMCompiler
