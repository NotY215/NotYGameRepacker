# Development Guide

## Development model

NotY Game Repacker is developed incrementally.

Do not generate or merge the entire planned system as one large implementation. Each phase should establish a working foundation for the next phase.

## Phase workflow

For each implementation phase:

1. State what is being implemented.
2. Identify files and folders being changed.
3. Keep the architecture consistent with previous phases.
4. Provide complete source files when a source file is intentionally replaced.
5. Explain dependencies.
6. Explain how the new component connects to the system.
7. Explain how to build and test it.
8. Mark unfinished behavior explicitly.
9. Do not claim completion without working functionality.

## Repository discipline

The project intentionally aims to remain compact.

Avoid:

- dozens of tiny source files
- unnecessary abstraction layers
- duplicate utility modules
- dependencies added only for convenience
- generated build artifacts in source control

Prefer meaningful modules grouped by responsibility.

## Language rules

Use C where it has a real low-level or interoperability advantage.

Use C++20 for:

- ownership
- resource management
- orchestration
- application state
- UI coordination
- higher-level package operations

Use clean C-compatible interfaces when C and C++ components need to communicate.

## Windows rules

Prefer native Unicode Windows APIs.

Support:

- Unicode paths
- large files
- Windows filesystem semantics
- DPI-aware UI
- x64 builds

Do not silently add cross-platform abstractions when the project requirement is Windows-first.

## CMake rules

Prefer target-specific CMake configuration:

- `target_sources`
- `target_include_directories`
- `target_link_libraries`
- `target_compile_features`
- `target_compile_definitions`

Avoid unnecessary global configuration.

## UI rules

The UI must remain responsive.

Heavy work belongs in worker jobs or threads, not directly in the window procedure.

Do not update Win32 controls unsafely from worker threads.

Keep progress updates controlled rather than sending an event for every byte processed.

## Resource rules

Project resources belong under `resources/`.

Branding uses the NotY215 identity.

Game cover artwork is package-specific and is separate from the project logo.

Bundled fonts must not be assumed to be installed globally.

## No fake features

Never use placeholder logic such as:

```cpp
// TODO: implement
return true;
```

to make an unfinished subsystem appear complete.

This applies especially to:

- compression
- package validation
- integrity verification
- encryption
- installation
- progress reporting

If a subsystem is unfinished, document it as unfinished.

## Testing expectations

Each completed subsystem should have tests or a practical verification procedure appropriate to its responsibility.

At minimum, test:

- normal operation
- empty inputs
- large inputs
- Unicode paths
- insufficient disk space
- permission failures
- cancellation
- malformed metadata
- interrupted operations where applicable

## Commit guidance

Use focused commits that describe the architectural change.

Examples:

- `Add native Win32 UI foundation`
- `Implement package manifest reader`
- `Add large-file streaming pipeline`
- `Document package format versioning`

Avoid commits that claim a feature is complete when only scaffolding exists.
