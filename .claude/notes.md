# KingDubby Revival Notes

## Original Developers
- **Franck Stauffer** - Lead developer (fstauffer)
- **Thomas** - Contributor (Franck will give heads-up)
- **Wolfgang** - Contributor (Franck will give heads-up)
- **Bitplant / Low Coders** - Original release (2008)

## Communication Log

### Feb 24, 2026 - Franck's Response
Franck responded positively to the revival project:

- **Blessing received** - "I don't mind at all"
- **Source code** - Has it backed up on HD in France, will retrieve in **April 2026** and share
- **Will notify** Thomas and Wolfgang about the project
- **Original filtering** - Based on [RBJ Audio EQ Cookbook](https://webaudio.github.io/Audio-EQ-Cookbook/Audio-EQ-Cookbook.txt)

### Franck's Unfinished Ideas (from his email)
Things he wanted to implement but never finished:

1. **Tape flutter** - LFO varying playback speed, affecting delay time and pitch
   - He went down rabbit hole of "continuous playback mechanism" with adjustable playback speed
   - Suggests "pragmatic approach" would work better

2. **Analog noise** - Adding more analog character

3. **Tape frequency response** - More closely reproducing tape characteristics

4. **Platform support** - What JUCE enables (now done: macOS AU/VST3, Windows VST3)

## Technical Notes

### Filtering
- Original used RBJ Audio EQ Cookbook biquad formulas
- Cookbook downloaded to: `Audio-EQ-Cookbook.txt`

### Current Implementation Status
- UI: Using original filmstrip assets
- DSP: Reimplemented (needs refinement)
- Platforms: macOS (AU + VST3), Windows (VST3)
- Build: JUCE 8.x, C++17

### Known DSP Issues (GitHub Issues)

**Phase 1 - Make It Usable (P0):**
- #3: Feedback ceiling (cap at 95%)
- #5: Soft knee limiting (clamp after softClip)
- #7: Clear state on bypass/transport
- #4: Roll off highs (simple LPF)

**Phase 2 - Tape Character:**
- #10: Feedback EQ stack (HPF + shelf) - merged #12 into this
- #14: Standardize biquad implementation

**Phase 3 - Polish:**
- #6: Ducking
- #13: Musical parameter mapping

**Phase 4 - Character (optional):**
- #8: Tape flutter
- #9: Analog noise
- #15: All-pass filter

**Cosmetic (skip for now):**
- #1: Last-frame jump
- #2: 1px offset

**Blocked until April:**
- #11: Integrate original source

### Key Invariant
**Feedback write-back is always clamped to [-0.95, +0.95] after EQ/saturation.**
See `.claude/domain.md` for canonical feedback path order.

## Timeline
- **April 2026** - Franck retrieves original source code from France
- **TBD** - Thomas and Wolfgang notified
