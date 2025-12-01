# DMCompiler

DMCompiler is a C++ implementation of a compiler for the DM (Dream Maker) language, . It compiles DM code (`.dme`, `.dm`) into a format compatible with the OpenDream runtime.

## Prerequisites

*   CMake 3.15 or higher
*   C++17 compatible compiler (e.g., MSVC, GCC, Clang)

## Building

1.  Clone the repository.
2.  Create a build directory:
    ```bash
    mkdir build
    cd build
    ```
3.  Generate build files with CMake:
    ```bash
    cmake ..
    ```
4.  Build the project:
    ```bash
    cmake --build . --config Release
    ```

The compiled executables (`dmcompiler` and `dmdisasm`) will be located in the `build/bin` directory (or `build/Release` depending on your platform/generator).

## Usage

### Compiler

To compile a DM project, run `dmcompiler` against your environment file (`.dme`) or a single `.dm` file.

```bash
./dmcompiler [options] <file>.dme
```

#### Options

*   `--help`: Show help message.
*   `--version [VER].[BUILD]`: Set the `DM_VERSION` and `DM_BUILD` macros.
*   `--skip-bad-args`: Skip arguments the compiler doesn't recognize.
*   `--suppress-unimplemented`: Do not warn about unimplemented proc and var uses.
*   `--suppress-unsupported`: Do not warn about proc and var uses that will not be supported.
*   `--dump-preprocessor`: Save the result of preprocessing to a file.
*   `--no-standard`: Disable the built-in standard library.
*   `--define [KEY=VAL]`: Add extra defines to the compilation.
*   `--verbose`: Show verbose output during compilation.
*   `--notices-enabled`: Show notice output during compilation.
*   `--no-opts`: Disable compiler optimizations (debug only).

### Disassembler

The project also includes a disassembler tool `dmdisasm` for inspecting the compiled JSON output.

```bash
./dmdisasm <file>.json [command]
```

#### Commands

*   `crash-on-test`: Test disassembly of the entire codebase (useful for CI).
*   `dump-all`: Dump all types and procs to stdout.

If no command is provided, the disassembler enters an interactive mode.
