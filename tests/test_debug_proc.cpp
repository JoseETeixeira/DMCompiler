#include <iostream>
#include "../include/DMParser.h"
#include "../include/DMLexer.h"
#include "../include/DMCompiler.h"

int main() {
    const char* source = R"(proc/Calculate(x, y, z = 0)
    var/result = 0
    
    if (x > y)
        result = x + y
    else
        result = x - y
        
    for (var/i = 0; i < z; i++)
        result += i
        
    return result
)";
    
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMLexer lexer("test.dm", source);
    DMCompiler::DMParser parser(&compiler, &lexer);
    
    auto file = parser.ParseFile();
    
    std::cout << "Number of top-level statements: " << file->Statements.size() << std::endl;
    
    for (size_t i = 0; i < file->Statements.size(); i++) {
        std::cout << "Statement " << i << ": ";
        
        if (dynamic_cast<DMCompiler::DMASTObjectProcDefinition*>(file->Statements[i].get())) {
            auto* procDef = dynamic_cast<DMCompiler::DMASTObjectProcDefinition*>(file->Statements[i].get());
            std::cout << "ProcDefinition (name: " << procDef->Name << ", ";
            std::cout << "params: " << procDef->Parameters.size() << ", ";
            if (procDef->Body) {
                std::cout << "body statements: " << procDef->Body->Statements.size();
            } else {
                std::cout << "body: null";
            }
            std::cout << ")" << std::endl;
        } else if (dynamic_cast<DMCompiler::DMASTProcStatementVarDeclaration*>(file->Statements[i].get())) {
            std::cout << "VarDeclaration" << std::endl;
        } else {
            std::cout << "Other type" << std::endl;
        }
    }
    
    return 0;
}
