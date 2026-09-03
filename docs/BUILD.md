# Building NotY Game Repacker

## Overview

This guide provides step-by-step instructions for building the NotY Game Repacker project from source on Windows 10/11 x64.

## Prerequisites

### Required Software

| Software | Version | Purpose |
|----------|---------|---------|
| **Visual Studio** | 2026 (Community or higher) | C++ compiler and IDE |
| **CMake** | 3.20+ | Build system |
| **vcpkg** | Latest | Package manager |
| **Git** | Latest | Version control (optional) |

### Visual Studio Installation

1. Download Visual Studio 2026 from [Microsoft's website](https://visualstudio.microsoft.com/)
2. Run the installer
3. Select the following workloads:
   - **Desktop development with C++**
   - **Windows 10/11 SDK** (included in the workload)
4. Click **Install**

### CMake Installation

1. Download CMake 3.20+ from [cmake.org](https://cmake.org/download/)
2. Run the installer
3. Select **Add CMake to system PATH** during installation
4. Verify installation:
```cmd
cmake --version
```

### vcpkg Installation

Follow the detailed guide in **[vcpkg.md](vcpkg.md)** for complete setup instructions.

**Quick Summary:**
1. Clone vcpkg to `F:\vcpkg`
2. Bootstrap vcpkg
3. Install dependencies using the global manifest
4. Set `VCPKG_ROOT` environment variable

### Git Installation (Optional)

If you need to update vcpkg or clone the repository:

1. Download Git from [git-scm.com](https://git-scm.com/)
2. Install with default settings
3. Verify installation:
```cmd
git --version
```

## Environment Setup

### Required Environment Variables

| Variable | Value | Description |
|----------|-------|-------------|
| `VCPKG_ROOT` | `F:\vcpkg` | Root directory of vcpkg installation |

### Method 1: Using the Setup Script (Recommended)

The project includes a script to set all required environment variables:

```cmd
call tools\setup_env.bat
```

This script sets:
- `VCPKG_ROOT=F:\vcpkg`
- `QT_DIR=F:\vcpkg\installed\x64-windows`
- Updates `PATH` with vcpkg and Qt bin directories

### Method 2: Manual Command Line (Current Session)

Open Developer Command Prompt and run:

```cmd
set VCPKG_ROOT=F:\vcpkg
set QT_DIR=%VCPKG_ROOT%\installed\x64-windows
set PATH=%VCPKG_ROOT%;%QT_DIR%\bin;%PATH%
```

### Method 3: Permanent System Variables

1. Press `Win + X` and select **System**
2. Click **Advanced system settings**
3. Click **Environment Variables...**
4. Under **System variables**, click **New**
5. Add:
   - **Variable name**: `VCPKG_ROOT`
   - **Variable value**: `F:\vcpkg`
6. Click **OK** twice
7. Restart your command prompt

### Verification

Verify the environment is correctly set:

```cmd
echo %VCPKG_ROOT%
```

Expected output: `F:\vcpkg`

## Building the Project

### Step 1: Open Developer Command Prompt

Open the **Developer Command Prompt for VS 2026**:
- Start Menu → Visual Studio 2026 → Developer Command Prompt for VS 2026

### Step 2: Navigate to Project Root

```cmd
cd /d F:\OwnApps\NotYGameRepacker
```

### Step 3: Set Environment Variables

```cmd
call tools\setup_env.bat
```

### Step 4: Build the Project

#### Option A: Using the Build Script (Recommended)

```cmd
tools\build.bat
```

The script will:
1. Clean previous builds
2. Configure CMake using the preset
3. Build Release configuration
4. Display output locations

#### Option B: Manual Build

**Configure:**
```cmd
cmake --preset default
```

**Build:**
```cmd
cmake --build build --config Release --parallel
```

### Step 5: Verify Build Output

Check that the executables were created:

```cmd
dir build\apps\Repacker\Release\NotYRepacker.exe
dir build\apps\Setup\Release\NotYSetup.exe
```

## Build Configurations

### Release Build (Optimized)

```cmd
cmake --preset default
cmake --build build --config Release --parallel
```

**Output:**
- `build/apps/Repacker/Release/NotYRepacker.exe`
- `build/apps/Setup/Release/NotYSetup.exe`

### Debug Build (With Debug Symbols)

```cmd
cmake --preset debug
cmake --build build --config Debug --parallel
```

**Output:**
- `build/apps/Repacker/Debug/NotYRepacker.exe`
- `build/apps/Setup/Debug/NotYSetup.exe`

### Clean Build

Remove the build directory and rebuild:

```cmd
rmdir /s /q build
tools\build.bat
```

## Build Output Locations

After a successful build, the outputs are located at:

```
F:\OwnApps\NotYGameRepacker\
├── build\
│   ├── apps\
│   │   ├── Repacker\
│   │   │   └── Release\
│   │   │       └── NotYRepacker.exe
│   │   └── Setup\
│   │       └── Release\
│   │           └── NotYSetup.exe
│   └── src\
│       └── Release\
│           └── NotYCore.lib
├── install\          (if CMake install was run)
└── dist\             (after running package.bat)
```

## Running the Applications

### Repacker

```cmd
build\apps\Repacker\Release\NotYRepacker.exe
```

Expected behavior:
- Welcome screen appears
- Fonts load correctly (Rubik family)
- Navigation buttons work
- Directory scanning functions

### Installer

```cmd
build\apps\Setup\Release\NotYSetup.exe
```

Expected behavior:
- Auto-detects package in same directory (if available)
- Shows game information from manifest
- Installation wizard works

## Troubleshooting

### Issue: VCPKG_ROOT not set

**Error:**
```
ERROR: VCPKG_ROOT environment variable not set!
```

**Solution:**
```cmd
call tools\setup_env.bat
```
or
```cmd
set VCPKG_ROOT=F:\vcpkg
```

### Issue: CMake Cannot Find Qt

**Error:**
```
CMake Error: Could not find a package configuration file provided by "Qt6"
```

**Solutions:**

1. Verify Qt is installed via vcpkg:
```cmd
cd /d F:\vcpkg
vcpkg list | findstr qtbase
```

2. Set CMake prefix path:
```cmd
cmake -DCMAKE_PREFIX_PATH="F:\vcpkg\installed\x64-windows" ..
```

3. Check CMakePresets.json has correct paths

### Issue: Link Errors

**Error:**
```
error LNK2019: unresolved external symbol
```

**Solutions:**

1. Verify all dependencies are installed:
```cmd
cd /d F:\vcpkg
vcpkg list
```

2. Clean and rebuild:
```cmd
rmdir /s /q build
tools\build.bat
```

3. Check that vcpkg triplet matches (x64-windows)

### Issue: Build Takes Too Long

**Optimizations:**

1. Use parallel builds:
```cmd
cmake --build build --config Release --parallel
```

2. Increase system memory (8GB+ recommended)

3. Use SSD storage for faster I/O

4. Consider using `--clean-after-build` with vcpkg to reduce disk usage

### Issue: Application Doesn't Start

**Solutions:**

1. Check that all required DLLs are present:
```cmd
dumpbin /dependents NotYRepacker.exe
```

2. Run from command prompt to see error messages

3. Verify Qt deployment:
```cmd
%QT_DIR%\bin\windeployqt6.exe NotYRepacker.exe
```

4. Check for missing fonts in resources

### Issue: Fonts Not Loading

**Solutions:**

1. Verify font files exist in resources:
```cmd
dir resources\fonts\Rubik-*.ttf
```

2. Check resource file is compiled:
```cmd
dir build\apps\Repacker\Release\NotYRepacker.exe
```

3. Run with console to see font loading errors

### Issue: vcpkg Dependencies Missing

**Error:**
```
CMake Error: Could not find a package configuration file provided by "zstd"
```

**Solution:**
```cmd
cd /d F:\vcpkg
vcpkg install zstd --triplet x64-windows
vcpkg install nlohmann-json --triplet x64-windows
vcpkg install blake3 --triplet x64-windows
```

## Build Logs

### Viewing Build Logs

CMake generates logs in the build directory:

```
build\
├── CMakeOutput.log    - CMake configuration output
├── CMakeError.log     - CMake error details
└── *.log              - Compilation logs
```

### Verbose Build

To see detailed build output:

```cmd
cmake --build build --config Release --verbose
```

## Performance Recommendations

### Development Machine

- **RAM**: 8GB minimum, 16GB recommended
- **Storage**: SSD for faster builds
- **CPU**: 4+ cores for parallel compilation

### Build Optimizations

1. **Use Release configuration** for production builds
2. **Enable parallel builds** with `--parallel`
3. **Use vcpkg binary caching** to speed up dependency builds
4. **Keep vcpkg updated** for latest optimizations

## Next Steps

After successfully building the project:

1. **Test the applications** - Verify both repacker and installer work
2. **Create a package** - Use the repacker to create a test package
3. **Test installation** - Use the installer with the created package
4. **Package for distribution** - Run `tools\package.bat` and `tools\deploy.bat`
5. **Review documentation** - Check [USER_GUIDE.md](USER_GUIDE.md) for usage

## Getting Help

### Documentation

- **[vcpkg.md](vcpkg.md)** - vcpkg setup guide
- **[DEPLOYMENT.md](DEPLOYMENT.md)** - Deployment guide
- **[USER_GUIDE.md](USER_GUIDE.md)** - User documentation

### Support

For issues and questions:
- **Email**: support@noty215.com
- **Website**: https://noty215.com

### Common Commands Cheat Sheet

| Command | Description |
|---------|-------------|
| `call tools\setup_env.bat` | Set environment variables |
| `tools\build.bat` | Full build |
| `cmake --preset default` | Configure CMake |
| `cmake --build build --config Release --parallel` | Build release |
| `rmdir /s /q build` | Clean build directory |
| `tools\package.bat` | Create distribution package |
| `tools\deploy.bat` | Deploy Qt dependencies |

---

**Build Version:** 1.0.0
**Last Updated:** 2026-09-02
