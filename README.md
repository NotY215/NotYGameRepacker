# NotY Game Repacker

[![Version](https://img.shields.io/badge/version-1.0.0-blue.svg)](https://github.com/NotY215/NotYGameRepacker)
[![Platform](https://img.shields.io/badge/platform-Windows%2010%2F11%20x64-brightgreen.svg)](https://www.microsoft.com/windows)
[![License](https://img.shields.io/badge/license-Proprietary-red.svg)](LICENSE)
[![C++](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://isocpp.org/)

**Professional Windows Game Packaging/Repacking System**

NotY Game Repacker is a comprehensive, professional-grade game packaging system for Windows. It compresses, encrypts, and packages game files into the optimized `.noty` format, complete with a professional installer.

---

## 🚀 Features

### Core Features
- **Package Games** into optimized `.noty` format
- **Zstandard Compression** with streaming support (levels 1-22)
- **AES-256-GCM Encryption** via Windows CNG (authenticated encryption)
- **BLAKE3 Integrity Verification** for tamper detection
- **Professional Installer** with modern dark theme
- **Adaptive Resource Management** based on available RAM

### Technical Highlights
- **Streaming Operations** - No loading entire archives into RAM
- **Multi-threaded Processing** - Parallel compression and encryption
- **Chunk-based Packaging** - Split packages into configurable chunks
- **Component Support** - Optional game components
- **Memory-Efficient** - Bounded buffers adapt to system resources
- **Progress Reporting** - Real-time ETA and throughput monitoring

### User Experience
- **Dark Theme UI** - Professional, eye-friendly interface
- **Wizard-Based Workflow** - Step-by-step repacking and installation
- **Live Preview** - Cover image preview during repacking
- **Detailed Logging** - Real-time progress log for transparency
- **Keyboard Navigation** - Full keyboard support
- **ETA & Throughput** - Real-time performance metrics

---

## 📋 System Requirements

### Development
| Component | Requirement |
|-----------|-------------|
| OS | Windows 10/11 (64-bit) |
| IDE | Visual Studio 2026 (Community or higher) |
| Compiler | MSVC v19.51+ (Visual Studio 2026 toolchain) |
| CMake | 3.20 or higher |
| vcpkg | Latest version |
| Git | For vcpkg bootstrap |

### Runtime
| Component | Requirement |
|-----------|-------------|
| OS | Windows 10/11 (64-bit) |
| RAM | 4GB minimum (8GB+ recommended) |
| Disk Space | 100MB for application + game packages |
| Processor | Multi-core recommended |

---

## 📦 Dependencies

All dependencies are managed through **vcpkg** and installed globally:

| Library | Version | Purpose |
|---------|---------|---------|
| Qt6 (qtbase) | 6.7.2 | GUI framework |
| Zstandard (zstd) | Latest | Compression |
| nlohmann-json | Latest | JSON parsing |
| BLAKE3 (blake3) | Latest | Cryptographic hashing |

### Qt Features
- Core, Widgets, Gui
- Network, Concurrent
- OpenGL, Freetype
- Harfbuzz, PNG

---

## 🔧 Quick Setup

### 1. Install vcpkg

```cmd
cd /d F:\
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
```

### 2. Install Dependencies

Create `F:\vcpkg\vcpkg.json` with:

```json
{
  "name": "global-dependencies",
  "version": "1.0",
  "builtin-baseline": "30ef65cad98f08e7197c9a1656fbd871bcb72f2d",
  "dependencies": [
    {
      "name": "qtbase",
      "default-features": false,
      "features": [
        "gui", "widgets", "network",
        "concurrent", "opengl",
        "freetype", "harfbuzz", "png"
      ]
    },
    "zstd",
    "nlohmann-json",
    "blake3"
  ],
  "overrides": [
    {
      "name": "qtbase",
      "version": "6.7.2"
    }
  ]
}
```

Then install:

```cmd
cd /d F:\vcpkg
vcpkg install --triplet x64-windows
```

### 3. Set Environment Variables

```cmd
set VCPKG_ROOT=F:\vcpkg
```

Or run the setup script:
```cmd
tools\setup_env.bat
```

For detailed vcpkg setup, see [docs/vcpkg.md](docs/vcpkg.md).

---

## 🏗️ Building

### Quick Build

```cmd
tools\build.bat
```

### Manual Build

```cmd
cmake --preset default
cmake --build build --config Release --parallel
```

### Build Output

- **Repacker**: `build/apps/Repacker/Release/NotYRepacker.exe`
- **Setup**: `build/apps/Setup/Release/NotYSetup.exe`

For detailed build instructions, see [docs/BUILD.md](docs/BUILD.md).

---

## 📚 Documentation

| Document | Description |
|----------|-------------|
| [BUILD.md](docs/BUILD.md) | Complete build instructions |
| [DEPLOYMENT.md](docs/DEPLOYMENT.md) | Deployment and packaging guide |
| [USER_GUIDE.md](docs/USER_GUIDE.md) | End-user documentation |
| [vcpkg.md](docs/vcpkg.md) | vcpkg setup guide |

---

## 🎮 Usage

### Repacker (Packaging)

1. Launch `NotYRepacker.exe`
2. Select your game folder
3. Choose a cover image
4. Configure package settings
5. Review and start repacking
6. Get your `.noty` package

### Installer (Installation)

1. Run `NotYSetup.exe` from package directory
2. Choose installation location
3. Select components
4. Install and verify
5. Launch the game

---

## 📁 Project Structure

```
NotYGameRepacker/
├── apps/                    # Applications
│   ├── Repacker/           # Repacker GUI
│   └── Setup/              # Installer GUI
├── include/noty/           # Public headers
│   ├── common/             # Common utilities
│   ├── filesystem/         # File operations
│   ├── package/            # Manifest handling
│   ├── hashing/            # Cryptographic hashing
│   ├── compression/        # Zstandard compression
│   ├── crypto/             # AES encryption
│   ├── repacker/           # Packaging engine
│   ├── installer/          # Installation engine
│   └── core/               # Resource management
├── src/                    # Implementation
├── resources/              # Application resources
│   └── fonts/              # Rubik font family
├── ui/                     # Qt UI files
│   ├── repacker/           # Repacker UI
│   └── installer/          # Installer UI
├── cmake/                  # CMake modules
├── tools/                  # Build tools
└── docs/                   # Documentation
```

---

## 🔒 Security

### Encryption
- **AES-256-GCM** via Windows CNG
- Authenticated encryption with tamper detection
- Secure key management
- No destructive anti-debugging

### Integrity
- **BLAKE3** cryptographic hashing
- File-level verification
- Chunk checksums
- Manifest validation

---

## 📊 Performance

### Adaptive Resource Management

| Memory Profile | Compression Buffer | Thread Pool |
|----------------|-------------------|-------------|
| Conservative (<8GB) | 512 KB | Cores/2 |
| Moderate (8-16GB) | 1 MB | Cores-1 |
| High (16-32GB) | 2 MB | All Cores |
| Aggressive (32+GB) | 4 MB | All Cores |

### Optimizations
- Streaming compression/encryption
- Multi-threaded processing
- Memory-mapped I/O
- Parallel compilation
- Link-time optimization

---

## 🛠️ Development

### Tools

| Tool | Purpose |
|------|---------|
| `tools/setup_env.bat` | Environment setup |
| `tools/build.bat` | Build automation |
| `tools/package.bat` | Package creation |
| `tools/deploy.bat` | Qt deployment |

### Coding Standards
- C++20 standard
- RAII principles
- Modern C++ idioms
- Comprehensive error handling
- Clear logging

---

## 📄 License

This project is proprietary software. All rights reserved.

---

## 👥 Authors

**NotY215** - Project Lead

- Website: https://noty215.com
- Email: support@noty215.com

---

## 🙏 Acknowledgments

- **Qt** - Cross-platform UI framework
- **Zstandard** - Fast compression
- **BLAKE3** - Cryptographic hashing
- **nlohmann/json** - JSON handling
- **Rubik Font** - Modern typography

---

## 📞 Support

### Resources
- Documentation: `/docs` directory
- Issues: Contact support via email
- Website: https://noty215.com

### Feedback
We welcome feedback and suggestions for improvement. Please reach out via email.

---

## 🗺️ Roadmap

### Completed Features ✅
- [x] Directory scanning and file enumeration
- [x] Manifest and metadata system
- [x] BLAKE3 hashing
- [x] Zstandard streaming compression
- [x] AES-256-GCM encryption
- [x] Package builder and repack engine
- [x] Installer engine
- [x] Setup.exe application
- [x] Repacker UI polish
- [x] Installer UI polish
- [x] Performance optimization
- [x] Build automation

### Future Plans 🔮
- [ ] Additional compression algorithms
- [ ] Delta patching support
- [ ] Network distribution
- [ ] Steam integration
- [ ] Linux support (future)

---

## 📝 Notes

### Important
- The project is designed for Windows 10/11 x64 only
- All dependencies are managed via vcpkg
- No project-local vcpkg.json is required
- Qt 6.7.2 is used, not 6.11.1

### Credits
- **NotY215** - All rights reserved
- **Rubik Font** - Copyright © 2020, The Rubik Project Authors
- **Zstandard** - Copyright © 2016-present, Facebook, Inc.
- **BLAKE3** - Copyright © 2019-2020, The BLAKE3 Team

---

## 📖 Quick Reference

### Build Commands

```cmd
# Setup environment
call tools\setup_env.bat

# Build
tools\build.bat

# Package
tools\package.bat

# Deploy Qt
tools\deploy.bat
```

### Environment Variables

```cmd
VCPKG_ROOT=F:\vcpkg
```

### Output Locations

```cmd
# Repacker
build/apps/Repacker/Release/NotYRepacker.exe

# Setup
build/apps/Setup/Release/NotYSetup.exe

# Distribution
dist/
```

---

**© 2026 NotY215. All Rights Reserved.**

