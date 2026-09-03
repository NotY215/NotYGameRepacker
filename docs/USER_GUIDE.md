# NotY Game Repacker - User Guide

## Table of Contents

1. [Overview](#overview)
2. [System Requirements](#system-requirements)
3. [Using the Repacker](#using-the-repacker)
   - [Step 1: Welcome](#step-1-welcome)
   - [Step 2: Source Directory](#step-2-source-directory)
   - [Step 3: Game Cover](#step-3-game-cover)
   - [Step 4: Package Configuration](#step-4-package-configuration)
   - [Step 5: Review](#step-5-review)
   - [Step 6: Repacking Progress](#step-6-repacking-progress)
   - [Step 7: Completion](#step-7-completion)
4. [Using the Installer](#using-the-installer)
   - [Step 1: Welcome](#installer-step-1-welcome)
   - [Step 2: Installation Location](#installer-step-2-installation-location)
   - [Step 3: Select Components](#installer-step-3-select-components)
   - [Step 4: Install](#installer-step-4-install)
   - [Step 5: Verification](#installer-step-5-verification)
   - [Step 6: Complete](#installer-step-6-complete)
5. [Package Format](#package-format)
6. [Performance Tips](#performance-tips)
7. [Troubleshooting](#troubleshooting)
8. [Frequently Asked Questions](#frequently-asked-questions)
9. [Support](#support)

---

## Overview

**NotY Game Repacker** is a professional game packaging system that compresses, encrypts, and packages game files into the `.noty` format. The system includes two main applications:

### NotYRepacker.exe
The repacker application allows you to create `.noty` packages from game directories. It provides:
- Directory scanning with file statistics
- BLAKE3 hashing for file integrity
- Zstandard compression (Level 1-22)
- AES-256-GCM authenticated encryption
- Chunk splitting for large packages
- Professional installer generation
- Cover image support
- Multi-threaded processing

### NotYSetup.exe
The installer application allows end users to install `.noty` packages. It provides:
- Professional dark theme installer
- Game cover display
- Installation directory selection with disk space checking
- Component selection (optional content)
- Real-time progress with ETA
- File integrity verification
- Launch option after installation

---

## System Requirements

### Minimum Requirements
- **OS**: Windows 10 (Build 19041+) or Windows 11
- **Architecture**: x64 (64-bit)
- **RAM**: 4GB (8GB recommended for large packages)
- **Disk Space**: 100MB for the application + space for game packages
- **CPU**: Any modern x64 processor

### Recommended Requirements
- **OS**: Windows 10/11 (64-bit)
- **RAM**: 8GB or more
- **Disk Space**: SSD for faster I/O operations
- **CPU**: Multi-core processor for parallel processing

---

## Using the Repacker

### Starting the Repacker
1. Double-click `NotYRepacker.exe` to launch the application
2. The welcome screen will appear

---

### Step 1: Welcome

The welcome screen displays:
- **Logo**: The NotY Game Repacker logo
- **Title**: "NotY Game Repacker"
- **Subtitle**: "Professional Game Packaging System"
- **Branding**: "Powered by NotY215"
- **Description**: Overview of what the tool does

**Action**: Click **Continue →** to proceed to the Source Directory page.

---

### Step 2: Source Directory

This page allows you to select the game folder you want to repack.

#### Instructions:
1. Click **Browse...** to open a file dialog
2. Navigate to your game folder
3. Select the folder and click **OK**
4. Click **Scan Directory** to analyze the folder
5. Review the directory information:
   - **Files**: Total number of files found
   - **Directories**: Total number of subdirectories
   - **Total size**: Total size of all files

#### Tips:
- Ensure the folder contains all game files
- Hidden files are not included by default (can be enabled in Configuration)
- Large directories may take a few seconds to scan

**Action**: Click **Continue →** to proceed to the Cover page.

---

### Step 3: Game Cover

This page allows you to select a cover image for your game package.

#### Instructions:
1. Click **Browse...** to open a file dialog
2. Navigate to your cover image
3. Select the image and click **OK**
4. The cover will be displayed in the preview area

#### Supported Formats:
- PNG (.png)
- JPEG (.jpg, .jpeg)
- BMP (.bmp)
- GIF (.gif)

#### Tips:
- Recommended resolution: 512x512 or higher
- PNG format is recommended for best quality
- The cover will be embedded in the package as `cover.png`

**Action**: Click **Continue →** to proceed to the Configuration page.

---

### Step 4: Package Configuration

This page contains all the settings for your package.

#### Output Settings

**Output Directory**: Where the package files will be saved
- Click **Browse...** to select a directory
- The directory will be created if it doesn't exist

#### Game Information

| Field | Description | Example |
|-------|-------------|---------|
| **Game Name** | The name of your game (required) | "MyGame" |
| **Game Version** | Version number (optional) | "1.0.0" |
| **Repacker Name** | Your name or username | "JohnDoe" |
| **Setup Name** | The installer filename (must end with .exe) | "MyGameSetup.exe" |

#### Technical Settings

| Setting | Description | Range |
|---------|-------------|-------|
| **Compression** | Zstandard compression level | 1-22 (higher = smaller file, slower) |
| **Chunk Size** | Maximum size per chunk file | 100-4096 MB |
| **Enable Encryption** | AES-256-GCM encryption | On/Off |
| **Include Hidden Files** | Include system-hidden files | On/Off |
| **Generate Setup.exe** | Create an installer for the package | On/Off |

#### Tips:
- **Compression Level**: Level 19 is recommended for good balance
- **Chunk Size**: 1GB is recommended for most packages
- **Encryption**: Recommended for security
- **Hidden Files**: Enable if your game uses hidden files

**Action**: Click **Continue →** to proceed to the Review page.

---

### Step 5: Review

This page displays a summary of all your configuration settings.

#### Review Items:
- **Source Directory**: Your selected game folder
- **Output Directory**: Where the package will be saved
- **Game Name**: The game title
- **Game Version**: Version number
- **Repacker**: Your name
- **Setup Name**: Installer filename
- **Cover Image**: Selected cover image
- **Compression**: Zstandard level
- **Chunk Size**: Maximum chunk size
- **Encryption**: Enabled/Disabled
- **Hidden Files**: Included/Not included
- **Generate Setup**: Enabled/Disabled

#### Important:
- **Read all settings carefully before proceeding**
- A warning banner reminds you to review everything
- Go back if you need to change anything

**Action**: Click **Continue →** to start the repacking process.

---

### Step 6: Repacking Progress

This page shows the progress of the repacking operation.

#### Progress Indicators:
- **Progress Bar**: Overall completion percentage
- **Status Label**: Current operation being performed
- **Progress Log**: Detailed log of all operations

#### Operations Performed:
1. **Scanning**: Reading all files from the source directory
2. **Hashing**: Calculating BLAKE3 hashes for each file
3. **Compressing**: Compressing files with Zstandard
4. **Encrypting**: Encrypting with AES-256-GCM (if enabled)
5. **Writing Chunks**: Saving chunk files to disk
6. **Writing Manifest**: Creating the manifest.json file
7. **Writing Cover**: Saving the cover image
8. **Generating Setup**: Creating the installer (if enabled)

#### Cancellation:
- Click **Cancel Repack** to stop the operation
- The operation will clean up and return to the Review page

**Action**: Wait for the repacking to complete.

---

### Step 7: Completion

This page displays the completion status of the repacking process.

#### Success Message:
- **✓ Repack Complete!**
- **Details**: "Your package has been successfully created"
- **Package Size**: Shows the total package size
- **Output Directory**: Shows where the package was saved

#### Output Files:
| File | Description |
|------|-------------|
| `manifest.noty` | Package manifest (JSON format) |
| `GameName.001.noty` | First chunk file |
| `GameName.002.noty` | Additional chunk files |
| `cover.png` | Cover image |
| `MyGameSetup.exe` | Installer (if enabled) |

**Action**: Click **Finish** to exit the repacker.

---

## Using the Installer

### Starting the Installer
1. Copy the entire package directory to the target system
2. Double-click `Setup.exe` (or the name you specified)
3. The installer will auto-detect the package

---

### Installer Step 1: Welcome

The welcome screen displays:
- **Logo**: Game icon or NotY logo
- **Game Name**: The game title from the manifest
- **Repacker**: "Repacked by [Your Name]"
- **Description**: Welcome message

**Action**: Click **Continue →** to proceed.

---

### Installer Step 2: Installation Location

This page allows you to choose where to install the game.

#### Instructions:
1. **Installation Directory**: Path where the game will be installed
2. Click **Browse...** to select a custom location
3. **Disk Space**: Shows required and available space

#### Tips:
- Default location: `%USERPROFILE%\Games\GameName`
- Ensure enough disk space is available
- The directory will be created if it doesn't exist

#### Validation:
- The installer checks if the directory is writable
- Checks for sufficient disk space
- Warns if space is insufficient

**Action**: Click **Continue →** to proceed.

---

### Installer Step 3: Select Components

This page allows you to choose which components to install.

#### Components:
- **Game Files (Required)**: Core game files - cannot be deselected
- **Additional Content**: Optional game content
- **Documentation**: Manuals and readme files

#### Instructions:
1. Click on a component to select/deselect it
2. The total size will update accordingly

**Action**: Click **Continue →** to proceed to the Install page.

---

### Installer Step 4: Install

This page shows the installation progress.

#### Progress Indicators:
- **Progress Bar**: Overall installation percentage
- **Status Label**: Current operation being performed
- **File Progress**: Current file being extracted
- **ETA**: Estimated time remaining (when available)
- **Throughput**: Current extraction speed

#### Operations Performed:
1. **Preparing**: Creating installation directory
2. **Extracting**: Extracting files from chunks
3. **Decompressing**: Decompressing Zstandard data
4. **Decrypting**: Decrypting AES-256-GCM (if enabled)
5. **Verifying**: Validating file integrity

**Action**: Click **Install** to begin the installation.

---

### Installer Step 5: Verification

This page shows the file verification progress.

#### Verification Process:
1. Checking each file exists
2. Validating file sizes
3. Verifying BLAKE3 hashes
4. Reporting any mismatches

**Action**: Wait for verification to complete.

---

### Installer Step 6: Complete

This page displays the installation completion status.

#### Success Message:
- **✓ Installation Complete!**
- **Details**: "The game has been successfully installed"
- **Launch Option**: "Launch game when finished" (checked by default)

#### If Failed:
- **✗ Installation Failed**
- **Error Message**: Details of what went wrong

**Action**: 
- Click **Finish** to exit
- Uncheck launch option if you don't want to launch the game

---

## Package Format

### Directory Structure

```
Package/
├── manifest.noty          # Package manifest (JSON)
├── GameName.001.noty      # Chunk 1
├── GameName.002.noty      # Chunk 2
├── cover.png              # Cover image
└── MyGameSetup.exe        # Installer (optional)
```

### Manifest Format

The manifest is a JSON file containing all package metadata:

```json
{
  "packageInfo": {
    "packageId": "NOTY-1234567890",
    "gameName": "MyGame",
    "gameVersion": "1.0.0",
    "repackerName": "JohnDoe",
    "setupName": "MyGameSetup.exe",
    "coverFormat": "png",
    "coverSize": 1048576,
    "originalSize": 1234567890,
    "compressedSize": 987654321,
    "compressionRatio": 80,
    "chunkCount": 2,
    "fileCount": 1000,
    "compressionMethod": "Zstandard",
    "encryptionMethod": "AES-256-GCM",
    "hashAlgorithm": "BLAKE3",
    "formatVersion": 1,
    "createdBy": "NotY Repacker v1.0.0"
  },
  "files": [
    {
      "path": "game.exe",
      "size": 12345678,
      "hash": "abc123...",
      "compressedSize": 9876543,
      "chunkId": 1,
      "offsetInChunk": 0
    }
  ],
  "chunks": [
    {
      "id": 1,
      "filename": "MyGame.001.noty",
      "compressedSize": 500000000,
      "uncompressedSize": 600000000,
      "fileCount": 500,
      "checksum": "def456..."
    }
  ],
  "components": [
    {
      "name": "Additional Content",
      "description": "Extra game content",
      "size": 100000000,
      "isRequired": false,
      "filePatterns": ["content/*"]
    }
  ]
}
```

### Chunk Format

Each chunk file contains:
- **Magic**: "NOTY" (4 bytes)
- **Version**: Format version (4 bytes)
- **Chunk ID**: Sequential chunk number (4 bytes)
- **Total Chunks**: Total number of chunks (4 bytes)
- **Compression**: "ZSTD" or "NONE" (4 bytes)
- **Encryption**: "AES-GCM" or "NONE" (4 bytes)
- **Uncompressed Size**: Size before compression (8 bytes)
- **Compressed Size**: Size after compression (8 bytes)
- **Checksum**: BLAKE3 hash of the payload (32 bytes)
- **Payload**: Compressed and encrypted file data

---

## Performance Tips

### For Faster Repacking

1. **Use SSD Storage**
   - Source and output directories on SSD
   - Significantly improves I/O performance

2. **Optimize Chunk Size**
   - Larger chunks = fewer files = faster processing
   - 2GB+ chunks for large games

3. **Adjust Compression Level**
   - Level 1-10: Fast, larger files
   - Level 11-19: Balanced (recommended)
   - Level 20-22: Slow, smallest files

4. **Disable Encryption**
   - Encryption adds overhead
   - Only use if security is required

5. **Close Other Applications**
   - Free up CPU and memory
   - Faster processing with more resources

### Memory Usage Optimization

The system automatically adapts based on available RAM:

| RAM | Buffer Size | Threads | Performance |
|-----|-------------|---------|-------------|
| < 8GB | 512 KB | Half cores | Conservative |
| 8-16GB | 1 MB | Cores - 1 | Moderate |
| 16-32GB | 2 MB | All cores | High |
| 32+GB | 4 MB | All cores | Aggressive |

### Network Deployment

1. **Chunk Distribution**
   - Split large packages into chunks
   - Distribute chunks separately
   - Verify checksums after transfer

2. **Compression for Transfer**
   - Use higher compression for network transfer
   - Balance compression time vs file size

3. **Verification**
   - Always verify files after transfer
   - Use the manifest for validation

---

## Troubleshooting

### Common Errors

#### "Invalid Game Name"
- **Cause**: Game name is empty or too short
- **Solution**: Enter at least 2 characters
- **Tip**: Avoid special characters in the name

#### "Insufficient Disk Space"
- **Cause**: Not enough space on the target drive
- **Solution**: Free up space or choose a different directory
- **Tip**: Check disk space before starting

#### "Failed to open file"
- **Cause**: File permissions or file in use
- **Solution**: Check file permissions
- **Tip**: Close any applications using the file

#### "Compression failed"
- **Cause**: Disk space, permissions, or corrupted file
- **Solution**: Check disk space and file integrity
- **Tip**: Try a lower compression level

#### "Decryption failed"
- **Cause**: Corrupted package or incorrect key
- **Solution**: Verify package integrity with manifest
- **Tip**: Re-download the package if corrupted

#### "Hash mismatch"
- **Cause**: File was modified or corrupted
- **Solution**: Re-verify the file integrity
- **Tip**: Re-download the affected chunk

### General Troubleshooting

#### Repacker Won't Start
1. Check if Visual C++ Redistributable is installed
2. Verify all Qt DLLs are present
3. Check for anti-virus false positives
4. Run as administrator

#### Installer Won't Start
1. Check if `manifest.noty` exists
2. Verify at least one `.noty` chunk exists
3. Check file permissions
4. Run as administrator

#### Slow Performance
1. Close other applications
2. Use SSD for source/output directories
3. Reduce compression level
4. Increase chunk size
5. Check system resources (CPU, RAM, Disk)

#### Package Verification Fails
1. Re-run the repacker
2. Check disk space
3. Verify source files are not corrupted
4. Check for anti-virus interference

### Logging

The applications log to the console:
- **Info**: Normal operations
- **Warning**: Non-critical issues
- **Error**: Critical failures

For detailed logs:
1. Run from command prompt
2. Redirect output to a file: `NotYRepacker.exe > log.txt 2>&1`
3. Check the log file for errors

---

## Frequently Asked Questions

### General

**Q: What is the .noty format?**
A: A proprietary format optimized for game distribution with compression, encryption, and integrity verification.

**Q: Can I use this for commercial games?**
A: Yes, the NotY Game Repacker is designed for professional use.

**Q: Is the encryption secure?**
A: Yes, AES-256-GCM is a military-grade encryption standard.

**Q: Can I customize the installer?**
A: The installer uses a professional dark theme. Customization options may be added in future versions.

### Repacker

**Q: What compression level should I use?**
A: Level 19 is recommended for the best balance of speed and compression ratio.

**Q: What chunk size should I use?**
A: 1GB is recommended for most use cases. Larger chunks mean fewer files.

**Q: Can I repack without encryption?**
A: Yes, simply disable encryption in the configuration.

**Q: How long does repacking take?**
A: Depends on file size, compression level, and hardware. Typically 10-60 minutes for a 10GB game.

### Installer

**Q: Can I install without administrator rights?**
A: Yes, if the installation directory is writable by the user.

**Q: Does the installer support silent installation?**
A: Not currently, but this may be added in future versions.

**Q: Can I choose which components to install?**
A: Yes, the installer supports component selection.

**Q: What happens if verification fails?**
A: The installer will report which files failed verification and stop the installation.

---

## Support

### Contact Information

- **Email**: support@noty215.com
- **Website**: https://noty215.com
- **GitHub**: https://github.com/NotY215/NotYGameRepacker

### Reporting Issues

When reporting issues, please include:
1. **Version**: The version of the application
2. **OS**: Windows version (e.g., Windows 11 22H2)
3. **Hardware**: CPU, RAM, disk type
4. **Steps to Reproduce**: Detailed steps
5. **Log File**: Console output or log file
6. **Package Size**: Approximate size of the game

### Feature Requests

We welcome feature requests:
1. Describe the feature clearly
2. Explain why it's useful
3. Provide examples if possible

---

## Changelog

### Version 1.0.0
- Initial release
- Full repacker functionality
- Full installer functionality
- BLAKE3 hashing
- Zstandard compression
- AES-256-GCM encryption
- Professional dark theme
- Component support
- Performance optimization

---

**Thank you for using NotY Game Repacker!**

© 2026 NotY215 - All Rights Reserved
