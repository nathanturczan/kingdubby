# KingDubby Domain Knowledge

## Feedback Loop Invariants

### INVARIANT: Feedback Write-Back Ceiling
**The value written back into the delay buffer from the feedback path is always bounded: `abs(feedbackWrite) <= FB_WRITE_LIMIT`.**

This is the final safety net. It lives at the END of the feedback chain, right before writing to the delay buffer.

```cpp
static constexpr float FB_WRITE_LIMIT = 0.95f;

// AFTER EQ stack, AFTER softClip, BEFORE buffer write:
feedbackL = juce::jlimit(-FB_WRITE_LIMIT, FB_WRITE_LIMIT, feedbackL);
feedbackR = juce::jlimit(-FB_WRITE_LIMIT, FB_WRITE_LIMIT, feedbackR);

delayBufferL[wp] = dryL + feedbackL;
delayBufferR[wp] = dryR + feedbackR;
```

**Why 0.95?** Leaves headroom for dry signal addition while guaranteeing decay. Exact number is taste, but the invariant is the real win.

### How #3 and #5 Work Together
- **#3 (parameter ceiling):** Caps feedback *coefficient* so knob can't exceed ~0.95 effective gain
- **#5 (write-back ceiling):** Clamps the actual *signal* being written back, preventing spikes regardless of content

Both are needed. #3 prevents over-unity gain. #5 catches anything that slips through (transients, filter resonance, etc).

---

## Canonical Feedback Path Order

```
┌─────────────────────────────────────────────────────────────────┐
│ READ        │ delayedL/R = readDelay(buffer, time)              │
├─────────────┼───────────────────────────────────────────────────┤
│ DEGRADE     │ [optional] sample-hold + degradeLPF               │
├─────────────┼───────────────────────────────────────────────────┤
│ USER BPF    │ filterL1/R1 (user-controlled freq/BW)             │
│             │ [optional] filterL2/R2 (24dB mode)                │
├─────────────┼───────────────────────────────────────────────────┤
│ CROSSFEED   │ ping-pong: crossL = filteredR * panRL             │
├─────────────┼───────────────────────────────────────────────────┤
│ GAIN        │ (filtered + cross) * feedback                     │
├─────────────┼───────────────────────────────────────────────────┤
│ EQ STACK    │ HPF (~80Hz) then LPF/shelf (~6-8kHz)              │
│             │ Shape accumulated signal BEFORE saturation        │
├─────────────┼───────────────────────────────────────────────────┤
│ SOFTCLIP    │ tanh(x) - musical saturation                      │
├─────────────┼───────────────────────────────────────────────────┤
│ CEILING     │ jlimit(-FB_WRITE_LIMIT, FB_WRITE_LIMIT, x)        │
│             │ INVARIANT - final safety clamp                    │
├─────────────┼───────────────────────────────────────────────────┤
│ WRITE       │ buffer[wp] = dryInput + feedbackL                 │
└─────────────┴───────────────────────────────────────────────────┘
```

**Why this order:**
- EQ stack shapes the accumulated signal before saturation turns buildup into ugliness
- tanh adds musical character
- Hard ceiling guarantees stability and prevents last-mile spikes
- Dry input goes into buffer even at low mix (common design, but makes stability fixes critical)

**Do not reorder these stages.** The invariant (CEILING) must always be last before WRITE.

---

## Debug Logging (Once Per Init)

Log these values once in `prepareToPlay()` or on param change, not per-sample:

```cpp
DBG("KingDubby DSP Config:");
DBG("  FB_WRITE_LIMIT: " + String(FB_WRITE_LIMIT));
DBG("  feedback coeff: " + String(feedback));
DBG("  HPF cutoff: " + String(hpfCutoff) + " Hz");
DBG("  LPF cutoff: " + String(lpfCutoff) + " Hz");
DBG("  sample rate: " + String(sampleRate));
```

Makes it easy to confirm you're testing what you think you're testing.

---

## Reset Events

Call `reset()` (clear buffers + filter state) on:
1. `prepareToPlay()`
2. Transport stopped→playing transition
3. Plugin un-bypassed (if detectable)

This prevents ghost feedback and makes A/B testing reliable.

---

## RBJ Biquad Implementation

All filters use RBJ Audio EQ Cookbook formulas. See `Audio-EQ-Cookbook.txt` in repo.

**Rules:**
- Always normalize coefficients (a0 = 1)
- Use Direct Form 1
- Reset filter state (x1, x2, y1, y2 = 0) in `reset()`
- Handle denormals if needed

---

## Parameter Ranges (Current)

| Param | Range | Notes |
|-------|-------|-------|
| TIME | 1-96 | Note divisions (24 = quarter note) |
| FEEDBACK | 0-100 | Maps to 0.0-0.95 internally (NOT 1.1!) |
| DEGRAD | 0-100 | PT2399-style degradation |
| FILTER_FREQ | 300-3000 Hz | User BPF center |
| FILTER_BW | 0-4 | Q mapping |
| GAIN | -12 to +12 dB | Output gain |
| PAN_LR/RL | 0-100 | Crossfeed amounts |
| MIX | 0-100 | Dry/wet |

---

## Phase 1 Priority (Make It Usable)

```
#3 (ceiling) → #5 (limiter spec) → #7 (reset on transport) → #4 (HF rolloff)
```

#7 comes early because deterministic resets make all other changes easier to verify.
