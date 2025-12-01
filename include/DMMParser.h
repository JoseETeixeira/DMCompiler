#pragma once

#include "DMParser.h"
#include "DMLexer.h"
#include "Token.h"
#include "DreamPath.h"
#include "JsonWriter.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <unordered_set>

namespace DMCompiler {

class DMCompiler;

/// <summary>
/// JSON representation of a map object in a DMM file.
/// Contains type ID and optional variable overrides.
/// </summary>
struct MapObjectJson {
    int Type;
    std::unordered_map<std::string, JsonValue> VarOverrides;
    
    MapObjectJson(int type) : Type(type) {}
    
    bool AddVarOverride(const std::string& varName, const JsonValue& varValue) {
        auto result = VarOverrides.insert({varName, varValue});
        return result.second; // Returns true if insertion happened (key didn't exist)
    }
};

/// <summary>
/// JSON representation of a cell definition in a DMM file.
/// Defines what objects, turf, and area exist at a cell location.
/// </summary>
struct CellDefinitionJson {
    std::string Name;
    std::unique_ptr<MapObjectJson> Turf;
    std::unique_ptr<MapObjectJson> Area;
    std::vector<std::unique_ptr<MapObjectJson>> Objects;
    
    CellDefinitionJson(const std::string& name) : Name(name) {}
};

/// <summary>
/// JSON representation of a map block in a DMM file.
/// Contains the coordinates and cell references for a rectangular region.
/// </summary>
struct MapBlockJson {
    int X;
    int Y;
    int Z;
    int Width;
    int Height;
    std::vector<std::string> Cells;
    
    MapBlockJson(int x, int y, int z) : X(x), Y(y), Z(z), Width(0), Height(0) {}
};

/// <summary>
/// JSON representation of an entire DMM map file.
/// Contains all cell definitions and map blocks.
/// </summary>
struct DreamMapJson {
    int MaxX;
    int MaxY;
    int MaxZ;
    std::unordered_map<std::string, std::unique_ptr<CellDefinitionJson>> CellDefinitions;
    std::vector<std::unique_ptr<MapBlockJson>> Blocks;
    
    DreamMapJson() : MaxX(0), MaxY(0), MaxZ(0) {}
};

/// <summary>
/// Parser for DMM (BYOND Map) files.
/// Inherits from DMParser to reuse token handling and expression parsing.
/// </summary>
class DMMParser : public DMParser {
public:
    DMMParser(DMCompiler* compiler, DMLexer* lexer, int zOffset);
    
    /// <summary>
    /// Parse the entire DMM file and return the map structure.
    /// </summary>
    std::unique_ptr<DreamMapJson> ParseMap();
    
    /// <summary>
    /// Parse a single cell definition (e.g., "aaa" = (/turf/floor, /area/room))
    /// </summary>
    std::unique_ptr<CellDefinitionJson> ParseCellDefinition();
    
    /// <summary>
    /// Parse a map block (e.g., (1,1,1) = {"aaaaabbbbb"})
    /// </summary>
    std::unique_ptr<MapBlockJson> ParseMapBlock();

private:
    int ZOffset_;
    int CellNameLength_;
    std::unordered_set<DreamPath, DreamPathHash> SkippedTypes_;
    
    /// <summary>
    /// Parse coordinates like (1,1,1) from a map block header.
    /// Returns nullptr if no coordinates are found.
    /// </summary>
    struct Coordinates {
        int X;
        int Y;
        int Z;
    };
    std::unique_ptr<Coordinates> ParseCoordinates();
    
    /// <summary>
    /// Parse a DM path expression (e.g., /turf/floor/steel)
    /// </summary>
    std::unique_ptr<DMASTPath> PathExpression();
    
    /// <summary>
    /// Parse a constant value expression
    /// </summary>
    std::unique_ptr<DMASTExpression> ConstantExpression();
};

} // namespace DMCompiler
