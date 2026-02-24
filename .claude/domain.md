# KingDubby Domain Knowledge

## Feedback Loop Invariants

### INVARIANT: Feedback Write-Back Ceiling
**Regardless of user params, the feedback write-back sample is limited to [-0.95, +0.95] after EQ/saturation.**

This is the final safety net. It lives at the END of the feedback chain, right before writing to the delay buffer.

```cpp
// AFTER softClip, BEFORE buffer write:
feedbackL = std::clamp(feedbackL, -0.95f, 0.95f);  // CEILING
feedbackR = std::clamp(feedbackR, -0.95f, 0.95f);

delayBufferL[wp] = dryL + feedbackL;
delayBufferR[wp] = dryR + feedbackR;
```

**Why 0.95?** Leaves headroom for dry signal addition while guaranteeing decay.

---

## Canonical Feedback Path Order

```
┌─────────────────────────────────────────────────────────────────┐
│ READ        │ delayedL/R = readDelay(buffer, time)              │
├─────────────┼───────────────────────────────────────────────────┤
│ DEGRADE     │ [optional] sample-hold + degradeLPF               │
├─────────────┼───────────────────────────────────────────────────┤
│ HPF         │ ~80Hz, Q=0.707 - prevents bass buildup            │
├─────────────┼───────────────────────────────────────────────────┤
│ USER BPF    │ filterL1/R1 (user-controlled freq/BW)             │
│             │ [optional] filterL2/R2 (24dB mode)                │
├─────────────┼───────────────────────────────────────────────────┤
│ LPF/SHELF   │ ~6-8kHz shelf or LPF - darkens repeats            │
├─────────────┼───────────────────────────────────────────────────┤
│ CROSSFEED   │ ping-pong: crossL = filteredR * panRL             │
├─────────────┼───────────────────────────────────────────────────┤
│ GAIN        │ (filtered + cross) * feedback                     │
├─────────────┼───────────────────────────────────────────────────┤
│ SOFTCLIP    │ tanh(x) - musical saturation                      │
├─────────────┼───────────────────────────────────────────────────┤
│ CEILING     │ clamp(x, -0.95, +0.95) - INVARIANT                │
├─────────────┼───────────────────────────────────────────────────┤
│ WRITE       │ buffer[wp] = dryInput + feedbackL                 │
└─────────────┴───────────────────────────────────────────────────┘
```

**Do not reorder these stages.** The invariant (CEILING) must always be last before WRITE.

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
