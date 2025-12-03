#include "DMASTExpression.h"
#include "DMCompiler.h"
#include <cmath>
#include <limits>

namespace DMCompiler {

// DMASTConstantNull
bool DMASTConstantNull::TryAsJsonRepresentation(DMCompiler* compiler, JsonValue& outJson) {
    outJson = nullptr;
    return true;
}

// DMASTConstantInteger
bool DMASTConstantInteger::TryAsJsonRepresentation(DMCompiler* compiler, JsonValue& outJson) {
    outJson = static_cast<int64_t>(Value);
    return true;
}

// DMASTConstantFloat
bool DMASTConstantFloat::TryAsJsonRepresentation(DMCompiler* compiler, JsonValue& outJson) {
    // Check for infinity
    if (std::isinf(Value)) {
        std::unordered_map<std::string, std::string> specialValue;
        if (Value > 0) {
            specialValue["type"] = "PositiveInfinity";
        } else {
            specialValue["type"] = "NegativeInfinity";
        }
        outJson = specialValue;
    } else {
        outJson = static_cast<double>(Value);
    }
    return true;
}

// DMASTConstantString
bool DMASTConstantString::TryAsJsonRepresentation(DMCompiler* compiler, JsonValue& outJson) {
    outJson = Value;
    return true;
}

// DMASTStringFormat
bool DMASTStringFormat::TryAsJsonRepresentation(DMCompiler* compiler, JsonValue& outJson) {
    // String format with embedded expressions - for now, just return a placeholder
    // Full support would require extending JsonValue to support arrays/objects
    std::string result = "[string format: ";
    result += std::to_string(StringParts.size()) + " parts, ";
    result += std::to_string(Expressions.size()) + " expressions]";
    outJson = result;
    return true;
}

// DMASTConstantResource
bool DMASTConstantResource::TryAsJsonRepresentation(DMCompiler* compiler, JsonValue& outJson) {
    // Resources are serialized as their path string
    outJson = Path;
    return true;
}

// DMASTConstantPath
bool DMASTConstantPath::TryAsJsonRepresentation(DMCompiler* compiler, JsonValue& outJson) {
    // Paths are serialized as their string representation
    outJson = Path.Path.ToString();
    return true;
}

} // namespace DMCompiler
