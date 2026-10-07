# Architecture

## Overview

NotY Game Repacker is designed as a compact native Windows application suite.

The primary technology direction is:

- C17
- C++20
- Windows SDK
- Win32
- CMake
- MSVC
- Windows x64

Qt and other GUI frameworks are intentionally excluded.

## Application model

The repository contains two application targets:

### Repacker

Responsible for the future package creation workflow:

```
Source directory
    |
    v
Directory scanning
    |
    v
Metadata collection
    |
    v
Manifest
    |
    v
Streaming processing
    |
    v
Chunk generation
    |
    v
Setup generation
    |
    v
Package validation
```

### Setup

Responsible for the future installation workflow:

```
Package
    |
    v
Metadata validation
    |
    v
Installation directory
    |
    v
Disk/permission checks
    |
    v
Extraction
    |
    v
File verification
    |
    v
Completion
```

These flows describe the target architecture. They are not a claim that every stage is currently implemented.

## C and C++ responsibilities

### C

C is appropriate for:

- low-level primitives
- simple C-compatible interfaces
- memory-efficient routines
- binary/data processing where a C implementation is clearer
- system-level helpers where C has a practical advantage

### C++

C++20 is appropriate for:

- application orchestration
- package management
- workflow/state handling
- resource ownership
- UI coordination
- configuration
- worker/job systems
- installer and repacker orchestration

C is not used merely to increase the language count, and C++ is not used where a simple C component is more appropriate.

## UI architecture

The UI is native Win32.

The project aims to provide:

- dark NotY215 styling
- DPI awareness
- Unicode support
- resizable layouts
- progress indicators
- dialogs
- logging
- file/folder selection
- game-cover presentation

Heavy work must not run on the UI thread.

Worker code communicates with the UI through safe state/message mechanisms.

## Core libraries

The current build defines a small C core library and a native Win32 UI library.

The architecture is intentionally prepared for future filesystem, package, processing, installer, and validation components without creating dozens of tiny source files.

## Resources

Project resources are stored under `resources/`.

The official NotY branding uses the project logo. Resource handling is designed so that branding can be embedded into the application where practical.

Bundled fonts may be used by the UI. The final font set is a project decision and should not be documented as fixed until selected.

## Memory model

Large files must be processed as streams.

The target architecture uses:

```
Disk
  |
  v
Bounded buffer
  |
  v
Processing
  |
  v
Bounded output
  |
  v
Disk
```

The system must not load a complete 50 GB or 100 GB game into RAM.

All important file sizes and package offsets use 64-bit values.

## Threading

The target worker model uses bounded concurrency.

The implementation should consider:

- logical CPU count
- memory availability
- workload
- storage characteristics

There must not be one uncontrolled thread per file.

The UI thread remains responsive.

## Cancellation

Cancellation should be cooperative:

```
User requests cancel
    |
    v
Cancellation requested
    |
    v
Finish safe current operation
    |
    v
Close/flush resources
    |
    v
Clean temporary state
    |
    v
Return safely
```

Abrupt worker termination is avoided.

## Error handling

Errors should be presented as useful user-facing messages.

Examples include:

- Source directory does not exist.
- Permission denied.
- Disk is full.
- Package chunk is missing.
- Package metadata is invalid.
- Unsupported package version.
- Installation directory is invalid.
- File cannot be written.

Low-level error codes may be retained for diagnostics but should not be the only user-facing explanation.

## Dependency policy

Prefer Windows platform facilities over external libraries when practical.

Do not add a third-party dependency merely because it is convenient.

Security-sensitive functionality must never be replaced with an unsafe homemade implementation just to remove a dependency.

## Current security implementation

Cryptography is **not implemented at this stage**.

The architecture may later use Windows-provided cryptographic APIs, but no encryption or authenticated-encryption feature should be advertised until a complete implementation exists.

