#include "DubDelay.h"
#include <algorithm>

namespace
{
    constexpr double kTwoPi = 6.283185307179586;
    constexpr double kLn2Over2 = 0.34657359027997264;
    constexpr float kMinDelaySamples = 4.0f;       // cubic read needs 3 written samples ahead
    constexpr float kDelaySmoothSeconds = 0.045f;  // glide when the delay time changes
    constexpr float kGainSmoothSeconds = 0.010f;

    float clamp01(float x) { return std::clamp(x, 0.0f, 1.0f); }
}

void DubDelay::BandPass::setCoefficients(double sampleRate, double freq, double octaves)
{
    freq = std::clamp(freq, 20.0, sampleRate * 0.45);

    const double w0 = kTwoPi * freq / sampleRate;
    const double sine = std::sin(w0);
    const double alpha = sine * std::sinh(kLn2Over2 * octaves * w0 / sine);
    const double a0 = 1.0 + alpha;

    // b0 = alpha, b1 = 0, b2 = -alpha (normalised by a0)
    b0 = alpha / a0;
    a1 = -2.0 * std::cos(w0) / a0;
    a2 = (1.0 - alpha) / a0;
}

void DubDelay::prepare(double sampleRate)
{
    currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;

    bufferSize = static_cast<int>(std::ceil(currentSampleRate * MAX_DELAY_SECONDS)) + 8;
    delayBufferL.assign(static_cast<size_t>(bufferSize), 0.0f);
    delayBufferR.assign(static_cast<size_t>(bufferSize), 0.0f);

    delaySmoothCoeff = 1.0f - std::exp(-1.0f / (kDelaySmoothSeconds * static_cast<float>(currentSampleRate)));
    gainSmoothCoeff = 1.0f - std::exp(-1.0f / (kGainSmoothSeconds * static_cast<float>(currentSampleRate)));

    filtersDirty = true;
    reset();
}

void DubDelay::reset()
{
    std::fill(delayBufferL.begin(), delayBufferL.end(), 0.0f);
    std::fill(delayBufferR.begin(), delayBufferR.end(), 0.0f);
    writePos = 0;

    filterL1.reset();
    filterR1.reset();
    filterL2.reset();
    filterR2.reset();

    // Jump to the current settings on the next block instead of gliding
    snapOnNextProcess = true;
}

void DubDelay::updateFilters()
{
    filterL1.setCoefficients(currentSampleRate, filterFreq, filterOctaves);

    for (auto* f : { &filterR1, &filterL2, &filterR2 })
    {
        f->b0 = filterL1.b0;
        f->a1 = filterL1.a1;
        f->a2 = filterL1.a2;
    }

    filtersDirty = false;
}

void DubDelay::updateGains()
{
    // Equal-power crossfades, as in the original
    sFeedback.target = feedback;
    sFiltered.target = std::sqrt(degradation);
    sClean.target = std::sqrt(1.0f - degradation);
    sDry.target = std::sqrt(1.0f - mix);
    sWet.target = std::sqrt(mix) * outputGain;
    sLeftToLeft.target = std::sqrt(1.0f - panLR);
    sLeftToRight.target = std::sqrt(panLR);
    sRightToRight.target = std::sqrt(1.0f - panRL);
    sRightToLeft.target = std::sqrt(panRL);
}

float DubDelay::safetyLimit(float x)
{
    // Transparent up to full scale, then bends towards +/-2. The loop gain is
    // already below unity, so this only acts on sustained resonant build-up
    // or input above full scale.
    const float a = std::abs(x);
    if (a <= 1.0f)
        return x;

    return std::copysign(1.0f + std::tanh(a - 1.0f), x);
}

void DubDelay::process(float* left, float* right, int numSamples)
{
    if (left == nullptr || bufferSize == 0)
        return;

    if (filtersDirty)
        updateFilters();

    updateGains();

    if (snapOnNextProcess)
    {
        delaySamples = targetDelaySamples;
        for (auto* s : { &sFeedback, &sFiltered, &sClean, &sDry, &sWet,
                         &sLeftToLeft, &sLeftToRight, &sRightToRight, &sRightToLeft })
            s->snap();
        snapOnNextProcess = false;
    }

    const float gc = gainSmoothCoeff;

    for (int i = 0; i < numSamples; ++i)
    {
        delaySamples += (targetDelaySamples - delaySamples) * delaySmoothCoeff;

        const float fb = sFeedback.next(gc);
        const float filtered = sFiltered.next(gc);
        const float clean = sClean.next(gc);
        const float dry = sDry.next(gc);
        const float wet = sWet.next(gc);
        const float ll = sLeftToLeft.next(gc);
        const float lr = sLeftToRight.next(gc);
        const float rr = sRightToRight.next(gc);
        const float rl = sRightToLeft.next(gc);

        const float inL = left[i];
        const float inR = right != nullptr ? right[i] : inL;

        const float readL = readDelay(delayBufferL, delaySamples);
        const float readR = right != nullptr ? readDelay(delayBufferR, delaySamples) : readL;

        // Bandpass the repeats, then blend filtered against clean (DEGRAD)
        float bandL = filterL1.process(readL);
        float bandR = right != nullptr ? filterR1.process(readR) : bandL;

        if (filter24dB)
        {
            bandL = filterL2.process(bandL);
            bandR = right != nullptr ? filterR2.process(bandR) : bandL;
        }

        bandL = filtered * bandL + clean * readL;
        bandR = filtered * bandR + clean * readR;

        // The filter peaks at 0 dB, so the loop peaks at feedback * (filtered + clean).
        // Scale so that peak is MAX_LOOP_GAIN with the knob at 100.
        const float loopGain = fb * MAX_LOOP_GAIN / (filtered + clean);

        delayBufferL[static_cast<size_t>(writePos)] = inL + safetyLimit(loopGain * bandL);
        delayBufferR[static_cast<size_t>(writePos)] = inR + safetyLimit(loopGain * bandR);

        if (++writePos >= bufferSize)
            writePos = 0;

        // Pans move the repeats between the outputs; they are not in the loop
        left[i] = dry * inL + wet * (ll * bandL + rl * bandR);
        if (right != nullptr)
            right[i] = dry * inR + wet * (rr * bandR + lr * bandL);
    }
}

float DubDelay::readDelay(const std::vector<float>& buffer, float delay) const
{
    // Split into whole and fractional samples first, so precision does not
    // depend on the buffer length
    const int whole = static_cast<int>(delay);
    const float frac = delay - static_cast<float>(whole);

    // y1 is `whole` samples old, y2 is one sample older; interpolate from y1 towards y2
    int i1 = writePos - whole;
    if (i1 < 0) i1 += bufferSize;
    int i0 = i1 + 1; if (i0 >= bufferSize) i0 -= bufferSize;
    int i2 = i1 - 1; if (i2 < 0) i2 += bufferSize;
    int i3 = i2 - 1; if (i3 < 0) i3 += bufferSize;

    const float y0 = buffer[static_cast<size_t>(i0)];
    const float y1 = buffer[static_cast<size_t>(i1)];
    const float y2 = buffer[static_cast<size_t>(i2)];
    const float y3 = buffer[static_cast<size_t>(i3)];

    // Catmull-Rom
    const float a0 = -0.5f * y0 + 1.5f * y1 - 1.5f * y2 + 0.5f * y3;
    const float a1 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
    const float a2 = -0.5f * y0 + 0.5f * y2;

    return ((a0 * frac + a1) * frac + a2) * frac + y1;
}

void DubDelay::setDelayTime(float timeValue, bool tempoSync, double bpm)
{
    double delayMs = timeValue;

    if (tempoSync)
    {
        // 24 = quarter note, 96 = whole note
        if (bpm <= 0.0) bpm = 120.0;
        delayMs = (60000.0 / bpm) * (timeValue / 24.0);
    }

    const float maxSamples = static_cast<float>(bufferSize - 8);
    const float samples = static_cast<float>(delayMs * currentSampleRate / 1000.0);
    targetDelaySamples = std::clamp(samples, kMinDelaySamples, std::max(kMinDelaySamples, maxSamples));
}

void DubDelay::setFeedback(float fb)
{
    feedback = clamp01(fb / 100.0f);
}

void DubDelay::setDegradation(float degrad)
{
    degradation = clamp01(degrad / 100.0f);
}

void DubDelay::setFilterType(bool is24dB)
{
    if (is24dB && ! filter24dB)
    {
        filterL2.reset();
        filterR2.reset();
    }

    filter24dB = is24dB;
}

void DubDelay::setFilterFrequency(float freq)
{
    freq = std::clamp(freq, 300.0f, 3000.0f);
    if (freq != filterFreq)
    {
        filterFreq = freq;
        filtersDirty = true;
    }
}

void DubDelay::setFilterBandwidth(float octaves)
{
    octaves = std::clamp(octaves, 0.01f, 4.0f);
    if (octaves != filterOctaves)
    {
        filterOctaves = octaves;
        filtersDirty = true;
    }
}

void DubDelay::setGain(float gainDb)
{
    outputGain = std::pow(10.0f, std::clamp(gainDb, -12.0f, 12.0f) / 20.0f);
}

void DubDelay::setPanLR(float pan)
{
    panLR = clamp01(pan / 100.0f);
}

void DubDelay::setPanRL(float pan)
{
    panRL = clamp01(pan / 100.0f);
}

void DubDelay::setMix(float m)
{
    mix = clamp01(m / 100.0f);
}
