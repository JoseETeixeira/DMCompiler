#include "DMBuiltinRegistry.h"
#include <algorithm>

namespace DMCompiler {

DMBuiltinRegistry& DMBuiltinRegistry::Instance() {
    static DMBuiltinRegistry instance;
    return instance;
}

DMBuiltinRegistry::DMBuiltinRegistry() {
    InitializeBuiltins();
}

void DMBuiltinRegistry::InitializeBuiltins() {
    // Initialize context variables
    ContextVars_ = {"src", "usr", "args", "global", "world", ".", ".."};
    
    // Initialize built-in types
    BuiltinTypes_ = {
        "/datum", "/atom", "/atom/movable", "/obj", "/mob", "/turf", "/area", 
        "/client", "/world", "/list", "/savefile", "/sound", "/image", "/icon", "/matrix", "/regex", "/exception"
    };
    
    // Initialize type hierarchy for built-ins
    TypeHierarchy_["/atom"] = "/datum";
    TypeHierarchy_["/atom/movable"] = "/atom";
    TypeHierarchy_["/obj"] = "/atom/movable";
    TypeHierarchy_["/mob"] = "/atom/movable";
    TypeHierarchy_["/turf"] = "/atom";
    TypeHierarchy_["/area"] = "/atom";
    TypeHierarchy_["/client"] = "/datum";
    TypeHierarchy_["/world"] = "/datum";
    TypeHierarchy_["/list"] = "/datum";
    TypeHierarchy_["/savefile"] = "/datum";
    TypeHierarchy_["/sound"] = "/datum";
    TypeHierarchy_["/image"] = "/datum";
    TypeHierarchy_["/icon"] = "/datum";
    TypeHierarchy_["/matrix"] = "/datum";
    TypeHierarchy_["/regex"] = "/datum";
    TypeHierarchy_["/exception"] = "/datum";

    RegisterGlobalProcs();
    RegisterTypeProcs();
    RegisterBuiltinVars();
}

bool DMBuiltinRegistry::IsGlobalBuiltinProc(const std::string& name) const {
    return GlobalProcs_.find(name) != GlobalProcs_.end();
}

bool DMBuiltinRegistry::IsTypeBuiltinProc(const DreamPath& type, const std::string& name) const {
    std::string typeStr = type.ToString();
    
    // Check exact type match
    auto it = TypeProcs_.find(typeStr);
    if (it != TypeProcs_.end()) {
        if (it->second.count(name)) return true;
    }
    
    // Check inheritance
    std::string currentType = typeStr;
    while (TypeHierarchy_.count(currentType)) {
        currentType = TypeHierarchy_.at(currentType);
        auto parentIt = TypeProcs_.find(currentType);
        if (parentIt != TypeProcs_.end()) {
            if (parentIt->second.count(name)) return true;
        }
    }
    
    return false;
}

std::optional<ProcSignature> DMBuiltinRegistry::GetGlobalProcSignature(const std::string& name) const {
    auto it = GlobalProcs_.find(name);
    if (it != GlobalProcs_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<ProcSignature> DMBuiltinRegistry::GetTypeProcSignature(const DreamPath& type, const std::string& name) const {
    std::string typeStr = type.ToString();
    
    // Check exact type match
    auto it = TypeProcs_.find(typeStr);
    if (it != TypeProcs_.end()) {
        auto procIt = it->second.find(name);
        if (procIt != it->second.end()) {
            return procIt->second;
        }
    }
    
    // Check inheritance
    std::string currentType = typeStr;
    while (TypeHierarchy_.count(currentType)) {
        currentType = TypeHierarchy_.at(currentType);
        auto parentIt = TypeProcs_.find(currentType);
        if (parentIt != TypeProcs_.end()) {
            auto procIt = parentIt->second.find(name);
            if (procIt != parentIt->second.end()) {
                return procIt->second;
            }
        }
    }
    
    return std::nullopt;
}

bool DMBuiltinRegistry::IsBuiltinVar(const DreamPath& type, const std::string& name) const {
    std::string typeStr = type.ToString();
    
    // Check exact type match
    auto it = TypeVars_.find(typeStr);
    if (it != TypeVars_.end()) {
        if (it->second.count(name)) return true;
    }
    
    // Check inheritance
    std::string currentType = typeStr;
    while (TypeHierarchy_.count(currentType)) {
        currentType = TypeHierarchy_.at(currentType);
        auto parentIt = TypeVars_.find(currentType);
        if (parentIt != TypeVars_.end()) {
            if (parentIt->second.count(name)) return true;
        }
    }
    
    return false;
}

bool DMBuiltinRegistry::IsContextVariable(const std::string& name) const {
    return ContextVars_.count(name);
}

std::optional<DMValueType> DMBuiltinRegistry::GetVarType(const DreamPath& type, const std::string& name) const {
    std::string typeStr = type.ToString();
    
    // Check exact type match
    auto it = TypeVars_.find(typeStr);
    if (it != TypeVars_.end()) {
        auto varIt = it->second.find(name);
        if (varIt != it->second.end()) {
            return varIt->second;
        }
    }
    
    // Check inheritance
    std::string currentType = typeStr;
    while (TypeHierarchy_.count(currentType)) {
        currentType = TypeHierarchy_.at(currentType);
        auto parentIt = TypeVars_.find(currentType);
        if (parentIt != TypeVars_.end()) {
            auto varIt = parentIt->second.find(name);
            if (varIt != parentIt->second.end()) {
                return varIt->second;
            }
        }
    }
    
    return std::nullopt;
}

bool DMBuiltinRegistry::IsBuiltinType(const DreamPath& type) const {
    return BuiltinTypes_.count(type.ToString());
}

bool DMBuiltinRegistry::TypeInheritsFrom(const DreamPath& derived, const DreamPath& base) const {
    std::string derivedStr = derived.ToString();
    std::string baseStr = base.ToString();
    
    if (derivedStr == baseStr) return true;
    
    std::string current = derivedStr;
    while (TypeHierarchy_.count(current)) {
        current = TypeHierarchy_.at(current);
        if (current == baseStr) return true;
    }
    
    return false;
}

void DMBuiltinRegistry::RegisterGlobalProcs() {
    // Text procs
    GlobalProcs_["findtext"] = ProcSignature("findtext", {"haystack", "needle", "Start", "End"}, DMValueType::Num);
    GlobalProcs_["findtextEx"] = ProcSignature("findtextEx", {"haystack", "needle", "Start", "End"}, DMValueType::Num);
    GlobalProcs_["copytext"] = ProcSignature("copytext", {"T", "Start", "End"}, DMValueType::Text);
    GlobalProcs_["splittext"] = ProcSignature("splittext", {"Text", "Delimiter"}, DMValueType::Anything);
    GlobalProcs_["length"] = ProcSignature("length", {"E"}, DMValueType::Num);
    GlobalProcs_["text2num"] = ProcSignature("text2num", {"T", "radix"}, DMValueType::Num);
    GlobalProcs_["num2text"] = ProcSignature("num2text", {"N", "sigfig", "grouping"}, DMValueType::Text);
    GlobalProcs_["text2ascii"] = ProcSignature("text2ascii", {"T", "pos"}, DMValueType::Num);
    GlobalProcs_["ascii2text"] = ProcSignature("ascii2text", {"N"}, DMValueType::Text);
    GlobalProcs_["uppertext"] = ProcSignature("uppertext", {"T"}, DMValueType::Text);
    GlobalProcs_["lowertext"] = ProcSignature("lowertext", {"T"}, DMValueType::Text);
    GlobalProcs_["ckey"] = ProcSignature("ckey", {"Key"}, DMValueType::Text);
    GlobalProcs_["sorttext"] = ProcSignature("sorttext", {"T1", "T2"}, DMValueType::Num);
    GlobalProcs_["sorttextEx"] = ProcSignature("sorttextEx", {"T1", "T2"}, DMValueType::Num);
    
    // File procs
    GlobalProcs_["file2text"] = ProcSignature("file2text", {"File"}, DMValueType::Text);
    GlobalProcs_["text2file"] = ProcSignature("text2file", {"Text", "File"}, DMValueType::Num);
    GlobalProcs_["fexists"] = ProcSignature("fexists", {"File"}, DMValueType::Num);
    GlobalProcs_["fcopy"] = ProcSignature("fcopy", {"Src", "Dst"}, DMValueType::Num);
    GlobalProcs_["fdel"] = ProcSignature("fdel", {"File"}, DMValueType::Num);
    GlobalProcs_["file"] = ProcSignature("file", {"Path"}, DMValueType::Anything); // Returns file resource
    
    // Math procs
    GlobalProcs_["sin"] = ProcSignature("sin", {"X"}, DMValueType::Num);
    GlobalProcs_["cos"] = ProcSignature("cos", {"X"}, DMValueType::Num);
    GlobalProcs_["tan"] = ProcSignature("tan", {"X"}, DMValueType::Num);
    GlobalProcs_["asin"] = ProcSignature("asin", {"X"}, DMValueType::Num);
    GlobalProcs_["acos"] = ProcSignature("acos", {"X"}, DMValueType::Num);
    GlobalProcs_["atan"] = ProcSignature("atan", {"X"}, DMValueType::Num);
    GlobalProcs_["sqrt"] = ProcSignature("sqrt", {"A"}, DMValueType::Num);
    GlobalProcs_["abs"] = ProcSignature("abs", {"A"}, DMValueType::Num);
    GlobalProcs_["min"] = ProcSignature("min", {"A"}, DMValueType::Anything, true); // Variadic
    GlobalProcs_["max"] = ProcSignature("max", {"A"}, DMValueType::Anything, true); // Variadic
    GlobalProcs_["round"] = ProcSignature("round", {"A", "B"}, DMValueType::Num);
    GlobalProcs_["rand"] = ProcSignature("rand", {"L", "H"}, DMValueType::Num);
    GlobalProcs_["prob"] = ProcSignature("prob", {"P"}, DMValueType::Num);
    GlobalProcs_["roll"] = ProcSignature("roll", {"dice"}, DMValueType::Num);
    GlobalProcs_["clamp"] = ProcSignature("clamp", {"Value", "Low", "High"}, DMValueType::Num);
    
    // Misc procs
    GlobalProcs_["sleep"] = ProcSignature("sleep", {"Delay"}, DMValueType::Null);
    GlobalProcs_["spawn"] = ProcSignature("spawn", {"Delay"}, DMValueType::Null);
    GlobalProcs_["del"] = ProcSignature("del", {"O"}, DMValueType::Null); // Note: del is a statement but also a proc
    GlobalProcs_["locate"] = ProcSignature("locate", {"X", "Y", "Z"}, DMValueType::Anything);
    GlobalProcs_["block"] = ProcSignature("block", {"Start", "End"}, DMValueType::Anything);
    GlobalProcs_["oview"] = ProcSignature("oview", {"Dist", "Center"}, DMValueType::Anything);
    GlobalProcs_["view"] = ProcSignature("view", {"Dist", "Center"}, DMValueType::Anything);
    GlobalProcs_["orange"] = ProcSignature("orange", {"Dist", "Center"}, DMValueType::Anything);
    GlobalProcs_["range"] = ProcSignature("range", {"Dist", "Center"}, DMValueType::Anything);
    GlobalProcs_["istype"] = ProcSignature("istype", {"Object", "Type"}, DMValueType::Num, true);
    GlobalProcs_["isnull"] = ProcSignature("isnull", {"Val"}, DMValueType::Num);
    GlobalProcs_["isnum"] = ProcSignature("isnum", {"Val"}, DMValueType::Num);
    GlobalProcs_["ispath"] = ProcSignature("ispath", {"Val"}, DMValueType::Num);
    GlobalProcs_["istext"] = ProcSignature("istext", {"Val"}, DMValueType::Num);
    GlobalProcs_["isloc"] = ProcSignature("isloc", {"Val"}, DMValueType::Num);
    GlobalProcs_["isicon"] = ProcSignature("isicon", {"Val"}, DMValueType::Num);
    GlobalProcs_["isfile"] = ProcSignature("isfile", {"Val"}, DMValueType::Num);
    GlobalProcs_["newlist"] = ProcSignature("newlist", {"..."}, DMValueType::Anything, true);
    GlobalProcs_["typesof"] = ProcSignature("typesof", {"TypePath"}, DMValueType::Anything);
    GlobalProcs_["params2list"] = ProcSignature("params2list", {"Params"}, DMValueType::Anything);
    GlobalProcs_["list2params"] = ProcSignature("list2params", {"List"}, DMValueType::Text);
    GlobalProcs_["html_encode"] = ProcSignature("html_encode", {"T"}, DMValueType::Text);
    GlobalProcs_["html_decode"] = ProcSignature("html_decode", {"T"}, DMValueType::Text);
    GlobalProcs_["url_encode"] = ProcSignature("url_encode", {"T"}, DMValueType::Text);
    GlobalProcs_["url_decode"] = ProcSignature("url_decode", {"T"}, DMValueType::Text);
    GlobalProcs_["time2text"] = ProcSignature("time2text", {"timestamp", "format"}, DMValueType::Text);
    GlobalProcs_["rgb"] = ProcSignature("rgb", {"r", "g", "b", "a"}, DMValueType::Text);
    GlobalProcs_["hascall"] = ProcSignature("hascall", {"Object", "ProcName"}, DMValueType::Num);
    GlobalProcs_["call"] = ProcSignature("call", {"Object", "ProcName"}, DMValueType::Anything);
    GlobalProcs_["initial"] = ProcSignature("initial", {"Var"}, DMValueType::Anything);
    GlobalProcs_["issaved"] = ProcSignature("issaved", {"Var"}, DMValueType::Num);
    GlobalProcs_["input"] = ProcSignature("input", {"..."}, DMValueType::Anything, true);
    GlobalProcs_["alert"] = ProcSignature("alert", {"..."}, DMValueType::Anything, true);
    GlobalProcs_["shutdown"] = ProcSignature("shutdown", {"Addr", "Reason"}, DMValueType::Null);
    GlobalProcs_["startup"] = ProcSignature("startup", {"Port", "Addr"}, DMValueType::Null);
}

void DMBuiltinRegistry::RegisterTypeProcs() {
    // /list procs
    RegisterProcForType("/list", ProcSignature("Add", {"Item"}, DMValueType::Anything, true));
    RegisterProcForType("/list", ProcSignature("Remove", {"Item"}, DMValueType::Anything, true));
    RegisterProcForType("/list", ProcSignature("Cut", {"Start", "End"}, DMValueType::Anything));
    RegisterProcForType("/list", ProcSignature("Copy", {"Start", "End"}, DMValueType::Anything));
    RegisterProcForType("/list", ProcSignature("Insert", {"Index", "Item"}, DMValueType::Anything, true));
    RegisterProcForType("/list", ProcSignature("Swap", {"Index1", "Index2"}, DMValueType::Anything));
    RegisterProcForType("/list", ProcSignature("Find", {"Elem", "Start", "End"}, DMValueType::Num));
    RegisterProcForType("/list", ProcSignature("Join", {"Glue", "Start", "End"}, DMValueType::Text));
    
    // /atom procs
    RegisterProcForType("/atom", ProcSignature("New", {"Loc"}, DMValueType::Null));
    RegisterProcForType("/atom", ProcSignature("Del", {}, DMValueType::Null));
    RegisterProcForType("/atom", ProcSignature("Move", {"Loc", "Dir"}, DMValueType::Num));
    RegisterProcForType("/atom", ProcSignature("Bump", {"Obstacle"}, DMValueType::Null));
    RegisterProcForType("/atom", ProcSignature("Enter", {"O", "OldLoc"}, DMValueType::Num));
    RegisterProcForType("/atom", ProcSignature("Exit", {"O", "NewLoc"}, DMValueType::Num));
    RegisterProcForType("/atom", ProcSignature("Entered", {"O", "OldLoc"}, DMValueType::Null));
    RegisterProcForType("/atom", ProcSignature("Exited", {"O", "NewLoc"}, DMValueType::Null));
    RegisterProcForType("/atom", ProcSignature("Click", {"Location", "Control", "Params"}, DMValueType::Null));
    RegisterProcForType("/atom", ProcSignature("DblClick", {"Location", "Control", "Params"}, DMValueType::Null));
    RegisterProcForType("/atom", ProcSignature("MouseDrag", {"Over", "SrcLocation", "OverLocation", "SrcControl", "OverControl", "Params"}, DMValueType::Null));
    RegisterProcForType("/atom", ProcSignature("MouseDown", {"Location", "Control", "Params"}, DMValueType::Null));
    RegisterProcForType("/atom", ProcSignature("MouseUp", {"Location", "Control", "Params"}, DMValueType::Null));
    RegisterProcForType("/atom", ProcSignature("MouseDrop", {"Over", "SrcLocation", "OverLocation", "SrcControl", "OverControl", "Params"}, DMValueType::Null));
    RegisterProcForType("/atom", ProcSignature("MouseEntered", {"Location", "Control", "Params"}, DMValueType::Null));
    RegisterProcForType("/atom", ProcSignature("MouseExited", {"Location", "Control", "Params"}, DMValueType::Null));
    RegisterProcForType("/atom", ProcSignature("Stat", {}, DMValueType::Null));
    
    // /world procs
    RegisterProcForType("/world", ProcSignature("New", {}, DMValueType::Null));
    RegisterProcForType("/world", ProcSignature("Del", {}, DMValueType::Null));
    RegisterProcForType("/world", ProcSignature("Reboot", {}, DMValueType::Null));
    RegisterProcForType("/world", ProcSignature("Topic", {"Topic", "Addr", "Master"}, DMValueType::Anything));
    RegisterProcForType("/world", ProcSignature("Repop", {}, DMValueType::Null));
    RegisterProcForType("/world", ProcSignature("Export", {"Addr", "File", "Data"}, DMValueType::Anything));
    RegisterProcForType("/world", ProcSignature("Import", {}, DMValueType::Anything));
    RegisterProcForType("/world", ProcSignature("Profile", {"Command", "Type", "Format"}, DMValueType::Anything));
    RegisterProcForType("/world", ProcSignature("GetConfig", {"Key"}, DMValueType::Anything));
    RegisterProcForType("/world", ProcSignature("SetConfig", {"Key", "Value"}, DMValueType::Anything));
    RegisterProcForType("/world", ProcSignature("OpenPort", {"Port"}, DMValueType::Num));
    
    // /client procs
    RegisterProcForType("/client", ProcSignature("New", {"Topic"}, DMValueType::Null));
    RegisterProcForType("/client", ProcSignature("Del", {}, DMValueType::Null));
    RegisterProcForType("/client", ProcSignature("Topic", {"Topic", "Addr", "Master"}, DMValueType::Anything));
    RegisterProcForType("/client", ProcSignature("Stat", {}, DMValueType::Null));
    RegisterProcForType("/client", ProcSignature("Click", {"Object", "Location", "Control", "Params"}, DMValueType::Null));
    RegisterProcForType("/client", ProcSignature("DblClick", {"Object", "Location", "Control", "Params"}, DMValueType::Null));
    RegisterProcForType("/client", ProcSignature("MouseDown", {"Object", "Location", "Control", "Params"}, DMValueType::Null));
    RegisterProcForType("/client", ProcSignature("MouseUp", {"Object", "Location", "Control", "Params"}, DMValueType::Null));
    RegisterProcForType("/client", ProcSignature("MouseDrag", {"SrcObject", "OverObject", "SrcLocation", "OverLocation", "SrcControl", "OverControl", "Params"}, DMValueType::Null));
    RegisterProcForType("/client", ProcSignature("MouseDrop", {"SrcObject", "OverObject", "SrcLocation", "OverLocation", "SrcControl", "OverControl", "Params"}, DMValueType::Null));
    RegisterProcForType("/client", ProcSignature("MouseEntered", {"Object", "Location", "Control", "Params"}, DMValueType::Null));
    RegisterProcForType("/client", ProcSignature("MouseExited", {"Object", "Location", "Control", "Params"}, DMValueType::Null));
    RegisterProcForType("/client", ProcSignature("Move", {"Loc", "Dir"}, DMValueType::Num));
    RegisterProcForType("/client", ProcSignature("North", {}, DMValueType::Null));
    RegisterProcForType("/client", ProcSignature("South", {}, DMValueType::Null));
    RegisterProcForType("/client", ProcSignature("East", {}, DMValueType::Null));
    RegisterProcForType("/client", ProcSignature("West", {}, DMValueType::Null));
    RegisterProcForType("/client", ProcSignature("Northeast", {}, DMValueType::Null));
    RegisterProcForType("/client", ProcSignature("Northwest", {}, DMValueType::Null));
    RegisterProcForType("/client", ProcSignature("Southeast", {}, DMValueType::Null));
    RegisterProcForType("/client", ProcSignature("Southwest", {}, DMValueType::Null));
    RegisterProcForType("/client", ProcSignature("Center", {}, DMValueType::Null));
    RegisterProcForType("/client", ProcSignature("Import", {}, DMValueType::Anything));
    RegisterProcForType("/client", ProcSignature("Export", {"File"}, DMValueType::Anything));
    
    // /datum procs
    RegisterProcForType("/datum", ProcSignature("New", {"..."}, DMValueType::Null, true));
    RegisterProcForType("/datum", ProcSignature("Del", {}, DMValueType::Null));
    RegisterProcForType("/datum", ProcSignature("Topic", {"Topic", "Addr", "Master"}, DMValueType::Anything));
    RegisterProcForType("/datum", ProcSignature("Read", {"File"}, DMValueType::Anything));
    RegisterProcForType("/datum", ProcSignature("Write", {"File"}, DMValueType::Anything));
    
    // /savefile procs
    RegisterProcForType("/savefile", ProcSignature("New", {"File", "Timeout"}, DMValueType::Null));
    RegisterProcForType("/savefile", ProcSignature("Del", {}, DMValueType::Null));
    RegisterProcForType("/savefile", ProcSignature("Flush", {}, DMValueType::Null));
    RegisterProcForType("/savefile", ProcSignature("ExportText", {"Path", "File"}, DMValueType::Text));
    RegisterProcForType("/savefile", ProcSignature("ImportText", {"Path", "Text"}, DMValueType::Null));
    
    // /sound procs
    RegisterProcForType("/sound", ProcSignature("New", {"File", "Repeat", "Wait", "Channel", "Volume"}, DMValueType::Null));
    
    // /image procs
    RegisterProcForType("/image", ProcSignature("New", {"Icon", "Loc", "IconState", "Layer", "Dir"}, DMValueType::Null));
    
    // /icon procs
    RegisterProcForType("/icon", ProcSignature("New", {"File", "IconState", "Dir", "Frame", "Moving"}, DMValueType::Null));
    RegisterProcForType("/icon", ProcSignature("IconStates", {"Mode"}, DMValueType::Anything));
    RegisterProcForType("/icon", ProcSignature("Turn", {"Angle"}, DMValueType::Anything));
    RegisterProcForType("/icon", ProcSignature("Flip", {"Dir"}, DMValueType::Anything));
    RegisterProcForType("/icon", ProcSignature("MapColors", {"..."}, DMValueType::Anything, true));
    RegisterProcForType("/icon", ProcSignature("Scale", {"Width", "Height"}, DMValueType::Anything));
    RegisterProcForType("/icon", ProcSignature("Crop", {"X1", "Y1", "X2", "Y2"}, DMValueType::Anything));
    RegisterProcForType("/icon", ProcSignature("Shift", {"Dir", "Offset", "Wrap"}, DMValueType::Anything));
    RegisterProcForType("/icon", ProcSignature("Blend", {"Icon", "Function", "X", "Y"}, DMValueType::Anything));
    RegisterProcForType("/icon", ProcSignature("Insert", {"Icon", "IconState", "Dir", "Frame", "Moving", "Delay"}, DMValueType::Anything));
    RegisterProcForType("/icon", ProcSignature("DrawBox", {"Color", "X1", "Y1", "X2", "Y2"}, DMValueType::Anything));
    RegisterProcForType("/icon", ProcSignature("SetIntensity", {"Intensity"}, DMValueType::Anything));
    RegisterProcForType("/icon", ProcSignature("SwapColor", {"Old", "New"}, DMValueType::Anything));
    RegisterProcForType("/icon", ProcSignature("Width", {}, DMValueType::Num));
    RegisterProcForType("/icon", ProcSignature("Height", {}, DMValueType::Num));
    
    // /matrix procs
    RegisterProcForType("/matrix", ProcSignature("New", {"..."}, DMValueType::Null, true));
    RegisterProcForType("/matrix", ProcSignature("Invert", {}, DMValueType::Num));
    RegisterProcForType("/matrix", ProcSignature("Multiply", {"Matrix"}, DMValueType::Anything));
    RegisterProcForType("/matrix", ProcSignature("Add", {"Matrix"}, DMValueType::Anything));
    RegisterProcForType("/matrix", ProcSignature("Subtract", {"Matrix"}, DMValueType::Anything));
    RegisterProcForType("/matrix", ProcSignature("Scale", {"X", "Y"}, DMValueType::Anything));
    RegisterProcForType("/matrix", ProcSignature("Translate", {"X", "Y"}, DMValueType::Anything));
    RegisterProcForType("/matrix", ProcSignature("Turn", {"Angle"}, DMValueType::Anything));
    RegisterProcForType("/matrix", ProcSignature("Interpolate", {"Matrix", "Time"}, DMValueType::Anything));
    
    // /regex procs
    RegisterProcForType("/regex", ProcSignature("New", {"Pattern", "Flags"}, DMValueType::Null));
    RegisterProcForType("/regex", ProcSignature("Find", {"Text", "Start"}, DMValueType::Num));
    RegisterProcForType("/regex", ProcSignature("Replace", {"Text", "Replacement", "Start"}, DMValueType::Text));
}

void DMBuiltinRegistry::RegisterBuiltinVars() {
    // /world variables
    RegisterVarForType("/world", "maxx", DMValueType::Num);
    RegisterVarForType("/world", "maxy", DMValueType::Num);
    RegisterVarForType("/world", "maxz", DMValueType::Num);
    RegisterVarForType("/world", "name", DMValueType::Text);
    RegisterVarForType("/world", "mob", DMValueType::Path);
    RegisterVarForType("/world", "turf", DMValueType::Path);
    RegisterVarForType("/world", "area", DMValueType::Path);
    RegisterVarForType("/world", "time", DMValueType::Num);
    RegisterVarForType("/world", "realtime", DMValueType::Num);
    RegisterVarForType("/world", "tick_lag", DMValueType::Num);
    RegisterVarForType("/world", "fps", DMValueType::Num);
    RegisterVarForType("/world", "icon_size", DMValueType::Num);
    RegisterVarForType("/world", "view", DMValueType::Num);
    RegisterVarForType("/world", "contents", DMValueType::Anything);
    RegisterVarForType("/world", "log", DMValueType::Anything);

    // /atom variables
    RegisterVarForType("/atom", "x", DMValueType::Num);
    RegisterVarForType("/atom", "y", DMValueType::Num);
    RegisterVarForType("/atom", "z", DMValueType::Num);
    RegisterVarForType("/atom", "loc", DMValueType::Anything);
    RegisterVarForType("/atom", "dir", DMValueType::Num);
    RegisterVarForType("/atom", "icon", DMValueType::Anything);
    RegisterVarForType("/atom", "icon_state", DMValueType::Text);
    RegisterVarForType("/atom", "name", DMValueType::Text);
    RegisterVarForType("/atom", "desc", DMValueType::Text);
    RegisterVarForType("/atom", "layer", DMValueType::Num);
    RegisterVarForType("/atom", "density", DMValueType::Num);
    RegisterVarForType("/atom", "opacity", DMValueType::Num);
    RegisterVarForType("/atom", "contents", DMValueType::Anything);
    RegisterVarForType("/atom", "vars", DMValueType::Anything);
    RegisterVarForType("/atom", "type", DMValueType::Path);
    RegisterVarForType("/atom", "parent_type", DMValueType::Path);

    // /mob variables
    RegisterVarForType("/mob", "key", DMValueType::Text);
    RegisterVarForType("/mob", "ckey", DMValueType::Text);
    RegisterVarForType("/mob", "client", DMValueType::Anything);
    RegisterVarForType("/mob", "see_invisible", DMValueType::Num);
    RegisterVarForType("/mob", "sight", DMValueType::Num);

    // /client variables
    RegisterVarForType("/client", "mob", DMValueType::Anything);
    RegisterVarForType("/client", "eye", DMValueType::Anything);
    RegisterVarForType("/client", "view", DMValueType::Anything);
    RegisterVarForType("/client", "screen", DMValueType::Anything);
    RegisterVarForType("/client", "images", DMValueType::Anything);
    RegisterVarForType("/client", "lazy_eye", DMValueType::Num);
    RegisterVarForType("/client", "dir", DMValueType::Num);

    // /datum variables
    RegisterVarForType("/datum", "type", DMValueType::Path);
    RegisterVarForType("/datum", "parent_type", DMValueType::Path);
    RegisterVarForType("/datum", "vars", DMValueType::Anything);
    RegisterVarForType("/datum", "tag", DMValueType::Text);

    // /list variables
    RegisterVarForType("/list", "len", DMValueType::Num);
}

void DMBuiltinRegistry::RegisterProcForType(const std::string& typePath, const ProcSignature& sig) {
    TypeProcs_[typePath][sig.Name] = sig;
}

void DMBuiltinRegistry::RegisterVarForType(const std::string& typePath, const std::string& name, DMValueType type) {
    TypeVars_[typePath][name] = type;
}

} // namespace DMCompiler
