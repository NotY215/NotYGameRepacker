# NotY Game Repacker

Professional Windows game packaging and installation system for files and games the user legitimately owns.

**NotY Game Repacker** is a Windows-first C/C++ project by **NotY215**. The project is being built around a compact native architecture using Win32, the Windows SDK, CMake, and MSVC.

> **Current development status:** The repository is in active development. The native Win32 foundation and build architecture are present, while the complete package/repack/install pipeline is still being implemented. Features are documented as planned until they are actually working.

## Project goals

- Native Windows x64 application
- C and C++ used according to component responsibility
- C++20 for application and high-level systems
- C17 for suitable low-level/backend code
- Native Win32 UI with an original NotY215 design
- Streaming and bounded-memory processing for large files
- Versioned `.noty` package format
- Safe cancellation and clear error handling
- Compact, maintainable repository structure
- Minimal external dependencies
- No fake or placeholder security, compression, verification, or progress features

## Important scope

This project is for packaging, distributing, restoring, and verifying files that the user legitimately possesses.

It does **not** implement DRM bypassing, license bypassing, activation bypassing, launcher bypassing, or mechanisms intended to defeat legitimate ownership systems.

## Current architecture

The current foundation uses:

- C
- C++20
- Windows SDK
- Win32 API
- CMake 3.24+
- MSVC / Visual Studio 2022
- Windows x64
- Windows-native libraries where appropriate
- Static MSVC runtime configuration for compact distribution

Qt and other large GUI frameworks are not used.

The project also avoids adding third-party libraries when the same functionality can reasonably be provided by C/C++ and the Windows platform.

### Cryptography status

Cryptography is **not being implemented in the current development stage**.

The architecture leaves room for future package authentication and confidentiality, but the current project must not claim to provide encryption or authenticated cryptography until a real implementation is completed and tested.

Do not treat an unfinished crypto design as a security feature.

## Planned applications

### NotY Game Repacker

The Repacker will eventually provide a workflow similar to:

Source folder
-> file scan
-> metadata collection
-> manifest generation
-> processing
-> chunk generation
-> Setup generation
-> package verification

Heavy work must run outside the UI thread.

### Generated Setup

The generated installer is intended to:

Load package metadata
-> validate package structure
-> select installation directory
-> check permissions and disk space
-> extract package data
-> verify installed files
-> finish safely

The generated Setup application must remain generic and must not be hardcoded for a particular game.

## Package direction

The project is designed around a custom, versioned `.noty` package format.

A package may contain:

- Setup executable
- Package manifest
- Game cover
- One or more numbered package chunks

The binary chunk format is intended to contain versioned metadata such as package identity, chunk identity, sizes, processing method information, integrity information, and payload data.

The exact on-disk format must be treated as versioned engineering work and should not be considered stable until documented and implemented.

## Large-file and performance goals

Large files are a core requirement.

The implementation should:

- use 64-bit file sizes and offsets
- avoid loading whole games into memory
- use bounded buffers
- reuse buffers where practical
- prefer sequential I/O
- use controlled worker counts
- keep the UI responsive
- support safe cancellation
- adapt resource usage to the workload

Correctness and data integrity take priority over benchmark numbers.

## Repository structure

```
NotYGameRepacker/
├── apps/
│   ├── Repacker/       # Repacker application
│   └── Setup/          # Installer application
├── docs/               # Detailed project documentation
├── include/            # Public/internal headers
├── resources/          # Logo, fonts, icons and other assets
├── src/
│   ├── core/           # Low-level C/core functionality
│   └── ui/             # Native Win32 UI foundation
├── CMakeLists.txt
├── CMakePresets.json
├── CMakeSettings.json
├── LICENSE
├── README.md
├── Documentation.md
├── CODE_OF_CONDUCT.md
└── REPORTING_GUIDELINES.md
```

The structure is intentionally compact. New files should be added only when they represent a meaningful responsibility.

## Build

### Requirements

- Windows 10 or Windows 11
- x64 environment
- Visual Studio 2022 with MSVC and Windows SDK
- CMake 3.24 or newer
- Ninja if using the Ninja presets

### Visual Studio 2022

```cmd
cmake --preset vs2022
cmake --build --preset release
```

### Ninja Release

```cmd
cmake --preset ninja-release
cmake --build --preset ninja-release
```

### Ninja Debug

```cmd
cmake --preset ninja-debug
cmake --build --preset ninja-debug
```

See [docs/BUILDING.md](docs/BUILDING.md) for the full build guide.

## Documentation

- [Documentation index](Documentation.md)
- [Architecture](docs/ARCHITECTURE.md)
- [Building](docs/BUILDING.md)
- [Development](docs/DEVELOPMENT.md)
- [Package format](docs/PACKAGE_FORMAT.md)
- [Security and trust](docs/SECURITY.md)
- [Roadmap](docs/ROADMAP.md)
- [User guide](docs/USER_GUIDE.md)
- [Code of Conduct](CODE_OF_CONDUCT.md)
- [Reporting Guidelines](REPORTING_GUIDELINES.md)

## Roadmap

The implementation is being developed incrementally:

1. Project and native UI foundation
2. Filesystem and directory scanning
3. Manifest and package metadata
4. Streaming processing architecture
5. Versioned `.noty` package format
6. Chunking and large-file support
7. Future security/integrity layer
8. Installer extraction engine
9. Installation verification
10. Repacker UI
11. Generated Setup UI
12. Performance and resource tuning
13. Resume and optional components
14. Release packaging and documentation

A phase is not considered complete merely because its files or interfaces exist. The functionality must actually work.

## Development principles

- Do not generate the whole codebase at once.
- Work phase-by-phase.
- Keep C and C++ responsibilities deliberate.
- Prefer native Windows functionality.
- Avoid unnecessary dependencies.
- Do not silently replace established architecture.
- Document unfinished functionality clearly.
- Do not use fake implementations to make a feature appear complete.
- Never trade correctness or data safety for benchmark results.

## License

NotY Game Repacker is licensed under the [Apache License 2.0](LICENSE).

## Author

**NotY215**

Powered by NotY215.


## Project policies

- [Contributing](CONTRIBUTING.md)
- [Code of Conduct](CODE_OF_CONDUCT.md)
- [Security](SECURITY.md)
- [Support](SUPPORT.md)
- [Citation](CITATION.cff)
- [Governance](GOVERNANCE.md)
- [License](LICENSE)
