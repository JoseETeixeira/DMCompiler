#include <iostream>
#include <string>

// Forward declarations
int RunLexerTests();
int RunParserTests();
int RunCompilerTests();
int RunPreprocessorTests();
void RunObjectTreeTests();
void RunProcTests();
void RunProcParameterRegistrationTests();
void RunVarBlockParsingTests();
int RunBytecodeTests();
int RunExpressionCompilerTests();
int RunStatementCompilerTests();
int RunDMMParserTests();
int RunGotoForwardRefTests();
int RunExampleCompilationTests();
int RunMultiLineVarBlockTests();

void PrintUsage() {
    std::cout << "Usage: dm_compiler_tests [filter]" << std::endl;
    std::cout << "Filters:" << std::endl;
    std::cout << "  [lexer]        - Run lexer tests only" << std::endl;
    std::cout << "  [parser]       - Run parser tests only" << std::endl;
    std::cout << "  [compiler]     - Run compiler tests only" << std::endl;
    std::cout << "  [preprocessor] - Run preprocessor tests only" << std::endl;
    std::cout << "  [objecttree]   - Run object tree tests only" << std::endl;
    std::cout << "  [proc]         - Run proc tests only" << std::endl;
    std::cout << "  [paramreg]     - Run proc parameter registration tests only" << std::endl;
    std::cout << "  [varblock]     - Run var block parsing tests only" << std::endl;
    std::cout << "  [bytecode]     - Run bytecode tests only" << std::endl;
    std::cout << "  [exprcompiler] - Run expression compiler tests only" << std::endl;
    std::cout << "  [dmmparser]    - Run DMM parser tests only" << std::endl;
    std::cout << "  [examples]     - Run example compilation tests only" << std::endl;
    std::cout << "  (no args)      - Run all tests" << std::endl;
}

int main(int argc, char** argv) {
    std::cout << "DMCompiler C++ Test Suite" << std::endl;
    std::cout << "=========================" << std::endl;
    
    std::string filter = (argc > 1) ? argv[1] : "";
    
    int failures = 0;
    
    if (filter.empty() || filter == "[lexer]") {
        failures += RunLexerTests();
    }
    
    if (filter.empty() || filter == "[preprocessor]") {
        failures += RunPreprocessorTests();
    }
    
    if (filter.empty() || filter == "[parser]") {
        failures += RunParserTests();
    }
    
    if (filter.empty() || filter == "[compiler]") {
        failures += RunCompilerTests();
    }
    
    if (filter.empty() || filter == "[objecttree]") {
        RunObjectTreeTests();
    }
    
    if (filter.empty() || filter == "[proc]") {
        RunProcTests();
    }
    
    if (filter.empty() || filter == "[paramreg]") {
        RunProcParameterRegistrationTests();
    }
    
    if (filter.empty() || filter == "[varblock]") {
        RunVarBlockParsingTests();
    }
    
    if (filter.empty() || filter == "[bytecode]") {
        failures += RunBytecodeTests();
    }
    
    if (filter.empty() || filter == "[exprcompiler]") {
        failures += RunExpressionCompilerTests();
    }
    
    if (filter.empty() || filter == "[stmtcompiler]") {
        failures += RunStatementCompilerTests();
    }
    
    if (filter.empty() || filter == "[dmmparser]") {
        failures += RunDMMParserTests();
    }
    
    if (filter.empty() || filter == "[gotoforward]") {
        failures += RunGotoForwardRefTests();
    }
    
    if (filter.empty() || filter == "[examples]") {
        failures += RunExampleCompilationTests();
    }

    if (filter.empty() || filter == "[multivar]") {
        failures += RunMultiLineVarBlockTests();
    }
    
    std::cout << "\n=========================" << std::endl;
    if (failures == 0) {
        std::cout << "All tests passed!" << std::endl;
        return 0;
    } else {
        std::cout << "Some tests failed!" << std::endl;
        return 1;
    }
}
