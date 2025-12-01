#include "DreamPath.h"
#include <sstream>
#include <algorithm>

namespace DMCompiler {

// Static paths
DreamPath DreamPath::Root = DreamPath("/");
DreamPath DreamPath::Datum = DreamPath("/datum");
DreamPath DreamPath::Atom = DreamPath("/atom");
DreamPath DreamPath::Area = DreamPath("/area");
DreamPath DreamPath::Turf = DreamPath("/turf");
DreamPath DreamPath::Obj = DreamPath("/obj");
DreamPath DreamPath::Mob = DreamPath("/mob");
DreamPath DreamPath::Client = DreamPath("/client");
DreamPath DreamPath::List = DreamPath("/list");

DreamPath::DreamPath()
    : Type_(PathType::Absolute)
{
}

DreamPath::DreamPath(const std::string& pathString) {
    ParseFromString(pathString);
}

DreamPath::DreamPath(PathType type, const std::vector<std::string>& elements)
    : Type_(type)
    , Elements_(elements)
{
}

void DreamPath::ParseFromString(const std::string& pathString) {
    if (pathString.empty()) {
        Type_ = PathType::Absolute;
        return;
    }
    
    if (pathString[0] == '/') {
        Type_ = PathType::Absolute;
    } else if (pathString[0] == '.') {
        if (pathString.size() > 1 && pathString[1] == '.') {
            Type_ = PathType::UpwardSearch;
        } else {
            Type_ = PathType::Relative;
        }
    } else {
        Type_ = PathType::Relative;
    }
    
    // Split by /
    std::string current;
    for (size_t i = (Type_ == PathType::Absolute ? 1 : 0); i < pathString.size(); ++i) {
        if (pathString[i] == '/') {
            if (!current.empty()) {
                Elements_.push_back(current);
                current.clear();
            }
        } else {
            current += pathString[i];
        }
    }
    
    if (!current.empty()) {
        Elements_.push_back(current);
    }
}

std::string DreamPath::ToString() const {
    std::ostringstream oss;
    
    switch (Type_) {
        case PathType::Absolute:
            oss << "/";
            break;
        case PathType::Relative:
            oss << ".";
            break;
        case PathType::UpwardSearch:
            oss << "..";
            break;
    }
    
    for (size_t i = 0; i < Elements_.size(); ++i) {
        if (i > 0) {
            oss << "/";
        }
        oss << Elements_[i];
    }
    
    return oss.str();
}

DreamPath DreamPath::Combine(const DreamPath& other) const {
    if (other.Type_ == PathType::Absolute) {
        return other;
    }
    
    std::vector<std::string> newElements = Elements_;
    newElements.insert(newElements.end(), other.Elements_.begin(), other.Elements_.end());
    
    return DreamPath(Type_, newElements);
}

DreamPath DreamPath::AddToPath(const std::string& element) const {
    std::vector<std::string> newElements = Elements_;
    newElements.push_back(element);
    return DreamPath(Type_, newElements);
}

DreamPath DreamPath::RemoveLastElement() const {
    if (Elements_.empty()) {
        return *this;
    }
    
    std::vector<std::string> newElements(Elements_.begin(), Elements_.end() - 1);
    return DreamPath(Type_, newElements);
}

std::string DreamPath::GetLastElement() const {
    if (Elements_.empty()) {
        return "";
    }
    return Elements_.back();
}

bool DreamPath::IsDescendantOf(const DreamPath& ancestor) const {
    if (ancestor.Type_ != PathType::Absolute || Type_ != PathType::Absolute) {
        return false;
    }
    
    if (ancestor.Elements_.size() > Elements_.size()) {
        return false;
    }
    
    for (size_t i = 0; i < ancestor.Elements_.size(); ++i) {
        if (ancestor.Elements_[i] != Elements_[i]) {
            return false;
        }
    }
    
    return true;
}

bool DreamPath::operator==(const DreamPath& other) const {
    return Type_ == other.Type_ && Elements_ == other.Elements_;
}

} // namespace DMCompiler
