#pragma once

#include <juce_dsp/juce_dsp.h>
#include <juce_audio_basics/juce_audio_basics.h>

/**
 * DubDelay - PT2399-style dub tape delay engine
 *
 * Features:
 * - Stereo delay with ping-pong
 * - Degradation (lo-fi at longer delay times, mimicking PT2399)
 * - Bandpass filter in feedback loop
 * - Tempo sync
 */
class DubDelay
{
public:
    DubDelay();
    ~DubDelay() = default;

    void prepare(double sampleRate, int samplesPerBlock);
    void reset();
    void process(juce::AudioBuffer<float>& buffer);

    // Parameters
    void setDelayTime(float timeValue, bool tempoSync, double bpm);  // 1-96 for sync, or ms
    void setFeedback(float feedback);           // 0-100
    void setDegradation(float degrad);          // 0-100
    void setFilterType(bool is24dB);            // false=12dB, true=24dB
    void setFilterFrequency(float freq);        // 300-3000 Hz
    void setFilterBandwidth(float q);           // 0.0-4.0
    void setGain(float gainDb);                 // -12 to +12 dB
    void setPanLR(float pan);                   // 0-100 (left to right crossfeed)
    void setPanRL(float pan);                   // 0-100 (right to left crossfeed)
    void setMix(float mix);                     // 0-100 (dry to wet)

private:
    // Delay buffers
    static constexpr int MAX_DELAY_SAMPLES = 192000 * 4;  // 4 seconds at 192kHz

    // Feedback write-back ceiling (invariant - see domain.md)
    // Guarantees stability regardless of EQ/saturation behavior
    static constexpr float FB_WRITE_LIMIT = 0.95f;
    std::array<float, MAX_DELAY_SAMPLES> delayBufferL{};
    std::array<float, MAX_DELAY_SAMPLES> delayBufferR{};
    int writePos = 0;

    // Sample rate
    double currentSampleRate = 44100.0;

    // Current delay time in samples
    float delayTimeSamples = 22050.0f;  // 500ms default
    float targetDelayTimeSamples = 22050.0f;

    // Parameters
    float feedback = 0.5f;
    float degradation = 0.0f;
    bool filter24dB = false;
    float filterFreq = 1000.0f;
    float filterQ = 1.0f;
    float outputGain = 1.0f;
    float panLR = 0.0f;
    float panRL = 0.0f;
    float wetMix = 0.5f;

    // Filters (state variable filter for flexibility)
    juce::dsp::StateVariableTPTFilter<float> filterL1, filterR1;
    juce::dsp::StateVariableTPTFilter<float> filterL2, filterR2;  // For 24dB mode

    // Degradation lowpass (simulates PT2399 bandwidth reduction)
    juce::dsp::StateVariableTPTFilter<float> degradeLPL, degradeLPR;

    // Feedback-path LPF (darkens repeats, prevents harsh buildup)
    // See: GitHub issue #4, domain.md
    juce::dsp::StateVariableTPTFilter<float> feedbackLPL, feedbackLPR;
    static constexpr float FEEDBACK_LPF_FREQ = 8000.0f;  // Hz

    // Sample-and-hold for degradation (sample rate reduction)
    float holdL = 0.0f, holdR = 0.0f;
    int holdCounter = 0;
    int holdPeriod = 1;

    // Helper functions
    float readDelay(const std::array<float, MAX_DELAY_SAMPLES>& buffer, float delaySamples);
    float softClip(float x);
    float calculateNoteDivisionMs(float noteValue, double bpm);
};
