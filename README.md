# DMCompiler

DMCompiler is a C++ implementation of a compiler for the DM (Dream Maker) language. It compiles DM code (`.dme`, `.dm`) into a format compatible with the MYG runtime. This project is a migration and reimplementation of OpenDream's DM compiler, aiming for improved performance, maintainability, and compliance with the DM language specification.

## Prerequisites

*   **Python 3.6+** (required for SCons)
*   **SCons 4.0+** (build system)
*   **C++17 compatible compiler:**
    *   Windows: MSVC (Visual Studio 2019 or later)
    *   Linux: GCC 9+ or Clang 10+

### Installing SCons

**Windows (via pip):**
```powershell
pip install scons
```

**Linux (Debian/Ubuntu):**
```bash
sudo apt install scons
# Or via pip: pip install scons
```

**Linux (Fedora/RHEL):**
```bash
sudo dnf install scons
# Or via pip: pip install scons
```

## Building with SCons

SCons is the primary build system for DMCompiler. All build commands are run from the project root directory.

### Basic Build (Release Mode)

```bash
scons
```

This builds the `dmcompiler` executable (and `dmdisasm` by default) in release mode. Output is placed in the `build/` directory.

### Debug Build

```bash
scons --mode=debug
```

Builds with debug symbols and no optimization.

### Build Options

| Option | Description |
|--------|-------------|
| `--mode=debug` | Build with debug symbols (`/Od /Zi` on MSVC, `-O0 -g` on GCC) |
| `--mode=release` | Build with optimizations (default) |
| `--no-tests` | Skip building test executables |
| `--no-disassembler` | Skip building the `dmdisasm` tool |
| `--help` | Show all available options |

**Examples:**
```bash
# Release build without tests
scons --no-tests

# Debug build, compiler only (no tests, no disassembler)
scons --mode=debug --no-tests --no-disassembler
```

### Running Tests

```bash
scons test
```

This builds all test executables and runs them. Test results are reported with pass/fail status.

### Cleaning Build Artifacts

```bash
scons -c
```

Removes all generated files from the `build/` directory, including:
- Object files
- Static libraries
- Executables
- Copied `DMStandard/` folder
- Test data copies

**Note:** The clean operation does not remove source files or the `third_party/` directory.

### Parallel Builds

SCons automatically uses multiple CPU cores for parallel compilation. To limit parallelism:

```bash
scons -j4  # Use 4 parallel jobs
```

### Build Output

After a successful build, the `build/` directory contains:
- `dmcompiler.exe` (Windows) or `dmcompiler` (Linux) — Main compiler
- `dmdisasm.exe` / `dmdisasm` — Bytecode disassembler
- `DMStandard/` — Standard library (automatically copied)
- `DMCompiler.lib` / `libDMCompiler.a` — Static library
- `tests/` — Test executables and data (if tests enabled)

### Dependencies

The build system automatically downloads [nlohmann_json](https://github.com/nlohmann/json) (v3.11.3) on first build if not present. No manual dependency management is required.


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
