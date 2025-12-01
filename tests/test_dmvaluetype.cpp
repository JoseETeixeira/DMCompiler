#include "DMValueType.h"
#include <cassert>
#include <iostream>

using namespace DMCompiler;

void TestParseTypeFlags() {
    // Test single types
    assert(ParseTypeFlags("num") == DMValueType::Num);
    assert(ParseTypeFlags("text") == DMValueType::Text);
    assert(ParseTypeFlags("obj") == DMValueType::Obj);
    assert(ParseTypeFlags("mob") == DMValueType::Mob);
    assert(ParseTypeFlags("turf") == DMValueType::Turf);
    assert(ParseTypeFlags("area") == DMValueType::Area);
    assert(ParseTypeFlags("null") == DMValueType::Null);
    assert(ParseTypeFlags("message") == DMValueType::Message);
    assert(ParseTypeFlags("color") == DMValueType::Color);
    assert(ParseTypeFlags("file") == DMValueType::File);
    assert(ParseTypeFlags("command_text") == DMValueType::CommandText);
    assert(ParseTypeFlags("sound") == DMValueType::Sound);
    assert(ParseTypeFlags("icon") == DMValueType::Icon);
    assert(ParseTypeFlags("path") == DMValueType::Path);
    assert(ParseTypeFlags("anything") == DMValueType::Anything);
    
    // Test empty string
    assert(ParseTypeFlags("") == DMValueType::Anything);
    
    // Test case insensitivity
    assert(ParseTypeFlags("NUM") == DMValueType::Num);
    assert(ParseTypeFlags("Text") == DMValueType::Text);
    assert(ParseTypeFlags("OBJ") == DMValueType::Obj);
    
    // Test multiple types with |
    DMValueType numText = ParseTypeFlags("num|text");
    assert(HasFlag(numText, DMValueType::Num));
    assert(HasFlag(numText, DMValueType::Text));
    assert(!HasFlag(numText, DMValueType::Obj));
    
    DMValueType mobObj = ParseTypeFlags("mob|obj");
    assert(HasFlag(mobObj, DMValueType::Mob));
    assert(HasFlag(mobObj, DMValueType::Obj));
    assert(!HasFlag(mobObj, DMValueType::Num));
    
    // Test with whitespace
    DMValueType withSpaces = ParseTypeFlags(" num | text ");
    assert(HasFlag(withSpaces, DMValueType::Num));
    assert(HasFlag(withSpaces, DMValueType::Text));
    
    // Test three types
    DMValueType threeTy = ParseTypeFlags("num|text|obj");
    assert(HasFlag(threeTy, DMValueType::Num));
    assert(HasFlag(threeTy, DMValueType::Text));
    assert(HasFlag(threeTy, DMValueType::Obj));
    
    std::cout << "All ParseTypeFlags tests passed!" << std::endl;
}

void TestDMValueTypeToString() {
    // Test single types
    assert(DMValueTypeToString(DMValueType::Num) == "num");
    assert(DMValueTypeToString(DMValueType::Text) == "text");
    assert(DMValueTypeToString(DMValueType::Anything) == "anything");
    
    // Test combined types
    DMValueType combined = DMValueType::Num | DMValueType::Text;
    std::string str = DMValueTypeToString(combined);
    // Should contain both "num" and "text"
    assert(str.find("num") != std::string::npos);
    assert(str.find("text") != std::string::npos);
    
    std::cout << "All DMValueTypeToString tests passed!" << std::endl;
}

void TestHasFlag() {
    DMValueType flags = DMValueType::Num | DMValueType::Text;
    
    assert(HasFlag(flags, DMValueType::Num));
    assert(HasFlag(flags, DMValueType::Text));
    assert(!HasFlag(flags, DMValueType::Obj));
    assert(!HasFlag(flags, DMValueType::Mob));
    
    // Test with Anything
    assert(!HasFlag(DMValueType::Anything, DMValueType::Num));
    
    std::cout << "All HasFlag tests passed!" << std::endl;
}

int main() {
    try {
        TestParseTypeFlags();
        TestDMValueTypeToString();
        TestHasFlag();
        
        std::cout << "\nAll DMValueType tests passed successfully!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
