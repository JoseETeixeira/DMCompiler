#!/usr/bin/env python3
"""
SCons build configuration for DMCompiler.

Usage:
    scons                    # Build all (release mode)
    scons --mode=debug       # Build with debug symbols
    scons --no-tests         # Skip building tests
    scons --no-disassembler  # Skip building disassembler
    scons -c                 # Clean build artifacts
    scons test               # Build and run tests

Cross-platform: Works on Windows (MSVC) and Linux (GCC/Clang)
"""

import os
import sys
import platform
import shutil
import urllib.request
import ssl

# =============================================================================
# Configuration
# =============================================================================

BUILD_DIR = 'build'
NLOHMANN_JSON_URL = 'https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp'
NLOHMANN_JSON_PATH = os.path.join('third_party', 'nlohmann', 'json.hpp')

# =============================================================================
# Platform Detection
# =============================================================================

def detect_platform():
    """
    Detect the current platform and return configuration.
    
    Returns a dictionary with:
        - platform: 'windows' or 'linux'
        - compiler: 'msvc' or 'gcc'
        - exe_suffix: '.exe' or ''
        - lib_prefix: '' or 'lib'
        - lib_suffix: '.lib' or '.a'
    """
    system = platform.system()
    
    if system == 'Windows':
        return {
            'platform': 'windows',
            'compiler': 'msvc',
            'exe_suffix': '.exe',
            'lib_prefix': '',
            'lib_suffix': '.lib',
            'static_lib_suffix': '.lib'
        }
    elif system == 'Linux':
        return {
            'platform': 'linux',
            'compiler': 'gcc',
            'exe_suffix': '',
            'lib_prefix': 'lib',
            'lib_suffix': '.a',
            'static_lib_suffix': '.a'
        }
    else:
        print(f"WARNING: Unsupported platform '{system}', attempting Linux-like build")
        return {
            'platform': 'linux',
            'compiler': 'gcc',
            'exe_suffix': '',
            'lib_prefix': 'lib',
            'lib_suffix': '.a',
            'static_lib_suffix': '.a'
        }

# Detect platform at module load time
PLATFORM_CONFIG = detect_platform()

# Print platform info
print(f"Platform: {PLATFORM_CONFIG['platform']}")
print(f"Compiler: {PLATFORM_CONFIG['compiler']}")

# =============================================================================
# Source Files
# =============================================================================

# Source files for the compiler library (everything except main entry points)
LIB_SOURCES = [
    'src/DMCompiler.cpp',
    'src/Lexer.cpp',
    'src/Token.cpp',
    'src/Location.cpp',
    'src/DMPreprocessor.cpp',
    'src/DMBuiltinRegistry.cpp',
    'src/DMLexer.cpp',
    'src/DMAST.cpp',
    'src/DMASTFolder.cpp',
    'src/DMASTExpression.cpp',
    'src/DMParser.cpp',
    'src/DMObject.cpp',
    'src/DMVariable.cpp',
    'src/DMValueType.cpp',
    'src/DMObjectTree.cpp',
    'src/DMCodeTree.cpp',
    'src/DMCodeTreeBuilder.cpp',
    'src/DMProc.cpp',
    'src/DMExpression.cpp',
    'src/DreamPath.cpp',
    'src/BytecodeEmitter.cpp',
    'src/BytecodeWriter.cpp',
    'src/DMExpressionCompiler.cpp',
    'src/DMStatementCompiler.cpp',
    'src/OpcodeDefinitions.cpp',
    'src/DMMParser.cpp',
    'src/JsonOutput.cpp',
    'src/JsonWriter.cpp',
]

# =============================================================================
# Command-Line Options
# =============================================================================

AddOption('--mode',
    dest='mode',
    type='string',
    default='release',
    help='Build mode: debug or release (default: release)')

AddOption('--no-tests',
    dest='build_tests',
    action='store_false',
    default=True,
    help='Disable building tests')

AddOption('--no-disassembler',
    dest='build_disassembler',
    action='store_false',
    default=True,
    help='Disable building the disassembler')

# Retrieve options
build_mode = GetOption('mode')
build_tests = GetOption('build_tests')
build_disassembler = GetOption('build_disassembler')

# Validate mode option
if build_mode not in ('debug', 'release'):
    print(f"ERROR: Invalid mode '{build_mode}'. Must be 'debug' or 'release'.")
    Exit(1)

# Print build configuration
print(f"Build mode: {build_mode}")
print(f"Build tests: {build_tests}")
print(f"Build disassembler: {build_disassembler}")

# =============================================================================
# Environment Configuration
# =============================================================================

# Create the build environment based on platform
if PLATFORM_CONFIG['platform'] == 'windows':
    # Windows: Use MSVC toolchain
    env = Environment(tools=['msvc', 'mslink', 'mslib'])
    
    # Base C++17 flags for MSVC
    env.Append(CCFLAGS=[
        '/std:c++17',   # C++17 standard
        '/EHsc',        # Exception handling
        '/FS',          # Force synchronous PDB writes (parallel build support)
        '/W3',          # Warning level 3
    ])
    
    # Preprocessor definitions
    env.Append(CPPDEFINES=[
        '_CRT_SECURE_NO_WARNINGS',  # Disable CRT security warnings
        'WIN32',
        '_WINDOWS',
    ])
    
    # Debug vs Release configuration
    if build_mode == 'debug':
        env.Append(CCFLAGS=[
            '/Od',      # Disable optimization
            '/Zi',      # Debug information
            '/MDd',     # Multi-threaded debug DLL runtime
            '/RTC1',    # Runtime error checks
        ])
        env.Append(LINKFLAGS=['/DEBUG'])
    else:
        env.Append(CCFLAGS=[
            '/O2',      # Optimize for speed
            '/MD',      # Multi-threaded DLL runtime
        ])
        env.Append(CPPDEFINES=['NDEBUG'])

else:
    # Linux/Unix: Use GCC/Clang toolchain
    env = Environment()
    
    # Base C++17 flags for GCC/Clang
    env.Append(CCFLAGS=[
        '-std=c++17',   # C++17 standard
        '-Wall',        # All warnings
        '-Wextra',      # Extra warnings
        '-Wpedantic',   # Strict ISO compliance warnings
    ])
    
    # Debug vs Release configuration
    if build_mode == 'debug':
        env.Append(CCFLAGS=[
            '-O0',      # No optimization
            '-g',       # Debug symbols
        ])
    else:
        env.Append(CCFLAGS=[
            '-O2',      # Optimize for speed
        ])
        env.Append(CPPDEFINES=['NDEBUG'])

# Common include paths (both platforms)
env.Append(CPPPATH=[
    'include',      # Project headers
    'third_party',  # Third-party dependencies (nlohmann_json)
])

# Ensure build directory exists
os.makedirs(BUILD_DIR, exist_ok=True)

# =============================================================================
# Dependency Management
# =============================================================================

def ensure_nlohmann_json():
    """
    Ensure nlohmann_json dependency is available.
    
    Downloads json.hpp from GitHub releases if not present.
    Returns True on success, False on failure.
    """
    if os.path.exists(NLOHMANN_JSON_PATH):
        print(f"Found dependency: {NLOHMANN_JSON_PATH}")
        return True
    
    print(f"Downloading nlohmann_json v3.11.3...")
    
    # Create directory if it doesn't exist
    json_dir = os.path.dirname(NLOHMANN_JSON_PATH)
    os.makedirs(json_dir, exist_ok=True)
    
    try:
        # Create SSL context for HTTPS
        context = ssl.create_default_context()
        
        # Download the file
        with urllib.request.urlopen(NLOHMANN_JSON_URL, context=context, timeout=30) as response:
            data = response.read()
        
        # Write to file
        with open(NLOHMANN_JSON_PATH, 'wb') as f:
            f.write(data)
        
        print(f"Downloaded: {NLOHMANN_JSON_PATH} ({len(data)} bytes)")
        return True
        
    except urllib.error.URLError as e:
        print(f"ERROR: Failed to download nlohmann_json: {e}")
        print(f"URL: {NLOHMANN_JSON_URL}")
        print("Please download manually and place at:")
        print(f"  {NLOHMANN_JSON_PATH}")
        return False
        
    except IOError as e:
        print(f"ERROR: Failed to write nlohmann_json: {e}")
        print(f"Path: {NLOHMANN_JSON_PATH}")
        return False
        
    except Exception as e:
        print(f"ERROR: Unexpected error downloading nlohmann_json: {e}")
        return False

# Check and download dependencies
if not ensure_nlohmann_json():
    print("ERROR: Required dependency not available. Build cannot proceed.")
    Exit(1)

# =============================================================================
# DMStandard Deployment
# =============================================================================

def copy_dmstandard(target, source, env):
    """
    Post-build action to copy DMStandard folder to build directory.
    
    This ensures the compiled executables can locate the standard library
    definitions at runtime. The DMStandard folder is required for the
    compiler to function properly.
    
    Args:
        target: SCons target (unused but required by post-action signature)
        source: SCons source (unused but required by post-action signature)
        env: SCons environment (unused but required by post-action signature)
    """
    src_dir = 'DMStandard'
    dst_dir = os.path.join(BUILD_DIR, 'DMStandard')
    
    # Check if source DMStandard folder exists
    if not os.path.exists(src_dir):
        print(f"WARNING: {src_dir}/ folder not found, skipping deployment")
        return
    
    print(f"Deploying DMStandard to {dst_dir}...")
    
    try:
        # Remove existing destination to ensure fresh copy
        # (Requirement 7.5: re-copy on clean build)
        if os.path.exists(dst_dir):
            shutil.rmtree(dst_dir)
        
        # Copy entire directory tree preserving structure
        # (Requirements 7.1, 7.3: copy entire folder, preserve structure)
        shutil.copytree(src_dir, dst_dir)
        
        print(f"Successfully deployed DMStandard to build directory")
        
    except shutil.Error as e:
        # Handle errors for individual files but continue
        # (Requirement 7.4: report error but continue)
        print(f"WARNING: Some files failed to copy: {e}")
        
    except OSError as e:
        # Handle OS-level errors (permissions, disk space, etc.)
        print(f"WARNING: Failed to deploy DMStandard: {e}")
        
    except Exception as e:
        # Catch any unexpected errors
        print(f"WARNING: Unexpected error deploying DMStandard: {e}")

# =============================================================================
# Build Targets
# =============================================================================

# DMCompiler - Static library containing the compiler core
# This is linked by both dmcompiler and dmdisasm executables
lib = env.StaticLibrary(
    target=os.path.join(BUILD_DIR, 'DMCompiler'),
    source=LIB_SOURCES
)

# dmcompiler - Main compiler executable
# Compiles DM source files into bytecode/JSON for OpenDream runtime
dmcompiler = env.Program(
    target=os.path.join(BUILD_DIR, 'dmcompiler'),
    source=['src/main.cpp'],
    LIBS=[lib]
)

# Set dmcompiler as the default build target
Default(dmcompiler)

# Register DMStandard deployment as post-build action
# This ensures DMStandard is copied after dmcompiler is successfully built
env.AddPostAction(dmcompiler, copy_dmstandard)

# dmdisasm - Bytecode disassembler executable (optional)
# Disassembles compiled bytecode for debugging and inspection
if build_disassembler:
    dmdisasm = env.Program(
        target=os.path.join(BUILD_DIR, 'dmdisasm'),
        source=[
            'src/disassembler_main.cpp',
            'src/DMDisassembler.cpp',
        ],
        LIBS=[lib]
    )
    # Add to default targets so it builds alongside dmcompiler
    Default(dmdisasm)

# =============================================================================
# Clean Target Configuration
# =============================================================================

# Register the entire build directory for cleaning with 'scons -c'
# This ensures all build artifacts are removed:
# - Object files (.obj, .o)
# - Static libraries (DMCompiler.lib, libDMCompiler.a)  
# - Executables (dmcompiler, dmdisasm, test executables)
# - Copied DMStandard folder
# - Copied test data directories
#
# Requirements 10.1-10.4:
# - Removes all generated files from build/
# - Removes copied DMStandard folder
# - Does NOT remove source files or third_party/ dependencies
Clean('.', BUILD_DIR)

# =============================================================================
# Test Targets
# =============================================================================

# Variable to hold test targets for the test runner alias
dm_compiler_tests = None
standalone_test_targets = {}

if build_tests:
    # Create test environment with src/ in include path
    # Tests may include internal headers from src/
    test_env = env.Clone()
    test_env.Append(CPPPATH=['src'])
    
    # Ensure test output directory exists
    test_dir = os.path.join(BUILD_DIR, 'tests')
    os.makedirs(test_dir, exist_ok=True)
    
    # Main test suite source files
    TEST_MAIN_SOURCES = [
        'tests/test_lexer.cpp',
        'tests/test_preprocessor.cpp',
        'tests/test_parser.cpp',
        'tests/test_compiler.cpp',
        'tests/test_objecttree.cpp',
        'tests/test_proc_simple.cpp',
        'tests/test_proc_parameter_registration.cpp',
        'tests/test_var_block.cpp',
        'tests/test_var_block_parsing.cpp',
        'tests/test_bytecode.cpp',
        'tests/test_expression_compiler.cpp',
        'tests/test_statement_compiler.cpp',
        'tests/test_dmmparser.cpp',
        'tests/test_goto_forward_ref.cpp',
        'tests/example_compilation_tests.cpp',
        'tests/test_main.cpp',
    ]
    
    # Standalone test executables (name -> source file)
    STANDALONE_TESTS = {
        'test_dmvaluetype': 'tests/test_dmvaluetype.cpp',
        'test_path_resolution': 'tests/test_path_resolution.cpp',
        'test_builtin_registry': 'tests/test_builtin_registry.cpp',
        'test_istype_variadic': 'tests/test_istype_variadic.cpp',
        'test_type_resolution_driver': 'tests/test_type_resolution_driver.cpp',
        'test_redeclaration_driver': 'tests/test_redeclaration_driver.cpp',
        'test_continue_in_switch_driver': 'tests/test_continue_in_switch_driver.cpp',
        'test_stack_safety_driver': 'tests/test_stack_safety_driver.cpp',
        'test_proc_resolution': 'tests/test_proc_resolution.cpp',
        'test_var_override': 'tests/test_var_override.cpp',
        'test_var_attributes': 'tests/test_var_attributes.cpp',
        'test_lexer_tokens': 'tests/test_lexer_tokens.cpp',
        'test_bytecode_writer': 'tests/test_bytecode_writer.cpp',
    }
    
    # Build main test suite
    dm_compiler_tests = test_env.Program(
        target=os.path.join(test_dir, 'dm_compiler_tests'),
        source=TEST_MAIN_SOURCES,
        LIBS=[lib]
    )
    
    # Build standalone test executables
    for test_name, source_file in STANDALONE_TESTS.items():
        standalone_test_targets[test_name] = test_env.Program(
            target=os.path.join(test_dir, test_name),
            source=[source_file],
            LIBS=[lib]
        )
    
    # Copy test data directories to build/tests/
    def copy_test_data(target, source, env):
        """Copy test data directories to the test output directory."""
        for data_dir in ['testdata', 'test_files']:
            src = os.path.join('tests', data_dir)
            dst = os.path.join(test_dir, data_dir)
            
            if not os.path.exists(src):
                print(f"WARNING: Test data directory '{src}' not found")
                continue
            
            try:
                if os.path.exists(dst):
                    shutil.rmtree(dst)
                shutil.copytree(src, dst)
                print(f"Copied {src} to {dst}")
            except Exception as e:
                print(f"WARNING: Failed to copy {src}: {e}")
    
    # Register test data copy as post-build action on main test suite
    test_env.AddPostAction(dm_compiler_tests, copy_test_data)
    
    print(f"Test targets configured: dm_compiler_tests + {len(STANDALONE_TESTS)} standalone tests")

# =============================================================================
# Test Runner Alias
# =============================================================================

def run_tests(target, source, env):
    """
    Execute all test executables and report results.
    
    Runs the main test suite first, then standalone tests.
    Reports which tests failed and exits with non-zero code on failure.
    
    Args:
        target: SCons target (unused but required by action signature)
        source: SCons source (unused but required by action signature)
        env: SCons environment (unused but required by action signature)
    """
    import subprocess
    
    test_dir = os.path.join(BUILD_DIR, 'tests')
    exe_suffix = PLATFORM_CONFIG['exe_suffix']
    
    failed_tests = []
    passed_tests = []
    
    # List of tests to run in order
    # Main test suite first, then standalone tests
    tests_to_run = ['dm_compiler_tests']
    
    # Add standalone tests that should be run
    # These are the tests registered with CTest in CMakeLists.txt
    standalone_to_run = [
        'test_builtin_registry',
        'test_istype_variadic',
        'test_type_resolution_driver',
        'test_proc_resolution',
        'test_var_override',
        'test_var_attributes',
        'test_lexer_tokens',
        'test_bytecode_writer',
    ]
    tests_to_run.extend(standalone_to_run)
    
    print("\n" + "=" * 60)
    print("RUNNING TESTS")
    print("=" * 60)
    
    for test_name in tests_to_run:
        test_exe = os.path.join(test_dir, f'{test_name}{exe_suffix}')
        
        if not os.path.exists(test_exe):
            print(f"\nWARNING: Test executable not found: {test_exe}")
            continue
        
        print(f"\n--- Running {test_name} ---")
        
        try:
            # Run test from the test directory so it can find test data
            result = subprocess.run(
                [test_exe],
                cwd=test_dir,
                timeout=120  # 2 minute timeout per test
            )
            
            if result.returncode == 0:
                print(f"PASSED: {test_name}")
                passed_tests.append(test_name)
            else:
                print(f"FAILED: {test_name} (exit code: {result.returncode})")
                failed_tests.append(test_name)
                
        except subprocess.TimeoutExpired:
            print(f"TIMEOUT: {test_name} (exceeded 120 seconds)")
            failed_tests.append(test_name)
            
        except Exception as e:
            print(f"ERROR: {test_name} - {e}")
            failed_tests.append(test_name)
    
    # Print summary
    print("\n" + "=" * 60)
    print("TEST SUMMARY")
    print("=" * 60)
    print(f"Passed: {len(passed_tests)}")
    print(f"Failed: {len(failed_tests)}")
    
    if failed_tests:
        print(f"\nFAILED TESTS:")
        for test in failed_tests:
            print(f"  - {test}")
        print("\n=== TESTS FAILED ===")
        Exit(1)
    else:
        print("\n=== ALL TESTS PASSED ===")

# Register test alias - runs when user executes 'scons test'
if build_tests and dm_compiler_tests is not None:
    # Collect all test targets as dependencies
    all_test_targets = [dm_compiler_tests] + list(standalone_test_targets.values())
    
    # Create 'test' alias that depends on all test executables
    # and runs the test runner function
    test_alias = env.Alias('test', all_test_targets, run_tests)
    
    # AlwaysBuild ensures the test runner runs every time 'scons test' is invoked
    # even if the test executables haven't changed
    AlwaysBuild(test_alias)

print("SConstruct loaded successfully.")
