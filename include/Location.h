#pragma once

#include <string>
#include <cstdint>

namespace DMCompiler {

/// <summary>
/// Represents a location in source code (file, line, column)
/// </summary>
class Location {
public:
    std::string SourceFile;
    int Line;
    int Column;
    bool InDMStandard; // True if this location is within DMStandard files

    Location() : Line(0), Column(0), InDMStandard(false) {}
    Location(const std::string& sourceFile, int line, int column, bool inDMStandard = false)
        : SourceFile(sourceFile), Line(line), Column(column), InDMStandard(inDMStandard) {}

    static Location Internal;

    std::string ToString() const;
    bool IsInternal() const { return SourceFile == "internal"; }
};

} // namespace DMCompiler
