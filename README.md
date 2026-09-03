# NotY Game Repacker

[![Version](https://img.shields.io/badge/version-1.0.0-blue.svg)](https://github.com/NotY215/NotYGameRepacker)
[![Platform](https://img.shields.io/badge/platform-Windows%2010%2F11%20x64-brightgreen.svg)](https://www.microsoft.com/windows)
[![License](https://img.shields.io/badge/license-GPL--3.0-red.svg)](LICENSE)
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
| vcpkg | Built-in with Visual Studio 2026 |

### Runtime
| Component | Requirement |
|-----------|-------------|
| OS | Windows 10/11 (64-bit) |
| RAM | 4GB minimum (8GB+ recommended) |
| Disk Space | 100MB for application + game packages |
| Processor | Multi-core recommended |

---

## 📦 Dependencies

All dependencies are managed through **Visual Studio's built-in vcpkg**:

| Library | Purpose |
|---------|---------|
| Qt6 (qtbase) | GUI framework |
| Zstandard (zstd) | Compression |
| nlohmann-json | JSON parsing |
| BLAKE3 (blake3) | Cryptographic hashing |

### Qt Features
- Core, Widgets, Gui
- Network, Concurrent
- OpenGL, Freetype
- Harfbuzz, PNG

---

## 🔧 Setup

### 1. Configure vcpkg.json

Create `vcpkg.json` in your project root with the following content:

```json
{
  "name": "noty-game-repacker",
  "version": "1.0.0",
  "description": "Professional Windows game packaging/repacking system",
  "homepage": "https://github.com",
  "license": "GPL-3.0 license",
  "supports": "windows & x64",
  "builtin-baseline": "set baseline to that repo's HEAD",
  "dependencies": [
    {
      "name": "qtbase",
      "default-features": false,
      "features": [
        "gui",
        "widgets",
        "network",
        "concurrent",
        "opengl",
        "freetype",
        "harfbuzz",
        "png"
      ]
    },
    "zstd",
    "nlohmann-json",
    "blake3"
  ]
}
```

### 2. Integrate vcpkg with Visual Studio

Run the following command in Developer Command Prompt for Visual Studio:

```cmd
vcpkg integrate install
```

### 3. Install Dependencies

```cmd
"C:\Program Files\Microsoft Visual Studio\18\Community\VC\vcpkg\vcpkg.exe" install --triplet x64-windows
```

---

## 🏗️ Building

### Build in Visual Studio

1. Open the project folder in Visual Studio
2. CMake will automatically detect the presets
3. Select the `default` preset
4. Build the solution

### Build from Command Line

```cmd
cmake --preset default
cmake --build build --config Release
```

### Build Output

- **Repacker**: `build/apps/Repacker/Release/NotYRepacker.exe`
- **Setup**: `build/apps/Setup/Release/NotYSetup.exe`

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
├── CMakeLists.txt          # Main CMake file
├── CMakePresets.json       # CMake presets
└── vcpkg.json              # vcpkg dependencies
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

## 📄 License

This project is licensed under the **GPL-3.0 License**. See the [LICENSE](LICENSE) file for details.

---

## 👥 Authors

**NotY215** - Project Lead

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

### Feedback
We welcome feedback and suggestions for improvement.

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
- All dependencies are managed via Visual Studio's built-in vcpkg
- Qt 6.x is used via vcpkg manifest mode

### Credits
- **NotY215** - All rights reserved
- **Rubik Font** - Copyright © 2020, The Rubik Project Authors
- **Zstandard** - Copyright © 2016-present, Facebook, Inc.
- **BLAKE3** - Copyright © 2019-2020, The BLAKE3 Team

---

**© 2026 NotY215. All Rights Reserved.**