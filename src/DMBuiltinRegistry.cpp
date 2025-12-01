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
        "/client", "/world", "/list", "/savefile", "/sound", "/image", "/icon", "/matrix", "/regex", "/exception",
        "/database", "/database/query", "/generator", "/mutable_appearance", "/particles", "/pixloc", "/vector"
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
    TypeHierarchy_["/database"] = "/datum";
    TypeHierarchy_["/database/query"] = "/datum";
    TypeHierarchy_["/generator"] = "/datum";
    TypeHierarchy_["/mutable_appearance"] = "/image";
    TypeHierarchy_["/particles"] = "/datum";
    TypeHierarchy_["/pixloc"] = "/datum";
    TypeHierarchy_["/vector"] = "/datum";

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
    GlobalProcs_["cmptext"] = ProcSignature("cmptext", {"T1"}, DMValueType::Num, true);
    GlobalProcs_["cmptextEx"] = ProcSignature("cmptextEx", {"T1"}, DMValueType::Num, true);
    GlobalProcs_["copytext_char"] = ProcSignature("copytext_char", {"T", "Start", "End"}, DMValueType::Text);
    GlobalProcs_["findlasttext"] = ProcSignature("findlasttext", {"Haystack", "Needle", "Start", "End"}, DMValueType::Num);
    GlobalProcs_["findlasttextEx"] = ProcSignature("findlasttextEx", {"Haystack", "Needle", "Start", "End"}, DMValueType::Num);
    GlobalProcs_["replacetext"] = ProcSignature("replacetext", {"Haystack", "Needle", "Replacement", "Start", "End"}, DMValueType::Text);
    GlobalProcs_["replacetextEx"] = ProcSignature("replacetextEx", {"Haystack", "Needle", "Replacement", "Start", "End"}, DMValueType::Text);
    GlobalProcs_["spantext"] = ProcSignature("spantext", {"Haystack", "Needles", "Start"}, DMValueType::Num);
    GlobalProcs_["nonspantext"] = ProcSignature("nonspantext", {"Haystack", "Needles", "Start"}, DMValueType::Num);
    GlobalProcs_["splicetext"] = ProcSignature("splicetext", {"Text", "Start", "End", "Insert"}, DMValueType::Text);
    GlobalProcs_["trimtext"] = ProcSignature("trimtext", {"Text"}, DMValueType::Text);
    GlobalProcs_["md5"] = ProcSignature("md5", {"T"}, DMValueType::Text);
    GlobalProcs_["sha1"] = ProcSignature("sha1", {"T"}, DMValueType::Text);
    GlobalProcs_["json_encode"] = ProcSignature("json_encode", {"Value"}, DMValueType::Text);
    GlobalProcs_["json_decode"] = ProcSignature("json_decode", {"JSON"}, DMValueType::Anything);
    GlobalProcs_["regex"] = ProcSignature("regex", {"pattern", "flags"}, DMValueType::Anything);
    
    // File procs
    GlobalProcs_["file2text"] = ProcSignature("file2text", {"File"}, DMValueType::Text);
    GlobalProcs_["text2file"] = ProcSignature("text2file", {"Text", "File"}, DMValueType::Num);
    GlobalProcs_["fexists"] = ProcSignature("fexists", {"File"}, DMValueType::Num);
    GlobalProcs_["fcopy"] = ProcSignature("fcopy", {"Src", "Dst"}, DMValueType::Num);
    GlobalProcs_["fdel"] = ProcSignature("fdel", {"File"}, DMValueType::Num);
    GlobalProcs_["file"] = ProcSignature("file", {"Path"}, DMValueType::Anything); // Returns file resource
    GlobalProcs_["browse"] = ProcSignature("browse", {"Body", "Options"}, DMValueType::Null);
    GlobalProcs_["browse_rsc"] = ProcSignature("browse_rsc", {"File", "FileName"}, DMValueType::Null);
    GlobalProcs_["fcopy_rsc"] = ProcSignature("fcopy_rsc", {"File"}, DMValueType::Num);
    GlobalProcs_["ftp"] = ProcSignature("ftp", {"File", "Name"}, DMValueType::Null);
    GlobalProcs_["load_resource"] = ProcSignature("load_resource", {"File"}, DMValueType::Anything);
    GlobalProcs_["run"] = ProcSignature("run", {"File"}, DMValueType::Null);
    GlobalProcs_["shell"] = ProcSignature("shell", {"Command"}, DMValueType::Null);
    
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
    GlobalProcs_["ceil"] = ProcSignature("ceil", {"A"}, DMValueType::Num);
    GlobalProcs_["floor"] = ProcSignature("floor", {"A"}, DMValueType::Num);
    GlobalProcs_["fract"] = ProcSignature("fract", {"n"}, DMValueType::Num);
    GlobalProcs_["trunc"] = ProcSignature("trunc", {"n"}, DMValueType::Num);
    GlobalProcs_["sign"] = ProcSignature("sign", {"A"}, DMValueType::Num);
    GlobalProcs_["log"] = ProcSignature("log", {"X", "Base"}, DMValueType::Num);
    GlobalProcs_["matrix"] = ProcSignature("matrix", {"..."}, DMValueType::Anything, true);
    GlobalProcs_["vector"] = ProcSignature("vector", {"..."}, DMValueType::Anything, true);
    GlobalProcs_["gradient"] = ProcSignature("gradient", {"A", "index"}, DMValueType::Anything);
    GlobalProcs_["lerp"] = ProcSignature("lerp", {"A", "B", "factor"}, DMValueType::Anything);
    GlobalProcs_["turn"] = ProcSignature("turn", {"Dir", "Angle"}, DMValueType::Num);
    
    // Misc procs
    GlobalProcs_["sleep"] = ProcSignature("sleep", {"Delay"}, DMValueType::Null);
    GlobalProcs_["spawn"] = ProcSignature("spawn", {"Delay"}, DMValueType::Null);
    GlobalProcs_["del"] = ProcSignature("del", {"O"}, DMValueType::Null); // Note: del is a statement but also a proc
    GlobalProcs_["arglist"] = ProcSignature("arglist", {"args"}, DMValueType::Anything);
    GlobalProcs_["missile"] = ProcSignature("missile", {"Type", "Start", "End"}, DMValueType::Null);
    GlobalProcs_["CRASH"] = ProcSignature("CRASH", {"msg"}, DMValueType::Null);
    GlobalProcs_["EXCEPTION"] = ProcSignature("EXCEPTION", {"message"}, DMValueType::Anything);
    GlobalProcs_["animate"] = ProcSignature("animate", {"Object", "time", "loop", "easing", "flags"}, DMValueType::Null);
    GlobalProcs_["flick"] = ProcSignature("flick", {"Icon", "Object"}, DMValueType::Null);
    GlobalProcs_["image"] = ProcSignature("image", {"icon", "loc", "icon_state", "layer", "dir"}, DMValueType::Anything);
    GlobalProcs_["icon"] = ProcSignature("icon", {"icon", "icon_state", "dir", "frame", "moving"}, DMValueType::Anything);
    GlobalProcs_["icon_states"] = ProcSignature("icon_states", {"Icon", "mode"}, DMValueType::Anything);
    GlobalProcs_["locate"] = ProcSignature("locate", {"X", "Y", "Z"}, DMValueType::Anything);
    GlobalProcs_["block"] = ProcSignature("block", {"Start", "End"}, DMValueType::Anything);
    GlobalProcs_["oview"] = ProcSignature("oview", {"Dist", "Center"}, DMValueType::Anything);
    GlobalProcs_["view"] = ProcSignature("view", {"Dist", "Center"}, DMValueType::Anything);
    GlobalProcs_["orange"] = ProcSignature("orange", {"Dist", "Center"}, DMValueType::Anything);
    GlobalProcs_["range"] = ProcSignature("range", {"Dist", "Center"}, DMValueType::Anything);
    GlobalProcs_["hearers"] = ProcSignature("hearers", {"Depth", "Center"}, DMValueType::Anything);
    GlobalProcs_["ohearers"] = ProcSignature("ohearers", {"Depth", "Center"}, DMValueType::Anything);
    GlobalProcs_["viewers"] = ProcSignature("viewers", {"Depth", "Center"}, DMValueType::Anything);
    GlobalProcs_["oviewers"] = ProcSignature("oviewers", {"Depth", "Center"}, DMValueType::Anything);
    GlobalProcs_["istype"] = ProcSignature("istype", {"Object", "Type"}, DMValueType::Num, true);
    GlobalProcs_["isnull"] = ProcSignature("isnull", {"Val"}, DMValueType::Num);
    GlobalProcs_["isnum"] = ProcSignature("isnum", {"Val"}, DMValueType::Num);
    GlobalProcs_["ispath"] = ProcSignature("ispath", {"Val"}, DMValueType::Num);
    GlobalProcs_["istext"] = ProcSignature("istext", {"Val"}, DMValueType::Num);
    GlobalProcs_["isloc"] = ProcSignature("isloc", {"Val"}, DMValueType::Num);
    GlobalProcs_["isicon"] = ProcSignature("isicon", {"Val"}, DMValueType::Num);
    GlobalProcs_["isarea"] = ProcSignature("isarea", {"Val"}, DMValueType::Num);
    GlobalProcs_["ismob"] = ProcSignature("ismob", {"Val"}, DMValueType::Num);
    GlobalProcs_["isobj"] = ProcSignature("isobj", {"Val"}, DMValueType::Num);
    GlobalProcs_["isturf"] = ProcSignature("isturf", {"Val"}, DMValueType::Num);
    GlobalProcs_["ismovable"] = ProcSignature("ismovable", {"Val"}, DMValueType::Num);
    GlobalProcs_["islist"] = ProcSignature("islist", {"Val"}, DMValueType::Num);
    GlobalProcs_["isinf"] = ProcSignature("isinf", {"n"}, DMValueType::Num);
    GlobalProcs_["isnan"] = ProcSignature("isnan", {"n"}, DMValueType::Num);
    GlobalProcs_["ispointer"] = ProcSignature("ispointer", {"Value"}, DMValueType::Num);
    GlobalProcs_["isfile"] = ProcSignature("isfile", {"Val"}, DMValueType::Num);
    GlobalProcs_["newlist"] = ProcSignature("newlist", {"..."}, DMValueType::Anything, true);
    GlobalProcs_["typesof"] = ProcSignature("typesof", {"TypePath"}, DMValueType::Anything);
    GlobalProcs_["params2list"] = ProcSignature("params2list", {"Params"}, DMValueType::Anything);
    GlobalProcs_["list2params"] = ProcSignature("list2params", {"List"}, DMValueType::Text);
    GlobalProcs_["text2path"] = ProcSignature("text2path", {"T"}, DMValueType::Path);
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
    GlobalProcs_["call_ext"] = ProcSignature("call_ext", {"LibName", "FuncName"}, DMValueType::Anything);
    GlobalProcs_["load_ext"] = ProcSignature("load_ext", {"LibName", "FuncName"}, DMValueType::Anything);
    GlobalProcs_["generator"] = ProcSignature("generator", {"type", "A", "B", "rand"}, DMValueType::Anything);
    GlobalProcs_["get_dir"] = ProcSignature("get_dir", {"Loc1", "Loc2"}, DMValueType::Num);
    GlobalProcs_["get_dist"] = ProcSignature("get_dist", {"Loc1", "Loc2"}, DMValueType::Num);
    GlobalProcs_["get_step"] = ProcSignature("get_step", {"Ref", "Dir"}, DMValueType::Anything);
    GlobalProcs_["get_step_away"] = ProcSignature("get_step_away", {"Ref", "Trg", "Max"}, DMValueType::Anything);
    GlobalProcs_["get_step_rand"] = ProcSignature("get_step_rand", {"Ref"}, DMValueType::Anything);
    GlobalProcs_["get_step_to"] = ProcSignature("get_step_to", {"Ref", "Trg", "Min"}, DMValueType::Anything);
    GlobalProcs_["get_step_towards"] = ProcSignature("get_step_towards", {"Ref", "Trg"}, DMValueType::Anything);
    GlobalProcs_["get_steps_to"] = ProcSignature("get_steps_to", {"Ref", "Trg", "Min"}, DMValueType::Anything);
    GlobalProcs_["link"] = ProcSignature("link", {"url"}, DMValueType::Null);
    GlobalProcs_["output"] = ProcSignature("output", {"Msg", "Control"}, DMValueType::Null);
    GlobalProcs_["pick"] = ProcSignature("pick", {"..."}, DMValueType::Anything, true);
    GlobalProcs_["rand_seed"] = ProcSignature("rand_seed", {"Seed"}, DMValueType::Null);
    GlobalProcs_["ref"] = ProcSignature("ref", {"Object"}, DMValueType::Text);
    GlobalProcs_["refcount"] = ProcSignature("refcount", {"Object"}, DMValueType::Num);
    GlobalProcs_["sound"] = ProcSignature("sound", {"file", "repeat", "wait", "channel", "volume"}, DMValueType::Anything);
    GlobalProcs_["stat"] = ProcSignature("stat", {"Name", "Value"}, DMValueType::Null);
    GlobalProcs_["statpanel"] = ProcSignature("statpanel", {"Panel", "Name", "Value"}, DMValueType::Null);
    GlobalProcs_["step"] = ProcSignature("step", {"Ref", "Dir", "Speed"}, DMValueType::Num);
    GlobalProcs_["step_away"] = ProcSignature("step_away", {"Ref", "Trg", "Max", "Speed"}, DMValueType::Num);
    GlobalProcs_["step_rand"] = ProcSignature("step_rand", {"Ref", "Speed"}, DMValueType::Num);
    GlobalProcs_["step_to"] = ProcSignature("step_to", {"Ref", "Trg", "Min", "Speed"}, DMValueType::Num);
    GlobalProcs_["step_towards"] = ProcSignature("step_towards", {"Ref", "Trg", "Speed"}, DMValueType::Num);
    GlobalProcs_["walk"] = ProcSignature("walk", {"Ref", "Dir", "Lag", "Speed"}, DMValueType::Null);
    GlobalProcs_["walk_away"] = ProcSignature("walk_away", {"Ref", "Trg", "Max", "Lag", "Speed"}, DMValueType::Null);
    GlobalProcs_["walk_rand"] = ProcSignature("walk_rand", {"Ref", "Lag", "Speed"}, DMValueType::Null);
    GlobalProcs_["walk_to"] = ProcSignature("walk_to", {"Ref", "Trg", "Min", "Lag", "Speed"}, DMValueType::Null);
    GlobalProcs_["walk_towards"] = ProcSignature("walk_towards", {"Ref", "Trg", "Lag", "Speed"}, DMValueType::Null);
    GlobalProcs_["winset"] = ProcSignature("winset", {"player", "control_id", "params"}, DMValueType::Null);
    GlobalProcs_["winget"] = ProcSignature("winget", {"player", "control_id", "params"}, DMValueType::Text);
    GlobalProcs_["winclone"] = ProcSignature("winclone", {"player", "window_name", "clone_name"}, DMValueType::Null);
    GlobalProcs_["winexists"] = ProcSignature("winexists", {"player", "control_id"}, DMValueType::Num);
    GlobalProcs_["winshow"] = ProcSignature("winshow", {"player", "window", "show"}, DMValueType::Null);
    GlobalProcs_["shutdown"] = ProcSignature("shutdown", {"Addr", "Natural"}, DMValueType::Null);
    GlobalProcs_["startup"] = ProcSignature("startup", {"Port", "Addr"}, DMValueType::Null);
    GlobalProcs_["set_background"] = ProcSignature("set_background", {"mode"}, DMValueType::Null);
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
    RegisterProcForType("/world", ProcSignature("IsBanned", {"Key", "Address", "ComputerID", "Type"}, DMValueType::Num));
    RegisterProcForType("/world", ProcSignature("Tick", {}, DMValueType::Null));
    RegisterProcForType("/world", ProcSignature("Error", {"Exception"}, DMValueType::Null));
    
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
    RegisterProcForType("/client", ProcSignature("MouseMove", {"Object", "Location", "Control", "Params"}, DMValueType::Null));
    RegisterProcForType("/client", ProcSignature("MouseWheel", {"Object", "DeltaX", "DeltaY", "Location", "Control", "Params"}, DMValueType::Null));
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
    RegisterProcForType("/client", ProcSignature("Command", {"Command"}, DMValueType::Null));
    RegisterProcForType("/client", ProcSignature("AllowUpload", {"Filename", "Filelength"}, DMValueType::Num));
    RegisterProcForType("/client", ProcSignature("SoundQuery", {}, DMValueType::Num));
    RegisterProcForType("/client", ProcSignature("MeasureText", {"Text", "Style", "Width"}, DMValueType::Anything));
    RegisterProcForType("/client", ProcSignature("RenderIcon", {"Object"}, DMValueType::Anything));
    
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

    // /database procs
    RegisterProcForType("/database", ProcSignature("Open", {"File"}, DMValueType::Num));
    RegisterProcForType("/database", ProcSignature("Close", {}, DMValueType::Null));
    RegisterProcForType("/database", ProcSignature("Error", {}, DMValueType::Num));
    RegisterProcForType("/database", ProcSignature("ErrorMsg", {}, DMValueType::Text));

    // /database/query procs
    RegisterProcForType("/database/query", ProcSignature("New", {"Query", "Cursor"}, DMValueType::Null));
    RegisterProcForType("/database/query", ProcSignature("Add", {"Text", "..."}, DMValueType::Null, true));
    RegisterProcForType("/database/query", ProcSignature("Execute", {"Database"}, DMValueType::Num));
    RegisterProcForType("/database/query", ProcSignature("NextRow", {}, DMValueType::Num));
    RegisterProcForType("/database/query", ProcSignature("GetRowData", {}, DMValueType::Anything));
    RegisterProcForType("/database/query", ProcSignature("GetColumn", {"Column"}, DMValueType::Anything));
    RegisterProcForType("/database/query", ProcSignature("RowsAffected", {}, DMValueType::Num));
    RegisterProcForType("/database/query", ProcSignature("Close", {}, DMValueType::Null));
    RegisterProcForType("/database/query", ProcSignature("Error", {}, DMValueType::Num));
    RegisterProcForType("/database/query", ProcSignature("ErrorMsg", {}, DMValueType::Text));
    RegisterProcForType("/database/query", ProcSignature("Columns", {"Column"}, DMValueType::Anything));
    RegisterProcForType("/database/query", ProcSignature("Clear", {}, DMValueType::Null));
    RegisterProcForType("/database/query", ProcSignature("Reset", {}, DMValueType::Null));

    // /generator procs
    RegisterProcForType("/generator", ProcSignature("Rand", {}, DMValueType::Num));
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
    RegisterVarForType("/world", "cpu", DMValueType::Num);
    RegisterVarForType("/world", "loop_checks", DMValueType::Num);
    RegisterVarForType("/world", "movement_mode", DMValueType::Num);
    RegisterVarForType("/world", "internet_address", DMValueType::Text);
    RegisterVarForType("/world", "url", DMValueType::Text);
    RegisterVarForType("/world", "visibility", DMValueType::Num);
    RegisterVarForType("/world", "status", DMValueType::Text);
    RegisterVarForType("/world", "map_cpu", DMValueType::Num);
    RegisterVarForType("/world", "map_format", DMValueType::Num);
    RegisterVarForType("/world", "reachable", DMValueType::Num);
    RegisterVarForType("/world", "byond_version", DMValueType::Num);
    RegisterVarForType("/world", "byond_build", DMValueType::Num);
    RegisterVarForType("/world", "address", DMValueType::Text);
    RegisterVarForType("/world", "port", DMValueType::Num);
    RegisterVarForType("/world", "system_type", DMValueType::Num);

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
    RegisterVarForType("/client", "statobj", DMValueType::Anything);
    RegisterVarForType("/client", "statpanel", DMValueType::Text);
    RegisterVarForType("/client", "edge_limit", DMValueType::Anything);
    RegisterVarForType("/client", "pixel_x", DMValueType::Num);
    RegisterVarForType("/client", "pixel_y", DMValueType::Num);
    RegisterVarForType("/client", "pixel_z", DMValueType::Num);
    RegisterVarForType("/client", "pixel_w", DMValueType::Num);
    RegisterVarForType("/client", "show_verb_panel", DMValueType::Num);
    RegisterVarForType("/client", "authenticate", DMValueType::Num);
    RegisterVarForType("/client", "CGI", DMValueType::Anything);
    RegisterVarForType("/client", "command_text", DMValueType::Text);
    RegisterVarForType("/client", "inactivity", DMValueType::Num);
    RegisterVarForType("/client", "tick_lag", DMValueType::Num);
    RegisterVarForType("/client", "show_map", DMValueType::Num);
    RegisterVarForType("/client", "script", DMValueType::Anything);
    RegisterVarForType("/client", "color", DMValueType::Anything);
    RegisterVarForType("/client", "control_freak", DMValueType::Num);
    RegisterVarForType("/client", "mouse_pointer_icon", DMValueType::Anything);
    RegisterVarForType("/client", "preload_rsc", DMValueType::Num);
    RegisterVarForType("/client", "fps", DMValueType::Num);
    RegisterVarForType("/client", "glide_size", DMValueType::Num);
    RegisterVarForType("/client", "virtual_eye", DMValueType::Anything);
    RegisterVarForType("/client", "bounds", DMValueType::Anything);
    RegisterVarForType("/client", "bound_x", DMValueType::Num);
    RegisterVarForType("/client", "bound_y", DMValueType::Num);
    RegisterVarForType("/client", "bound_width", DMValueType::Num);
    RegisterVarForType("/client", "bound_height", DMValueType::Num);

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
