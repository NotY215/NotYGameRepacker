# NotY Game Repacker User Guide

## Status

The user-facing workflow is under active development.

This guide describes the intended workflow from the master architecture. It does not claim that every screen or package operation is currently implemented.

## What NotY Game Repacker is

NotY Game Repacker is a native Windows application for packaging files and games that the user legitimately owns or possesses.

The project is intended to provide two applications:

- **NotY Game Repacker** for creating packages
- **Generated Setup.exe** for installing packages

The project does not provide DRM, license, activation, launcher, or ownership bypass functionality.

## Repacker workflow

The intended workflow is:

1. Welcome
2. Select source directory
3. Select game cover
4. Configure package
5. Review settings
6. Repack
7. Verify package
8. Finish

### Source directory

The source directory should contain the complete set of files intended for packaging.

The final implementation should collect:

- files
- directories
- file sizes
- relevant metadata
- package totals

Large directories should be processed without loading all file data into memory.

### Game cover

The package cover is game-specific artwork.

It is separate from the project's NotY215 logo.

The intended supported image set includes common formats such as PNG and JPEG. Exact implementation support will be documented once the image subsystem is complete.

### Package configuration

The Repacker is intended to collect:

- game name
- source directory
- game cover
- chunk configuration
- Setup executable name
- repacker username

The displayed repacker name is explicitly entered by the package creator.

It must not silently use the Windows `%USERNAME%` value.

The default project value is intended to be:

```
NotY215
```

### Review

Before starting a package operation, the UI should present the selected configuration so the creator can confirm it.

### Repacking

The target processing flow is:

```
Scanning files
    |
    v
Building manifest
    |
    v
Processing data
    |
    v
Creating chunks
    |
    v
Generating Setup
    |
    v
Validating package
```

Progress must represent actual work.

The project must not use fake progress percentages.

### Cancellation

Cancellation should be safe and cooperative.

The intended behavior is:

```
Cancel
 -> request cancellation
 -> finish safe current operation
 -> close files
 -> clean temporary state
 -> return safely
```

## Generated Setup workflow

The intended Setup flow is:

1. Launch
2. Load package metadata
3. Validate package structure
4. Select installation directory
5. Check permissions
6. Check disk space
7. Select optional components when available
8. Install
9. Verify
10. Finish

The generated installer must be generic and must not contain game-specific hardcoded logic.

## Installation safety

Before installation, the Setup application should validate:

- package metadata
- package structure
- chunk count
- chunk identifiers
- supported format version
- required disk space
- installation directory

If a validation step fails, installation should stop safely and display a useful explanation.

## Disk space

The installer should show:

```
Required: 64.2 GB
Available: 183.4 GB
Status: Enough space
```

If there is not enough space, installation should be disabled or stopped before destructive installation work begins.

## Package files

The intended package layout is:

```
GameName/
├── Setup.exe
├── manifest.noty
├── cover.png
├── GameName.001.noty
├── GameName.002.noty
└── ...
```

The exact final layout may change while the package format is being implemented.

## Performance

For large packages:

- Prefer SSD storage when available.
- Keep source and output on reliable storage.
- Avoid unnecessary copies.
- Use bounded memory.
- Do not run one thread per file.
- Keep the UI responsive.
- Use 64-bit file sizes.

The fastest configuration is not always the safest configuration. Correctness and data integrity come first.

## Cryptography status

Cryptography is **not implemented in the current stage**.

Do not enable, advertise, or assume encryption features until they are actually implemented.

## Troubleshooting

### The application does not build

Check:

- Windows x64 environment
- Visual Studio 2022/MSVC
- Windows SDK
- CMake 3.24+
- selected CMake preset

See [BUILDING.md](BUILDING.md).

### The UI does not show the project logo

Check that the expected resource exists under `resources/`.

The build system can continue without the logo, but reports that the embedded logo is unavailable.

### A package operation fails

Record:

- the application/build version
- Windows version
- build configuration
- operation being performed
- error message
- relevant log output
- approximate package size

Do not include passwords, tokens, private keys, or unrelated personal data.

## Support

For bug and documentation reports, see [REPORTING_GUIDELINES.md](../REPORTING_GUIDELINES.md).

For architecture and development questions, see [DEVELOPMENT.md](DEVELOPMENT.md).
