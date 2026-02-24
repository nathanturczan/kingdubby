# KingDubby

![KingDubby](Assets/kingdubbysplash.jpg)

A modern rebuild of the classic KingDubby dub delay plugin (Lowcoders & Bitplant, 2004–2008). The original stopped working when macOS dropped 32-bit support. This project recreates it for modern systems.

![UI](Assets/kingdubby_dubbg.png)

## Download

**[Releases](https://github.com/soney/KingDubby/releases)** — macOS (AU/VST3), Windows (VST3)

## Features

- PT2399-style dub delay with degradation
- Bandpass filter in feedback loop (12/24 dB)
- Stereo ping-pong
- Tempo sync

## Building

Requires JUCE 8+, Xcode 14+ (macOS), or Visual Studio 2022 (Windows).

```bash
cd Builds/MacOSX
xcodebuild -scheme "KingDubby - AU" -configuration Release
xcodebuild -scheme "KingDubby - VST3" -configuration Release
```

## Credits

**Original (2004–2008):** Franck Stauffer / Lowcoders (code), Thomas & Wolfgang Merkle / Bitplant (GUI)

**Revival (2026):** [Scale Navigator](https://scalenavigator.com)

## License

Fan revival project. Original UI assets are property of Lowcoders & Bitplant.
