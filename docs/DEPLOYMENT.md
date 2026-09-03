# NotY Game Repacker - Deployment Guide

## Overview

This guide explains how to deploy the NotY Game Repacker system for distribution to end users. The deployment process creates a self-contained package that includes the repacker application, installer, and all required dependencies.

## Deployment Methods

### Method 1: Automated Deployment (Recommended)

Use the provided deployment scripts for automated deployment:

```cmd
# Step 1: Build the project
tools\build.bat

# Step 2: Package the executables
tools\package.bat

# Step 3: Deploy Qt dependencies
tools\deploy.bat
```

### Method 2: Manual Deployment

Follow these steps for manual deployment:

#### Step 1: Build the Project

```cmd
# Install dependencies
%VCPKG_ROOT%\vcpkg install --triplet x64-windows

# Configure CMake
cmake --preset default

# Build Release configuration
cmake --build build --config Release --parallel
```

#### Step 2: Collect Executables

Create a distribution directory and copy the executables:

```cmd
mkdir dist

# Copy Repacker
copy build\apps\Repacker\Release\NotYRepacker.exe dist\

# Copy Setup
copy build\apps\Setup\Release\NotYSetup.exe dist\
```

#### Step 3: Deploy Qt Dependencies

Run windeployqt on each executable:

```cmd
cd dist

# Deploy Repacker
windeployqt6 --no-compiler-runtime --no-system-d3d-compiler --no-opengl-sw --verbose 0 NotYRepacker.exe

# Deploy Setup
windeployqt6 --no-compiler-runtime --no-system-d3d-compiler --no-opengl-sw --verbose 0 NotYSetup.exe
```

#### Step 4: Copy Resources

Copy required resources to the distribution directory:

```cmd
# Copy fonts
xcopy ..\resources\fonts fonts\ /E /I

# Copy logo
copy ..\resources\logo.png

# Copy documentation
xcopy ..\docs docs\ /E /I

# Copy README
copy ..\README.md
```

#### Step 5: Create Version File

Create a version information file:

```cmd
echo NotY Game Repacker v1.0.0 > version.txt
echo Build Date: %DATE% >> version.txt
echo Build Time: %TIME% >> version.txt
```

#### Step 6: Create Distribution Archive

Create a zip archive of the distribution directory:

```cmd
powershell -Command "Compress-Archive -Path 'dist\*' -DestinationPath 'NotYRepacker-v1.0.0.zip' -Force"
```

## Distribution Package Structure

### Required Files

```
dist/
├── NotYRepacker.exe           # Repacker application
├── NotYSetup.exe              # Installer application
├── logo.png                   # Application logo
├── version.txt                # Version information
├── README.md                  # Quick start guide
│
├── fonts/                     # Required fonts
│   ├── Rubik-Black.ttf
│   ├── Rubik-Bold.ttf
│   ├── Rubik-ExtraBold.ttf
│   ├── Rubik-Medium.ttf
│   ├── Rubik-Regular.ttf
│   └── Rubik-SemiBold.ttf
│
├── docs/                      # Documentation
│   ├── BUILD.md
│   ├── DEPLOYMENT.md
│   └── USER_GUIDE.md
│
└── [Qt DLLs]                  # Qt dependencies (deployed by windeployqt)
    ├── Qt6Core.dll
    ├── Qt6Gui.dll
    ├── Qt6Widgets.dll
    ├── platforms/
    │   └── qwindows.dll
    ├── styles/
    │   └── qwindowsvistastyle.dll
    └── [additional Qt plugins]
```

### Optional Files

```
dist/
├── examples/                  # Example package files
│   ├── manifest.noty
│   └── GameName.001.noty
│
└── tools/                     # Additional tools
    ├── verify_package.exe
    └── extract_chunks.exe
```

## System Requirements

### Minimum Requirements

- **OS**: Windows 10 (Build 19041+) or Windows 11 (64-bit)
- **CPU**: x64 processor with SSE2 support
- **RAM**: 4GB (8GB recommended)
- **Disk**: 100MB for application + space for game packages
- **Other**: Visual C++ Redistributable 2022

### Recommended Requirements

- **OS**: Windows 10/11 (64-bit) with latest updates
- **CPU**: Multi-core processor (4+ cores)
- **RAM**: 8GB or more
- **Disk**: SSD for faster I/O operations
- **Other**: Latest Visual C++ Redistributable

## Deployment Scenarios

### Scenario 1: End User Deployment

Deploy to end users who will install and play games:

```
1. Package the dist/ directory as a zip file
2. Upload to your distribution platform (website, Steam, etc.)
3. Users download and extract to a folder of their choice
4. Users run NotYSetup.exe to install games
5. Advanced users can run NotYRepacker.exe to create packages
```

### Scenario 2: Developer Deployment

Deploy to developers who will create game packages:

```
1. Package the dist/ directory as a zip file
2. Distribute to your development team
3. Developers extract to a known location
4. Developers use NotYRepacker.exe to create .noty packages
5. Generated Setup.exe can be included with game packages
```

### Scenario 3: Standalone Installer Deployment

Deploy the Setup.exe as a standalone installer:

```
1. Copy NotYSetup.exe and required DLLs to a package directory
2. Place manifest.noty and .noty chunk files in the same directory
3. Users run NotYSetup.exe directly
4. The installer auto-detects the package files
```

## Verifying Deployment

### Test Checklist

- [ ] Both executables launch without errors
- [ ] Fonts are loaded correctly (Rubik family)
- [ ] Logo displays in both applications
- [ ] Repacker can scan directories
- [ ] Repacker can create .noty packages
- [ ] Setup can install .noty packages
- [ ] File verification works correctly
- [ ] Progress bars and status updates display properly
- [ ] Cancellation works during long operations

### Common Deployment Issues

#### Issue: Application fails to start with "missing DLL" error

**Solution**: Run windeployqt on the executable to deploy Qt dependencies.

#### Issue: Fonts not loading

**Solution**: Ensure the `fonts/` directory is in the same location as the executable and contains all Rubik font files.

#### Issue: Logo not displaying

**Solution**: Ensure `logo.png` is in the same location as the executable.

#### Issue: Installer cannot find manifest

**Solution**: Place `manifest.noty` and `.noty` chunk files in the same directory as `NotYSetup.exe`.

## Security Considerations

### Code Signing

For production releases, sign your executables with a trusted certificate:

```cmd
signtool sign /a /t http://timestamp.digicert.com /fd SHA256 NotYRepacker.exe
signtool sign /a /t http://timestamp.digicert.com /fd SHA256 NotYSetup.exe
```

### Anti-Virus Exclusions

Some anti-virus software may flag the applications due to compression/encryption. Provide users with instructions to add exceptions:

1. Windows Defender: Add exclusion for the application directory
2. Third-party AV: Add to trusted applications list

## Updating Deployment

### Version Updates

1. Update version in `CMakeLists.txt`
2. Update version in `common/Constants.cpp`
3. Rebuild the project
4. Create new deployment package

### Hotfix Updates

For minor fixes without full deployment:

1. Replace specific executables
2. Update version.txt with hotfix information
3. Notify users of the update

## Support

For deployment issues, contact:

- **Email**: support@noty215.com
- **Website**: https://noty215.com/support
- **Documentation**: https://noty215.com/docs

## Troubleshooting Checklist

### Pre-Deployment

- [ ] Project builds successfully in Release configuration
- [ ] All unit tests pass (if applicable)
- [ ] Qt dependencies are correctly deployed
- [ ] Resource files are in correct locations

### Post-Deployment

- [ ] Application runs on target system
- [ ] All features function as expected
- [ ] Performance is acceptable on target hardware
- [ ] Error messages are user-friendly
- [ ] Documentation is complete and accessible

## Next Steps

After successful deployment:

1. **Monitor**: Track application usage and error reports
2. **Collect Feedback**: Gather user feedback for improvements
3. **Plan Updates**: Schedule regular updates and feature additions
4. **Maintain**: Keep dependencies up-to-date
5. **Document**: Update documentation based on user feedback

---

*Documentation Version: 1.0.0*
*Last Updated: 2026-09-02*
