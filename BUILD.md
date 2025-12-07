# Building br-ar with Autotools

## Prerequisites

- autoconf
- automake
- GNU make
- compatible C compiler (gcc, clang, etc.)

## Building

The build system uses GNU autotools. You must generate the build files first:

```bash
autoreconf -fiv
./configure
make
make check
make install
```

**Note**: The `autoreconf` step generates all the intermediate files (`configure`, `*.in` files, etc.) from the source templates (`configure.ac`, `Makefile.am`). These generated files are not included in the source repository.

## Configuration Options

### Tool Name

By default, the tool is installed as `br-ar`. You can change this by setting the `TOOL_NAME` variable:

```bash
./configure TOOL_NAME=my-tool
```

This will install the tool as `my-tool` instead of `br-ar`.

### Platform-Specific Endian Operations

By default, platform-specific endian operations are used for better performance on:
- Darwin/macOS (using `libkern/OSByteOrder.h`)
- FreeBSD (using `sys/endian.h`)
- OpenBSD (using `sys/endian.h`)
- Linux (using `endian.h`)

To use generic (portable) endian operations instead:

```bash
./configure --disable-platform-endian
```

### Symlinks and Wrappers

By default, the following are created during installation:
- `brarchive` - symlink to the main tool
- `brarchive-cli` - drop-in compatibility wrapper for the Rust crate interface

To disable:

```bash
./configure --disable-symlinks
```

The `brarchive-cli` wrapper is a POSIX-compliant shell script that provides the same interface as the Rust [brarchive-cli](https://crates.io/crates/brarchive-cli) crate:
- `brarchive-cli encode <folder> <archive>` - creates archive
- `brarchive-cli decode <archive> <folder>` - extracts archive
- `brarchive-cli help` - shows help message

The wrapper automatically finds the `br-ar` binary and translates commands appropriately.

## Cross-Platform Compatibility

The code has been designed for maximum portability across:
- Different architectures (x86, x86_64, ARM, etc.)
- Different kernels (Linux, BSD variants, Darwin)
- Different libc implementations (glibc, musl, BSD libc)

The build system automatically detects:
- Available headers
- Type sizes
- Endianness
- Platform-specific features

### POSIX Shell Compliance

All shell scripts in this project are POSIX-compliant:
- The `brarchive-cli` wrapper uses only POSIX shell features
- All test scripts use POSIX-compliant commands
- No bash-specific features or GNU extensions are used
- Scripts work with any POSIX-compliant shell (dash, ash, sh, etc.)

This ensures the tool works on minimal systems and embedded environments that may not have bash installed.

## Testing

### Running the Test Suite

The project includes a comprehensive test suite using `make check`:

```bash
# Run all tests
make check

# Run tests with verbose output
make check VERBOSE=1
```

The test suite includes:
- **test-list**: Tests listing archive contents (with and without filters)
- **test-extract**: Tests extracting files (all files and specific files)
- **test-print**: Tests printing file contents to stdout
- **test-create**: Tests creating archives and round-trip verification
- **test-combined-flags**: Tests combined flags
- **test-delete**: Tests deleting files from archives

All tests use the provided `recipes.brarchive` file as a test source.

**Note**: The built binary is named `br_ar` in the build directory, but it will be installed as `br-ar` (or the configured tool name) when you run `make install`.

