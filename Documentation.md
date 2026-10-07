# NotY Game Repacker Documentation

This is the documentation index for **NotY Game Repacker**.

The project is a native Windows packaging and installation system built around C/C++, Win32, the Windows SDK, CMake, and MSVC.

## Start here

| Document | Purpose |
|---|---|
| [README](README.md) | Project overview and quick start |
| [Architecture](docs/ARCHITECTURE.md) | System design and component responsibilities |
| [Building](docs/BUILDING.md) | Build requirements and CMake workflows |
| [Development](docs/DEVELOPMENT.md) | Development rules and implementation workflow |
| [Package Format](docs/PACKAGE_FORMAT.md) | `.noty` format direction and versioning |
| [Security](docs/SECURITY.md) | Security boundaries, integrity and current crypto status |
| [Roadmap](docs/ROADMAP.md) | Development phases and completion rules |
| [User Guide](docs/USER_GUIDE.md) | Planned user-facing Repacker and Setup workflows |
| [Code of Conduct](CODE_OF_CONDUCT.md) | Community behavior expectations |
| [Reporting Guidelines](REPORTING_GUIDELINES.md) | Bug and security reporting |

## Current implementation status

The repository has a native Windows foundation including:

- Windows x64 platform guards
- C17 and C++20 configuration
- CMake 3.24+ configuration
- MSVC target configuration
- Native Win32 UI foundation
- Dark UI/theme support
- DPI-aware UI direction
- Embedded logo/resource support
- Separate Repacker and Setup application targets
- A small C core library

The full packaging pipeline is still under development.

In particular, documentation must not imply that the following are complete unless the implementation has been verified:

- production compression
- final package serialization
- final installer extraction
- package verification
- cryptographic protection
- resume support
- optional component installation

## Architecture principles

The master development direction is intentionally compact:

- C for suitable low-level operations
- C++20 for application architecture and orchestration
- Win32 for the UI
- Windows filesystem and system APIs
- CMake for the build
- MSVC for Windows builds
- bounded memory and streaming for large files
- no unnecessary GUI frameworks
- no unnecessary third-party dependencies

## Security boundary

The project is intended for legitimate files owned or otherwise lawfully possessed by the user.

It does not provide DRM, activation, license, launcher, or ownership bypass functionality.

Cryptography is deliberately **not implemented at the current stage**. Any future security layer must be implemented and tested before being advertised as a security feature.

## Documentation rule

Documentation describes intended architecture separately from implemented functionality. If a feature is unfinished, it must be labelled as unfinished rather than presented as working.
