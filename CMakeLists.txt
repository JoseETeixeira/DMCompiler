cmake_minimum_required(VERSION 3.15)
project(DMCompilerCpp VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add /FS flag for MSVC to avoid PDB file conflicts during parallel compilation
if(MSVC)
    add_compile_options(/FS)
endif()

# Options
option(BUILD_TESTS "Build test suite" ON)
option(BUILD_DISASSEMBLER "Build disassembler tool" ON)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Third-party libraries
find_package(nlohmann_json 3.2.0 QUIET)
if(NOT nlohmann_json_FOUND)
    message(STATUS "nlohmann_json not found, downloading single-header version...")
    
    set(JSON_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/third_party/nlohmann)
    file(MAKE_DIRECTORY ${JSON_INCLUDE_DIR})
    
    if(NOT EXISTS ${JSON_INCLUDE_DIR}/json.hpp)
        file(DOWNLOAD 
            https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp
            ${JSON_INCLUDE_DIR}/json.hpp
            SHOW_PROGRESS
        )
    endif()
    
    # Create an interface library for consistency
    add_library(nlohmann_json INTERFACE)
    target_include_directories(nlohmann_json INTERFACE ${CMAKE_SOURCE_DIR}/third_party)
    add_library(nlohmann_json::nlohmann_json ALIAS nlohmann_json)
endif()

# Compiler library
add_library(DMCompilerLib STATIC
    src/DMCompiler.cpp
    src/Lexer.cpp
    src/Token.cpp
    src/Location.cpp
    src/DMPreprocessor.cpp
    src/DMBuiltinRegistry.cpp
    src/DMLexer.cpp
    src/DMAST.cpp
    src/DMASTFolder.cpp
    src/DMASTExpression.cpp
    src/DMParser.cpp
    src/DMObject.cpp
    src/DMVariable.cpp
    src/DMValueType.cpp
    src/DMObjectTree.cpp
    src/DMCodeTree.cpp
    src/DMCodeTreeBuilder.cpp
    src/DMProc.cpp
    src/DMExpression.cpp
    src/DreamPath.cpp
    src/BytecodeEmitter.cpp
    src/BytecodeWriter.cpp
    src/DMExpressionCompiler.cpp
    src/DMStatementCompiler.cpp
    src/OpcodeDefinitions.cpp
    src/DMMParser.cpp
    src/JsonOutput.cpp
    src/JsonWriter.cpp
)

target_include_directories(DMCompilerLib PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# Fix MSVC PDB file conflicts
if(MSVC)
    target_compile_options(DMCompilerLib PRIVATE /FS)
endif()

# Compiler executable
add_executable(dmcompiler
    src/main.cpp
)

target_link_libraries(dmcompiler PRIVATE DMCompilerLib)



# Disassembler executable
if(BUILD_DISASSEMBLER)
    add_executable(dmdisasm
        src/disassembler_main.cpp
        src/DMDisassembler.cpp
    )
    target_link_libraries(dmdisasm PRIVATE DMCompilerLib nlohmann_json::nlohmann_json)
endif()

# Tests
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Installation
install(TARGETS dmcompiler DESTINATION bin)
if(BUILD_DISASSEMBLER)
    install(TARGETS dmdisasm DESTINATION bin)
endif()
