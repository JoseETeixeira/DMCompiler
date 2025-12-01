#pragma once

#include <string>
#include <vector>
#include <memory>

namespace DMCompiler {

/// <summary>
/// Represents a path in the DM object tree (e.g., /obj/item/weapon)
/// </summary>
class DreamPath {
public:
    enum class PathType {
        Absolute,  // Starts with /
        Relative,  // Starts with .
        UpwardSearch  // Starts with ..
    };

    DreamPath();
    explicit DreamPath(const std::string& pathString);
    DreamPath(PathType type, const std::vector<std::string>& elements);

    PathType GetPathType() const { return Type_; }
    const std::vector<std::string>& GetElements() const { return Elements_; }
    std::string ToString() const;
    
    // Path operations
    DreamPath Combine(const DreamPath& other) const;
    DreamPath AddToPath(const std::string& element) const;
    DreamPath RemoveLastElement() const;
    std::string GetLastElement() const;
    
    bool IsDescendantOf(const DreamPath& ancestor) const;
    bool operator==(const DreamPath& other) const;
    bool operator!=(const DreamPath& other) const { return !(*this == other); }
    
    // Special paths
    static DreamPath Root;
    static DreamPath Datum;
    static DreamPath Atom;
    static DreamPath Area;
    static DreamPath Turf;
    static DreamPath Obj;
    static DreamPath Mob;
    static DreamPath Client;
    static DreamPath List;

private:
    PathType Type_;
    std::vector<std::string> Elements_;
    
    void ParseFromString(const std::string& pathString);
};

/// <summary>
/// Hash function for DreamPath to use in unordered containers
/// </summary>
struct DreamPathHash {
    std::size_t operator()(const DreamPath& path) const {
        // Simple hash based on string representation
        return std::hash<std::string>{}(path.ToString());
    }
};

} // namespace DMCompiler
