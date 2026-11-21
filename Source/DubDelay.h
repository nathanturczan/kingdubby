#pragma once

#include <cmath>
#include <vector>

/**
 * DubDelay - the KingDubby delay engine
 *
 * Follows the signal flow of the original Lowcoders plugin:
 *
 *   read   = delayLine.read()
 *   band   = sqrt(d) * bandpass(read) + sqrt(1 - d) * read      (d = DEGRAD)
 *   out    = dryGain * in + wetGain * gain * pan(bandL, bandR)
 *   delayLine.write(in + feedback * band)
 *
 * The bandpass is the RBJ cookbook "constant 0 dB peak gain" type, so the
 * filter never adds gain to the loop at any bandwidth. Pans act on the wet
 * output only. Every crossfade (DEGRAD, pans, mix) is equal power.
 *
 * One deliberate departure: the feedback amount is scaled so the loop gain
 * peaks at MAX_LOOP_GAIN, which keeps repeats decaying at every setting.
 *
 * No JUCE dependency, so it can be tested offline (see Tests/).
 */
class DubDelay
{
public:
    DubDelay() = default;

    void prepare(double sampleRate);
    void reset();

    /** Processes in place. Pass right = nullptr for mono. */
    void process(float* left, float* right, int numSamples);

    // Parameters
    void setDelayTime(float timeValue, bool tempoSync, double bpm);  // 1-96 (24 = quarter note) for sync, or ms
    void setFeedback(float feedback);           // 0-100
    void setDegradation(float degrad);          // 0-100 (clean repeats -> filtered repeats)
    void setFilterType(bool is24dB);            // false = one bandpass, true = two in series
    void setFilterFrequency(float freq);        // 300-3000 Hz
    void setFilterBandwidth(float octaves);     // 0.01-4.0 octaves
    void setGain(float gainDb);                 // -12 to +12 dB (wet only)
    void setPanLR(float pan);                   // 0-100 (left repeats move to the right output)
    void setPanRL(float pan);                   // 0-100 (right repeats move to the left output)
    void setMix(float mix);                     // 0-100 (dry to wet)

    float getDelayTimeSamples() const { return targetDelaySamples; }

    static constexpr float MAX_DELAY_SECONDS = 10.0f;

    // Peak gain of the feedback loop with FEEDBACK at 100
    static constexpr float MAX_LOOP_GAIN = 0.98f;

private:
    // RBJ cookbook bandpass, constant 0 dB peak gain, bandwidth in octaves
    struct BandPass
    {
        void setCoefficients(double sampleRate, double freq, double octaves);
        void reset() { x1 = x2 = y1 = y2 = 0.0; }

        float process(float in)
        {
            const double x = in;
            const double y = b0 * (x - x2) - a1 * y1 - a2 * y2;
            x2 = x1; x1 = x;
            y2 = y1; y1 = y;
            return static_cast<float>(y);
        }

        double b0 = 0.0, a1 = 0.0, a2 = 0.0;
        double x1 = 0.0, x2 = 0.0, y1 = 0.0, y2 = 0.0;
    };

    // One-pole parameter smoother
    struct Smoothed
    {
        float current = 0.0f, target = 0.0f;
        void snap() { current = target; }
        float next(float coeff) { current += (target - current) * coeff; return current; }
    };

    float readDelay(const std::vector<float>& buffer, float delaySamples) const;
    void updateFilters();
    void updateGains();
    static float safetyLimit(float x);

    std::vector<float> delayBufferL, delayBufferR;
    int bufferSize = 0;
    int writePos = 0;

    double currentSampleRate = 44100.0;

    float delaySamples = 22050.0f;
    float targetDelaySamples = 22050.0f;
    float delaySmoothCoeff = 0.0005f;
    float gainSmoothCoeff = 0.002f;
    bool snapOnNextProcess = true;

    // Raw parameter values (0-1 unless noted)
    float feedback = 0.5f;
    float degradation = 0.1f;
    float panLR = 0.0f;
    float panRL = 0.0f;
    float mix = 0.5f;
    float outputGain = 1.0f;       // linear
    bool filter24dB = false;
    float filterFreq = 800.0f;     // Hz
    float filterOctaves = 1.0f;
    bool filtersDirty = true;

    // Smoothed gains
    Smoothed sFeedback, sFiltered, sClean, sDry, sWet;
    Smoothed sLeftToLeft, sLeftToRight, sRightToRight, sRightToLeft;

    BandPass filterL1, filterR1, filterL2, filterR2;
};
