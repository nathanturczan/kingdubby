# KingDubby Revival

A modern rebuild of the classic KingDubby dub tape delay plugin, originally created by **Lowcoders & Bitplant** (2004-2008).

The original plugin stopped working when macOS dropped 32-bit support in Catalina (2019). This project recreates it from scratch for modern systems.

## Features

- **PT2399-style dub delay** with degradation at longer delay times
- **Bandpass filter** in feedback loop (-12dB/-24dB per octave)
- **Stereo ping-pong** delay
- **Tempo sync** (reads BPM from host)
- Original UI graphics

## Parameters

| Section | Parameter | Range |
|---------|-----------|-------|
| **DELAY** | Time | 1-96 (note divisions) |
| | Feedback | 0-100% |
| | Degrad | 0-100% |
| **FILTER** | Type | 12/24 dB/oct |
| | Frequency | 300-3000 Hz |
| | Bandwidth | 0.0-4.0 Q |
| **OUTPUT** | Gain | -12 to +12 dB |
| | Pan L-R | 0-100% |
| | Pan R-L | 0-100% |
| | Mix | Dry-Wet |

## Building

Requires:
- JUCE 7+ (with juce_dsp module)
- Xcode 14+
- macOS 12+

```bash
# Generate Xcode project
/path/to/Projucer --resave KingDubby.jucer

# Build
cd Builds/MacOSX
xcodebuild -scheme "KingDubby - AU" -configuration Release
xcodebuild -scheme "KingDubby - VST3" -configuration Release
```

## Credits

### Original Plugin (2004-2008)
- **Programming:** Franck Stauffer / Lowcoders
- **GUI Design:** Thomas & Wolfgang Merkle / Bitplant

### Revival (2026)
- Reverse engineering and rebuild

## References

- [KVR Audio - KingDubby](https://www.kvraudio.com/product/kingdubby_by_lowcoders)
- [YouTube Demo](https://www.youtube.com/watch?v=JTKkdlzViho)
- [ElectroSmash - PT2399 Analysis](https://www.electrosmash.com/pt2399-analysis)

## License

This is a fan revival project. Original UI assets are property of Lowcoders & Bitplant. Reaching out to original developers for permission.
