# Building NotY Game Repacker

## Supported environment

The project currently targets:

- Windows 10/11
- x64
- Visual Studio 2022 / MSVC
- Windows SDK
- CMake 3.24 or newer

Ninja is supported by the repository presets when installed.

## CMake standards

The project uses:

- C17
- C++20
- CMake 3.24+
- x64-only configuration

The top-level CMake configuration rejects non-Windows and non-64-bit builds.

## Visual Studio 2022

From a Developer Command Prompt or a shell with the required MSVC environment:

```cmd
cmake --preset vs2022
cmake --build --preset release
```

Debug:

```cmd
cmake --preset vs2022
cmake --build --preset debug
```

RelWithDebInfo:

```cmd
cmake --preset vs2022
cmake --build --preset relwithdebinfo
```

## Ninja

Release:

```cmd
cmake --preset ninja-release
cmake --build --preset ninja-release
```

Debug:

```cmd
cmake --preset ninja-debug
cmake --build --preset ninja-debug
```

The Ninja presets use the MSVC environment supplied by the developer environment.

## Build output

The exact output path depends on the selected preset.

Visual Studio presets use a configuration-specific build tree under:

```
build/vs2022/
```

Ninja presets use:

```
build/ninja-release/
build/ninja-debug/
```

Do not commit generated build output.

## Build configuration

The project supports:

- Debug
- Release
- RelWithDebInfo

Release is intended for distribution.

## Runtime model

The CMake configuration uses the static MSVC runtime setting to support compact distribution where compatible.

The application is otherwise designed to rely on Windows platform APIs rather than requiring a large third-party runtime stack.

## Troubleshooting

### CMake cannot find MSVC

Use a Visual Studio Developer Command Prompt or ensure the MSVC environment is available to CMake.

### CMake rejects the architecture

Configure an x64 build. The project is intentionally 64-bit only.

### Build succeeds but a resource is missing

Check the `resources/` directory and confirm that the expected project resource exists. The current CMake configuration can build without `resources/logo.png`, but reports that the embedded logo is unavailable.

### Clean build

Remove the generated `build/` directory and configure again.

```cmd
rmdir /s /q build
cmake --preset vs2022
cmake --build --preset release
```

Only run the removal command when you are sure the directory contains generated build output.

## Development builds

Use Debug for debugging and Release for normal performance testing.

RelWithDebInfo is useful when performance investigation requires symbols.
