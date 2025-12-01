#include "DMAST.h"
#include "DMASTExpression.h"
#include "DMASTStatement.h"
#include <sstream>

namespace DMCompiler {

// ============================================================================
// DMASTNode
// ============================================================================

std::string DMASTNode::ToString() const {
    std::ostringstream oss;
    oss << typeid(*this).name() << " [" << Location_.ToString() << "]";
    return oss.str();
}

// ============================================================================
// DMASTFile
// ============================================================================

DMASTFile::DMASTFile(const Location& location, std::vector<std::unique_ptr<DMASTStatement>> statements)
    : DMASTNode(location), Statements(std::move(statements)) {}

// ============================================================================
// DMASTBlockInner
// ============================================================================

DMASTBlockInner::DMASTBlockInner(const Location& location, std::vector<std::unique_ptr<DMASTStatement>> statements)
    : DMASTNode(location), Statements(std::move(statements)) {}

// ============================================================================
// DMASTProcBlockInner
// ============================================================================

DMASTProcBlockInner::DMASTProcBlockInner(const Location& location)
    : DMASTNode(location) {}

DMASTProcBlockInner::DMASTProcBlockInner(const Location& location,
                                         std::vector<std::unique_ptr<DMASTProcStatement>> statements,
                                         std::vector<std::unique_ptr<DMASTProcStatement>> setStatements)
    : DMASTNode(location), Statements(std::move(statements)), SetStatements(std::move(setStatements)) {}

// DMASTPath is now a struct with inline constructor, no implementation needed

// ============================================================================
// DMASTCallParameter
// ============================================================================

DMASTCallParameter::DMASTCallParameter(const Location& location,
                                       std::unique_ptr<DMASTExpression> value,
                                       std::unique_ptr<DMASTExpression> key)
    : DMASTNode(location), Value(std::move(value)), Key(std::move(key)) {}

// ============================================================================
// DMASTDefinitionParameter
// ============================================================================

DMASTDefinitionParameter::DMASTDefinitionParameter(const Location& location,
                                                   const std::string& name,
                                                   const DreamPath& typePath,
                                                   bool isList,
                                                   std::unique_ptr<DMASTExpression> defaultValue,
                                                   std::unique_ptr<DMASTExpression> possibleValues,
                                                   std::optional<DMComplexValueType> explicitValueType)
    : DMASTNode(location), Name(name), TypePath(typePath), IsList(isList),
      DefaultValue(std::move(defaultValue)), PossibleValues(std::move(possibleValues)),
      ExplicitValueType(explicitValueType) {}

} // namespace DMCompiler

