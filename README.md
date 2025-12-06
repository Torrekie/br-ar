# br-ar - Bedrock Archive Tool

A lightweight C tool for creating, extracting, and listing `.brarchive` files, similar to the `ar` command.

### Why not [brarchive-cli](https://github.com/theaddonn/brarchive-rs)?

You will never want to see that hell, aren’t ya?

<details>
<summary>Be careful to expand this if you don't want a heartattack</summary>

```
$ cargo install brarchive-cli
    Updating crates.io index
  Downloaded brarchive-cli v0.1.1
  Downloaded 1 crate (8.2 KB) in 18.33s
  Installing brarchive-cli v0.1.1
    Updating crates.io index
  Downloaded autocfg v1.5.0
  Downloaded anstyle-query v1.1.5
  Downloaded is_terminal_polyfill v1.70.2
  Downloaded colorchoice v1.0.4
  Downloaded anstyle-parse v0.2.7
  Downloaded heck v0.5.0
  Downloaded clap_lex v0.7.60
  Downloaded utf8parse v0.2.2
  Downloaded strsim v0.11.1
  Downloaded anstyle v1.0.13
  Downloaded thiserror-impl v2.0.17
  Downloaded humantime v2.3.0
  Downloaded anstream v0.6.21
  Downloaded byteorder v1.5.0
  Downloaded iana-time-zone v0.1.64
  Downloaded thiserror v2.0.17
  Downloaded colored v2.2.0
  Downloaded brarchive v0.1.0
  Downloaded quote v1.0.42
  Downloaded clap_derive v4.5.49
  Downloaded unicode-ident v1.0.22
  Downloaded num-traits v0.2.19
  Downloaded proc-macro2 v1.0.103
  Downloaded log v0.4.29
  Downloaded clap v4.5.53
  Downloaded clap_builder v4.5.53
  Downloaded chrono v0.4.42
  Downloaded syn v2.0.111
  Downloaded fern v0.7.1
  Downloaded 30 crates (1.7 MB) in 3m 27s
   Compiling proc-macro2 v1.0.103
   Compiling quote v1.0.42
   Compiling unicode-ident v1.0.22
   Compiling autocfg v1.5.0
   Compiling syn v2.0.111
   Compiling utf8parse v0.2.2
   Compiling anstyle-parse v0.2.7
   Compiling num-traits v0.2.19
   Compiling anstyle v1.0.13
   Compiling anstyle-query v1.1.5
   Compiling is_terminal_polyfill v1.70.2
   Compiling colorchoice v1.0.4
   Compiling thiserror v2.0.17
   Compiling anstream v0.6.21
   Compiling clap_lex v0.7.6
   Compiling heck v0.5.0
   Compiling strsim v0.11.1
   Compiling lazy_static v1.5.0
   Compiling colored v2.2.0
   Compiling clap_builder v4.5.53
   Compiling thiserror-impl v2.0.17
   Compiling clap_derive v4.5.49
   Compiling iana-time-zone v0.1.64
   Compiling log v0.4.29
   Compiling byteorder v1.5.0
   Compiling fern v0.7.1
   Compiling brarchive v0.1.0
   Compiling chrono v0.4.42
   Compiling clap v4.5.53
   Compiling humantime v2.3.0
   Compiling brarchive-cli v0.1.1
error[E0425]: cannot find function `exists` in module `fs`
  --> /home/ubuntu/.cargo/registry/src/index.crates.io-6f17d22bba15001f/brarchive-cli-0.1.1/src/main.rs:29:30
   |
29 |             let exists = fs::exists(&out).unwrap_or_else(|err| {
   |                              ^^^^^^ not found in `fs`

error[E0425]: cannot find function `exists` in module `fs`
  --> /home/ubuntu/.cargo/registry/src/index.crates.io-6f17d22bba15001f/brarchive-cli-0.1.1/src/main.rs:44:30
   |
44 |             let exists = fs::exists(&path).unwrap_or_else(|err| {
   |                              ^^^^^^ not found in `fs`

error[E0425]: cannot find function `exists` in module `fs`
   --> /home/ubuntu/.cargo/registry/src/index.crates.io-6f17d22bba15001f/brarchive-cli-0.1.1/src/main.rs:122:30
    |
122 |             let exists = fs::exists(&path).unwrap_or_else(|err| {
    |                              ^^^^^^ not found in `fs`

error[E0425]: cannot find function `exists` in module `fs`
   --> /home/ubuntu/.cargo/registry/src/index.crates.io-6f17d22bba15001f/brarchive-cli-0.1.1/src/main.rs:142:30
    |
142 |             let exists = fs::exists(&out).unwrap_or_else(|err| {
    |                              ^^^^^^ not found in `fs`

For more information about this error, try `rustc --explain E0425`.
error: could not compile `brarchive-cli` (bin "brarchive-cli") due to 4 previous errors
error: failed to compile `brarchive-cli v0.1.1`, intermediate artifacts can be found at `/tmp/cargo-installVYqirb`.
To reuse those artifacts with a future compilation, set the environment variable `CARGO_TARGET_DIR` to that path
```

</details>

## Building

This project uses GNU autotools. To build from source:

```bash
# Generate build system files
autoreconf -fiv

# Configure for your system
./configure

# Build
make

# Run tests (optional)
make check

# Install (optional)
make install
```

See `BUILD.md` for detailed build instructions and configuration options.

## Usage

The tool follows a similar interface to the standard `ar` command. The default tool name is `br-ar`.

### Create/Replace Archive

Create or replace files in a `.brarchive` file from a directory:

```bash
br-ar -r <archive.brarchive> <directory>
br-ar -rc <archive.brarchive> <directory>       # Combined flags (r + c, silent create)
br-ar -rv <archive.brarchive> <directory>      # Verbose create
```

Options:
- `-r`: Replace/add files (creates archive if doesn't exist)
- `-c`: Suppress "creating archive" message (silent mode)
- `-v`: Verbose mode (shows extracted files)

Examples:
```bash
br-ar -r pack.brarchive ./mydir
br-ar -rc pack.brarchive ./mydir             # Silent create
br-ar -rv pack.brarchive ./mydir             # Verbose create
```

### List Archive Contents

List the contents of a `.brarchive` file:

```bash
br-ar -t <archive.brarchive> [file ...]
```

Examples:
```bash
br-ar -t pack.brarchive
br-ar -t pack.brarchive file1.json file2.json
```

### Extract Archive

Extract files from a `.brarchive` to the current directory:

```bash
br-ar -x <archive.brarchive> [file ...]
br-ar -xv <archive.brarchive> [file ...]     # Verbose mode
```

Examples:
```bash
br-ar -x pack.brarchive              # Extract all files
br-ar -x pack.brarchive file1.json   # Extract specific file
br-ar -xv pack.brarchive             # Verbose extract
```

### Print Archive Contents

Print file contents to stdout:

```bash
br-ar -p <archive.brarchive> [file ...]
```

Examples:
```bash
br-ar -p pack.brarchive              # Print all files
br-ar -p pack.brarchive file1.json   # Print specific file
```

### Delete Files from Archive

Delete files from a `.brarchive` file:

```bash
br-ar -d <archive.brarchive> <file ...>
br-ar -dv <archive.brarchive> <file ...>     # Verbose mode
```

Examples:
```bash
br-ar -d pack.brarchive file1.json           # Delete one file
br-ar -d pack.brarchive file1.json file2.json # Delete multiple files
br-ar -dv pack.brarchive file1.json          # Verbose delete (shows deleted files)
```

**Note**: The delete operation modifies the archive in place by recreating it with the remaining files. Files are matched by exact name as they appear in the archive listing.

## `brarchive-cli` Compatibility Wrapper

A drop-in compatibility wrapper `brarchive-cli` is provided that matches the interface of the Rust [brarchive-cli](https://crates.io/crates/brarchive-cli) crate. The wrapper is a POSIX-compliant shell script that translates commands to the underlying `br-ar` tool.

```bash
# Encode (create archive from directory)
brarchive-cli encode <input_folder> <output_file>

# Decode (extract archive to directory)
brarchive-cli decode <input_file> <output_folder>

# Help
brarchive-cli help
```

Examples:
```bash
brarchive-cli encode ./mydir ./pack.brarchive
brarchive-cli decode ./pack.brarchive ./output
```

The wrapper is installed automatically when `--enable-symlinks` is used (default). It can be disabled with `./configure --disable-symlinks`.

Additionally, a `brarchive` symlink to `br-ar` is created for convenience.

## File Format

The `.brarchive` format is a simple uncompressed text archive format used by Minecraft Bedrock Edition since version 1.21.40.20 ([changelog](https://feedback.minecraft.net/hc/en-us/articles/29937458432397-Minecraft-Beta-Preview-1-21-40-20#user-content-resource-and-behavior-packs)).

### Header (16 bytes)
- 8 bytes: Magic number (`7d2725b1a0527026`)
- 4 bytes: Number of entries (little-endian)
- 4 bytes: Version number (always `1` for now, little-endian)

### Entries (256 bytes each)
- 1 byte: Length of entry name
- 247 bytes: Entry name (padded with zeros)
- 4 bytes: File contents offset (little-endian)
- 4 bytes: File contents length (little-endian)

### Data Block
- Uninterrupted stream of file contents (UTF-8 encoded)

## Portability

This tool is designed for maximum portability:

- **Cross-platform**: Works on Linux, BSD variants (FreeBSD, OpenBSD), macOS/iOS/Darwin, and other POSIX-compliant systems
- **POSIX shell scripts**: All shell scripts (wrapper and tests) are POSIX-compliant and work with any POSIX shell (dash, ash, sh, etc.)
- **Architecture support**: Supports various architectures (x86, x86_64, ARM, etc.)
- **Endian handling**: Automatically detects and handles endianness, with platform-specific optimizations available
- **Standard C**: Written in C11 with careful attention to portability

## Limitations

- Maximum file name length: 247 bytes
- Files are stored as text (UTF-8), binary files may not work correctly
- No compression (uncompressed archive format)

## License

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

### Attribution

This program is based on the original Rust implementation
[brarchive-cli](https://github.com/theaddonn/brarchive-rs) by
Lucy <theaddonn@gmail.com>. This C implementation is a complete rewrite
with the following modifications:

- Rewritten in C11 for better portability and performance
- Uses GNU Autotools for cross-platform build system
- Implements ar(1)-like command-line interface
- Added delete operation support
- POSIX-compliant shell scripts for wrapper and tests

