# Building KingDubby for Windows

## Prerequisites

1. **Visual Studio 2022** (Community edition is fine)
   - Install with "Desktop development with C++" workload

2. **JUCE Framework**
   - Install JUCE to `C:\JUCE\`
   - Or update module paths in the .jucer file

3. **Inno Setup 6.x** (for creating installer)
   - Download from https://jrsoftware.org/isinfo.php

## Building the Plugin

### Option 1: Using Projucer

1. Open `KingDubby.jucer` in Projucer
2. Click "Save and Open in IDE" for Visual Studio 2022
3. In Visual Studio:
   - Select "Release" configuration and "x64" platform
   - Build the solution

### Option 2: Command Line

```batch
cd Builds\VisualStudio2022
msbuild KingDubby.sln /p:Configuration=Release /p:Platform=x64
```

## Build Output

After building, the VST3 plugin will be at:
```
Builds\VisualStudio2022\x64\Release\VST3\KingDubby.vst3\
```

## Creating the Installer

1. Open `installers\KingDubby.iss` in Inno Setup
2. Click "Compile" (or press Ctrl+F9)
3. The installer will be created at:
   ```
   installers\KingDubby-1.0.0-Windows.exe
   ```

## Manual Installation (without installer)

Copy the `KingDubby.vst3` folder to:
```
C:\Program Files\Common Files\VST3\
```

## Notes

- The Windows build produces VST3 only (no AU on Windows)
- Tested with Visual Studio 2022 and JUCE 8.x
- Requires C++17 compiler
