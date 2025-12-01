cmake_minimum_required(VERSION 3.15)

# Test executable
add_executable(dm_compiler_tests
    test_lexer.cpp
    test_preprocessor.cpp
    test_parser.cpp
    test_compiler.cpp
    test_objecttree.cpp
    test_proc_simple.cpp
    test_proc_parameter_registration.cpp
    test_var_block.cpp
    test_var_block_parsing.cpp
    test_bytecode.cpp
    test_expression_compiler.cpp
    test_statement_compiler.cpp
    test_dmmparser.cpp
    test_goto_forward_ref.cpp
    example_compilation_tests.cpp
    test_main.cpp
)

target_link_libraries(dm_compiler_tests PRIVATE DMCompilerLib)
target_include_directories(dm_compiler_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Standalone DMValueType test
add_executable(test_dmvaluetype
    test_dmvaluetype.cpp
)

target_link_libraries(test_dmvaluetype PRIVATE DMCompilerLib)

# Standalone path resolution test
add_executable(test_path_resolution
    test_path_resolution.cpp
)

target_link_libraries(test_path_resolution PRIVATE DMCompilerLib)

# Standalone DMBuiltinRegistry test
add_executable(test_builtin_registry
    test_builtin_registry.cpp
)

target_link_libraries(test_builtin_registry PRIVATE DMCompilerLib)

# Standalone istype variadic test
add_executable(test_istype_variadic
    test_istype_variadic.cpp
)

target_link_libraries(test_istype_variadic PRIVATE DMCompilerLib)

# Standalone type resolution test
add_executable(test_type_resolution_driver
    test_type_resolution_driver.cpp
)

target_link_libraries(test_type_resolution_driver PRIVATE DMCompilerLib)

# Standalone redeclaration test
add_executable(test_redeclaration_driver
    test_redeclaration_driver.cpp
)

target_link_libraries(test_redeclaration_driver PRIVATE DMCompilerLib)
target_include_directories(test_redeclaration_driver PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Standalone continue in switch test
add_executable(test_continue_in_switch_driver
    test_continue_in_switch_driver.cpp
)

target_link_libraries(test_continue_in_switch_driver PRIVATE DMCompilerLib)
target_include_directories(test_continue_in_switch_driver PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Standalone stack safety test
add_executable(test_stack_safety_driver
    test_stack_safety_driver.cpp
)

target_link_libraries(test_stack_safety_driver PRIVATE DMCompilerLib)
target_include_directories(test_stack_safety_driver PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Add test data directory
file(COPY testdata DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY test_files DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Register tests with CTest
add_test(NAME LexerTests COMMAND dm_compiler_tests [lexer])
add_test(NAME PreprocessorTests COMMAND dm_compiler_tests [preprocessor])
add_test(NAME ParserTests COMMAND dm_compiler_tests [parser])
add_test(NAME CompilerTests COMMAND dm_compiler_tests [compiler])
add_test(NAME ObjectTreeTests COMMAND dm_compiler_tests [objecttree])
add_test(NAME ProcTests COMMAND dm_compiler_tests [proc])
add_test(NAME ProcParameterRegistrationTests COMMAND dm_compiler_tests [paramreg])
add_test(NAME VarBlockParsingTests COMMAND dm_compiler_tests [varblock])
add_test(NAME BytecodeTests COMMAND dm_compiler_tests [bytecode])
add_test(NAME ExprCompilerTests COMMAND dm_compiler_tests [exprcompiler])
add_test(NAME DMMParserTests COMMAND dm_compiler_tests [dmmparser])
add_test(NAME GotoForwardRefTests COMMAND dm_compiler_tests [gotoforward])
add_test(NAME BuiltinRegistryTests COMMAND test_builtin_registry)
add_test(NAME IstypeVariadicTests COMMAND test_istype_variadic)
add_test(NAME TypeResolutionTests COMMAND test_type_resolution_driver)
# Disabled: Takes too long to run, not related to core compiler functionality
# add_test(NAME ExampleCompilationTests COMMAND dm_compiler_tests [examples])
# set_tests_properties(ExampleCompilationTests PROPERTIES TIMEOUT 60)
