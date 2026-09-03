# vcpkg Setup Guide for NotY Game Repacker

## Overview

This guide explains how to set up vcpkg and install all required dependencies for the NotY Game Repacker project using the global manifest method.

## Prerequisites

- Windows 10/11 (64-bit)
- Visual Studio 2026 with C++ workload
- Git (for vcpkg bootstrap)
- Internet connection

---

## Step 1: Install vcpkg

1. Open Command Prompt or PowerShell as Administrator

2. Navigate to F:\ (or your preferred drive):
```cmd
cd /d F:\
```

3. Clone vcpkg from GitHub:
```cmd
git clone https://github.com/Microsoft/vcpkg.git
```

4. Navigate to the vcpkg directory:
```cmd
cd vcpkg
```

5. Bootstrap vcpkg:
```cmd
.\bootstrap-vcpkg.bat
```

6. Verify installation:
```cmd
.\vcpkg.exe --version
```

---

## Step 2: Get the Git Baseline

Get the current commit hash for the baseline:

```cmd
cd /d F:\vcpkg
git rev-parse HEAD
```

This will output a hash like: `30ef65cad98f08e7197c9a1656fbd871bcb72f2d`

---

## Step 3: Create the Global Manifest File

1. Open your text editor (Notepad, VS Code, etc.)

2. Create a new file with the following content. Replace the baseline with the hash you got from Step 2:

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
  ],
  "overrides": [
    {
      "name": "qtbase",
      "version": "6.7.2"
    }
  ]
}
```

3. Save this file as: `F:\vcpkg\vcpkg.json`

---

## Step 4: Install Dependencies

Open Command Prompt and run:

```cmd
cd /d F:\vcpkg
vcpkg install --triplet x64-windows
```

This will:
- Download all required source packages
- Compile each library for x64-windows
- Take approximately 30-60 minutes depending on your system

---

## Step 5: Move Binaries to Global Folder

After installation completes (100%):

1. Open Windows File Explorer
2. Navigate to: `F:\vcpkg\vcpkg_installed\x64-windows\`
3. Select all folders (bin, include, lib, share, etc.)
4. Cut (Ctrl + X)
5. Navigate to: `F:\vcpkg\installed\x64-windows\`
6. Paste (Ctrl + V)
7. Click "Yes" or "Merge" when prompted

---

## Step 6: Clean Up

1. Delete: `F:\vcpkg\vcpkg.json`
2. Delete: `F:\vcpkg\vcpkg_installed\` (the entire folder)

---

## Step 7: Set Environment Variables

### Manual Setup (Current Session)

Run these commands in your Command Prompt:

```cmd
set VCPKG_ROOT=F:\vcpkg
set PATH=%VCPKG_ROOT%;%PATH%
```

### Permanent Setup (Recommended)

#### Method 1: Windows GUI

1. Press `Win + X` and select "System"
2. Click "Advanced system settings"
3. Click "Environment Variables..."
4. Under "System variables", click "New"
5. Variable name: `VCPKG_ROOT`
6. Variable value: `F:\vcpkg`
7. Click OK

#### Method 2: Using the Setup Script

Run the setup script from the project directory:

```cmd
cd /d F:\OwnApps\NotYGameRepacker
call tools\setup_env.bat
```

---

## Step 8: Verify Installation

Check that everything is installed correctly:

```cmd
cd /d F:\vcpkg
vcpkg list
```

You should see:
- `qtbase:x64-windows 6.7.2`
- `zstd:x64-windows`
- `nlohmann-json:x64-windows`
- `blake3:x64-windows`

To verify the Qt installation specifically:

```cmd
dir F:\vcpkg\installed\x64-windows\bin\Qt6Core.dll
dir F:\vcpkg\installed\x64-windows\include\Qt6\QtCore
```

---

## Step 9: Test the Build

After setting up vcpkg and environment variables:

```cmd
cd /d F:\OwnApps\NotYGameRepacker
tools\build.bat
```

---

## Troubleshooting

### Installation Fails

If vcpkg installation fails:

1. Ensure Visual Studio 2026 is installed with C++ workload
2. Try running Command Prompt as Administrator
3. Check internet connection
4. Delete the `F:\vcpkg\vcpkg_installed` folder and retry:

```cmd
cd /d F:\vcpkg
rmdir /s /q vcpkg_installed
vcpkg install --triplet x64-windows
```

### Missing Dependencies

If vcpkg cannot find a package:

```cmd
vcpkg search qtbase
vcpkg search zstd
```

### Slow Compilation

To speed up compilation:

```cmd
vcpkg install --triplet x64-windows --clean-after-build
```

### Environment Variable Not Set

If VCPKG_ROOT is not recognized:

1. Close and reopen Command Prompt
2. Or run:
```cmd
set VCPKG_ROOT=F:\vcpkg
```

### Qt Not Found by CMake

If CMake cannot find Qt:

1. Ensure the CMakePresets.json has the correct path:

```json
"CMAKE_PREFIX_PATH": {
  "type": "STRING",
  "value": "F:/vcpkg/installed/x64-windows"
}
```

2. Or set it manually:
```cmd
cmake -DCMAKE_PREFIX_PATH=F:/vcpkg/installed/x64-windows ..
```

---

## Files Installed

After successful installation, you will have:

```
F:\vcpkg\installed\x64-windows\
├── bin\              - DLL files
│   ├── Qt6Core.dll   - Qt Core
│   ├── Qt6Widgets.dll - Qt Widgets
│   ├── Qt6Gui.dll    - Qt GUI
│   ├── zstd.dll      - Zstandard
│   └── ...
├── include\          - Header files
│   ├── Qt6\          - Qt headers
│   ├── zstd.h        - Zstandard
│   ├── nlohmann\     - JSON library
│   └── blake3.h      - BLAKE3
├── lib\              - Static libraries
│   ├── Qt6Core.lib
│   ├── Qt6Widgets.lib
│   ├── Qt6Gui.lib
│   ├── zstd.lib
│   └── ...
└── share\            - CMake configuration files
    ├── Qt6\          - Qt CMake modules
    ├── zstd\         - Zstandard CMake
    ├── nlohmann_json\ - JSON CMake
    └── blake3\       - BLAKE3 CMake
```

---

## Next Steps

After successful installation:

1. ✅ vcpkg is installed at `F:\vcpkg\`
2. ✅ Dependencies are installed in `F:\vcpkg\installed\x64-windows\`
3. ✅ VCPKG_ROOT environment variable is set
4. ✅ Qt 6.7.2 is available

Now you can build the project:

```cmd
cd /d F:\OwnApps\NotYGameRepacker
tools\build.bat
```

---

## Quick Reference

| Command | Purpose |
|---------|---------|
| `cd /d F:\vcpkg` | Navigate to vcpkg directory |
| `git rev-parse HEAD` | Get current baseline hash |
| `vcpkg install --triplet x64-windows` | Install dependencies |
| `vcpkg list` | List installed packages |
| `set VCPKG_ROOT=F:\vcpkg` | Set environment variable |
| `tools\build.bat` | Build the project |