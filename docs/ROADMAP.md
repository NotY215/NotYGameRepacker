# Roadmap

This roadmap follows the current master development direction.

A phase is complete only when its functionality works and can be verified.

## Phase 1: Project foundation

- [x] Native Windows/x64 CMake foundation
- [x] C17 and C++20 configuration
- [x] MSVC configuration
- [x] Repacker application target
- [x] Setup application target
- [x] Native Win32 UI foundation
- [x] Basic resource/logo support
- [ ] Complete production UI workflows

## Phase 2: Filesystem engine

- [ ] Directory scanning
- [ ] File enumeration
- [ ] File metadata collection
- [ ] 64-bit size handling
- [ ] Unicode path handling
- [ ] Long-path handling
- [ ] Disk-space calculation

## Phase 3: Manifest and metadata

- [ ] Package identity
- [ ] Manifest reader
- [ ] Manifest writer
- [ ] File metadata records
- [ ] Chunk mapping
- [ ] Controlled metadata serialization

## Phase 4: Streaming processing

- [ ] Bounded input buffers
- [ ] Bounded output buffers
- [ ] Reusable buffers
- [ ] Cancellation-aware processing
- [ ] Worker/job architecture
- [ ] Resource-aware concurrency

## Phase 5: .noty package format

- [ ] Versioned package header
- [ ] Package metadata integration
- [ ] Chunk header
- [ ] Chunk indexing
- [ ] Stable serialization rules
- [ ] Compatibility validation

## Phase 6: Large-file support

- [ ] Files above 4 GB
- [ ] Large package offsets
- [ ] Large chunk sizes
- [ ] Streaming package creation
- [ ] Streaming package reading

## Phase 7: Security and integrity

Current decision:

**Cryptography is not implemented now.**

Future work may include package/file integrity and later cryptographic protection, but only after a complete design and implementation are available.

- [ ] Package integrity
- [ ] Chunk validation
- [ ] Installed-file verification
- [ ] Future Windows CNG integration if required
- [ ] Security test coverage

## Phase 8: Installer engine

- [ ] Package discovery
- [ ] Manifest validation
- [ ] Installation directory validation
- [ ] Permission checks
- [ ] Disk-space checks
- [ ] Extraction
- [ ] Safe cancellation
- [ ] Temporary-state cleanup

## Phase 9: Installation verification

- [ ] File existence validation
- [ ] Size validation
- [ ] Integrity validation
- [ ] Clear failure reporting
- [ ] Safe stop behavior

## Phase 10: Repacker UI

- [ ] Source selection
- [ ] Cover selection
- [ ] Package configuration
- [ ] Repacker name
- [ ] Setup name
- [ ] Review screen
- [ ] Accurate progress
- [ ] Logging
- [ ] Completion screen

## Phase 11: Generated Setup UI

- [ ] Package metadata loading
- [ ] Welcome screen
- [ ] Installation directory selection
- [ ] Component selection
- [ ] Disk-space display
- [ ] Installation progress
- [ ] Verification progress
- [ ] Completion screen

## Phase 12: Performance

- [ ] Adaptive worker count
- [ ] Bounded memory tuning
- [ ] SSD/HDD-aware I/O strategy
- [ ] Reduced unnecessary copies
- [ ] Stable ETA calculation
- [ ] Controlled UI updates

## Phase 13: Resume and components

- [ ] Resume state
- [ ] Completed chunk tracking
- [ ] Interrupted-install recovery
- [ ] Optional components
- [ ] Component-specific disk-space calculation

## Phase 14: Release

- [ ] Release packaging
- [ ] Generated Setup distribution
- [ ] Documentation completion
- [ ] Compatibility testing
- [ ] Clean installation testing
- [ ] Release notes

## Completion rule

The repository must never mark a feature complete solely because:

- a header exists
- an interface exists
- a placeholder returns success
- documentation describes the intended behavior
- a build target exists

Implementation, testing, and documentation must agree.
