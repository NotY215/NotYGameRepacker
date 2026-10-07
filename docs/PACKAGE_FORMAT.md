# .noty Package Format

## Status

The `.noty` package format is a project design target and is still under active development.

This document describes the intended format direction. It is not a guarantee of a stable or backward-compatible file format.

## Package layout

A generated package is intended to resemble:

```
GameName/
├── Setup.exe
├── manifest.noty
├── cover.png
├── GameName.001.noty
├── GameName.002.noty
└── ...
```

The exact filenames may be customized by the package creator.

## Package metadata

The manifest is intended to describe information such as:

- format version
- package ID
- game name
- repacker name
- Setup filename
- original size
- processed size
- chunk count
- processing method
- cover information
- file count
- directories
- file paths
- file sizes
- file integrity information
- chunk mapping
- optional components

Large binary payloads should not be embedded directly into the manifest.

## Chunk design

Each chunk is intended to have a binary header followed by its payload.

Conceptually:

```
NOTY HEADER
------------
Magic
Format version
Package ID
Chunk ID
Chunk count
Processing method
Integrity information
Uncompressed size
Processed size
Payload
```

The exact binary field widths and serialization rules must be finalized before the format is declared stable.

## Versioning

The format must be explicitly versioned.

A future reader must be able to distinguish:

- supported versions
- unsupported versions
- malformed data

An unsupported version should produce a clear error instead of being silently interpreted as another version.

## Large files

Package sizes, file sizes, offsets, and chunk sizes must support values larger than 4 GB.

Use 64-bit types throughout the package implementation.

## Streaming

The package engine should stream data instead of loading a complete game into memory.

The intended pipeline is:

```
Input file
  -> bounded read
  -> processing
  -> bounded output
  -> chunk
```

## Manifest representation

The final manifest representation has not been frozen.

The implementation may use a compact controlled text format or a limited internal JSON-compatible representation where appropriate, but it must not introduce a large third-party JSON dependency solely for convenience.

## Cryptography

Cryptographic package protection is **not implemented in the current development stage**.

Therefore the current documentation does not claim that `.noty` packages are encrypted or cryptographically protected.

Future security fields must be versioned and documented when a real implementation is introduced.

## Compatibility

Until the format is declared stable:

- do not assume all development packages are mutually compatible
- do not treat internal fields as public API
- do not distribute undocumented format variants as stable releases
