#include "DubDelay.h"
#include <cmath>

DubDelay::DubDelay()
{
    // Initialize filters as bandpass
    filterL1.setType(juce::dsp::StateVariableTPTFilterType::bandpass);
    filterR1.setType(juce::dsp::StateVariableTPTFilterType::bandpass);
    filterL2.setType(juce::dsp::StateVariableTPTFilterType::bandpass);
    filterR2.setType(juce::dsp::StateVariableTPTFilterType::bandpass);

    // Degradation lowpass
    degradeLPL.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    degradeLPR.setType(juce::dsp::StateVariableTPTFilterType::lowpass);

    // Feedback-path LPF (darkens repeats - issue #4)
    feedbackLPL.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    feedbackLPR.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    feedbackLPL.setCutoffFrequency(FEEDBACK_LPF_FREQ);
    feedbackLPR.setCutoffFrequency(FEEDBACK_LPF_FREQ);
}

void DubDelay::prepare(double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;

    // Prepare filters
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = 512;
    spec.numChannels = 1;

    filterL1.prepare(spec);
    filterR1.prepare(spec);
    filterL2.prepare(spec);
    filterR2.prepare(spec);
    degradeLPL.prepare(spec);
    degradeLPR.prepare(spec);
    feedbackLPL.prepare(spec);
    feedbackLPR.prepare(spec);

    // Log DSP config (once per init - see domain.md)
    DBG("DubDelay::prepare() - sampleRate=" + juce::String(sampleRate)
        + " FB_WRITE_LIMIT=" + juce::String(FB_WRITE_LIMIT)
        + " FEEDBACK_LPF_FREQ=" + juce::String(FEEDBACK_LPF_FREQ));

    reset();
}

void DubDelay::reset()
{
    // Clear delay buffers
    std::fill(delayBufferL.begin(), delayBufferL.end(), 0.0f);
    std::fill(delayBufferR.begin(), delayBufferR.end(), 0.0f);
    writePos = 0;

    // Reset all filter states (prevents ghost tones)
    filterL1.reset();
    filterR1.reset();
    filterL2.reset();
    filterR2.reset();
    degradeLPL.reset();
    degradeLPR.reset();
    feedbackLPL.reset();
    feedbackLPR.reset();

    // Reset degradation state
    holdL = holdR = 0.0f;
    holdCounter = 0;

    // Sync delay time (avoid smoothing zipper on restart)
    delayTimeSamples = targetDelayTimeSamples;

    DBG("DubDelay::reset() - buffers and filters cleared");
}

void DubDelay::process(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    if (numChannels < 1) return;

    float* leftChannel = buffer.getWritePointer(0);
    float* rightChannel = numChannels > 1 ? buffer.getWritePointer(1) : nullptr;

    // Smoothly interpolate delay time
    const float smoothingCoeff = 0.9995f;

    for (int i = 0; i < numSamples; ++i)
    {
        // Smooth delay time changes
        delayTimeSamples = delayTimeSamples * smoothingCoeff + targetDelayTimeSamples * (1.0f - smoothingCoeff);

        // Read from delay lines with interpolation
        float delayedL = readDelay(delayBufferL, delayTimeSamples);
        float delayedR = rightChannel ? readDelay(delayBufferR, delayTimeSamples) : delayedL;

        // Apply degradation (sample rate reduction + lowpass)
        if (degradation > 0.001f)
        {
            // Sample-and-hold for "digital" degradation
            holdCounter++;
            if (holdCounter >= holdPeriod)
            {
                holdL = delayedL;
                holdR = delayedR;
                holdCounter = 0;
            }

            // Mix between clean and degraded based on degradation amount
            float degradeMix = degradation;
            delayedL = delayedL * (1.0f - degradeMix) + holdL * degradeMix;
            delayedR = delayedR * (1.0f - degradeMix) + holdR * degradeMix;

            // Lowpass filter for bandwidth reduction
            delayedL = degradeLPL.processSample(0, delayedL);
            delayedR = degradeLPR.processSample(0, delayedR);
        }

        // Apply bandpass filter in feedback path
        float filteredL = filterL1.processSample(0, delayedL);
        float filteredR = filterR1.processSample(0, delayedR);

        if (filter24dB)
        {
            filteredL = filterL2.processSample(0, filteredL);
            filteredR = filterR2.processSample(0, filteredR);
        }

        // Ping-pong crossfeed
        float crossL = filteredR * panRL;
        float crossR = filteredL * panLR;

        // GAIN
        float feedbackL = (filteredL + crossL) * feedback;
        float feedbackR = (filteredR + crossR) * feedback;

        // SOFTCLIP (musical saturation - generates HF harmonics)
        feedbackL = softClip(feedbackL);
        feedbackR = softClip(feedbackR);

        // LPF (after softclip! removes edge harmonics before re-injection)
        // See: GitHub issue #4, domain.md
        feedbackL = feedbackLPL.processSample(0, feedbackL);
        feedbackR = feedbackLPR.processSample(0, feedbackR);

        // CEILING (invariant - see domain.md, GitHub #5)
        // Clamp feedback only, not dry input - preserves transients
        feedbackL = juce::jlimit(-FB_WRITE_LIMIT, FB_WRITE_LIMIT, feedbackL);
        feedbackR = juce::jlimit(-FB_WRITE_LIMIT, FB_WRITE_LIMIT, feedbackR);

        // Get dry input
        float dryL = leftChannel[i];
        float dryR = rightChannel ? rightChannel[i] : dryL;

        // Write to delay buffer (input + feedback)
        int wp = writePos % MAX_DELAY_SAMPLES;
        delayBufferL[wp] = dryL + feedbackL;
        delayBufferR[wp] = dryR + feedbackR;

        // Mix dry/wet and apply output gain
        float wetL = filteredL * outputGain;
        float wetR = filteredR * outputGain;

        leftChannel[i] = dryL * (1.0f - wetMix) + wetL * wetMix;
        if (rightChannel)
            rightChannel[i] = dryR * (1.0f - wetMix) + wetR * wetMix;

        writePos++;
        if (writePos >= MAX_DELAY_SAMPLES)
            writePos = 0;
    }
}

float DubDelay::readDelay(const std::array<float, MAX_DELAY_SAMPLES>& buffer, float delaySamples)
{
    // Cubic interpolation for smooth delay time changes
    float readPos = static_cast<float>(writePos) - delaySamples;
    while (readPos < 0) readPos += MAX_DELAY_SAMPLES;

    int pos0 = static_cast<int>(readPos) % MAX_DELAY_SAMPLES;
    int pos1 = (pos0 + 1) % MAX_DELAY_SAMPLES;
    int posM1 = (pos0 - 1 + MAX_DELAY_SAMPLES) % MAX_DELAY_SAMPLES;
    int pos2 = (pos0 + 2) % MAX_DELAY_SAMPLES;

    float frac = readPos - std::floor(readPos);

    // Cubic interpolation (Catmull-Rom)
    float y0 = buffer[posM1];
    float y1 = buffer[pos0];
    float y2 = buffer[pos1];
    float y3 = buffer[pos2];

    float a0 = -0.5f * y0 + 1.5f * y1 - 1.5f * y2 + 0.5f * y3;
    float a1 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
    float a2 = -0.5f * y0 + 0.5f * y2;
    float a3 = y1;

    return a0 * frac * frac * frac + a1 * frac * frac + a2 * frac + a3;
}

float DubDelay::softClip(float x)
{
    // Soft saturation using tanh
    return std::tanh(x);
}

float DubDelay::calculateNoteDivisionMs(float noteValue, double bpm)
{
    // noteValue 1-96 maps to note divisions
    // 96 = whole note, 48 = half, 24 = quarter, 12 = eighth, 6 = sixteenth, etc.
    // At 120 BPM: quarter note = 500ms

    if (bpm <= 0) bpm = 120.0;

    double quarterNoteMs = 60000.0 / bpm;

    // Convert noteValue (1-96) to fraction of whole note
    // 96 = 1 whole note = 4 quarter notes
    // So noteValue / 24 = number of quarter notes
    double quarterNotes = noteValue / 24.0;

    return static_cast<float>(quarterNoteMs * quarterNotes);
}

void DubDelay::setDelayTime(float timeValue, bool tempoSync, double bpm)
{
    float delayMs;

    if (tempoSync)
    {
        delayMs = calculateNoteDivisionMs(timeValue, bpm);
    }
    else
    {
        // Direct milliseconds (not used in original UI, but supported)
        delayMs = timeValue;
    }

    // Clamp to reasonable range
    delayMs = std::clamp(delayMs, 1.0f, 4000.0f);

    targetDelayTimeSamples = static_cast<float>(delayMs * currentSampleRate / 1000.0);
    targetDelayTimeSamples = std::clamp(targetDelayTimeSamples, 1.0f, static_cast<float>(MAX_DELAY_SAMPLES - 1));

    // Update degradation characteristics based on delay time
    // PT2399 degrades at longer delay times
    // At 30ms: full bandwidth (~15kHz)
    // At 500ms+: reduced bandwidth (~3kHz)
    float degradeCutoff = juce::jmap(delayMs, 30.0f, 500.0f, 15000.0f, 3000.0f);
    degradeCutoff = std::clamp(degradeCutoff, 2000.0f, 15000.0f);
    degradeLPL.setCutoffFrequency(degradeCutoff);
    degradeLPR.setCutoffFrequency(degradeCutoff);

    // Sample rate reduction period increases with delay time
    holdPeriod = static_cast<int>(juce::jmap(delayMs, 30.0f, 500.0f, 1.0f, 4.0f));
    holdPeriod = std::max(1, holdPeriod);
}

void DubDelay::setFeedback(float fb)
{
    // 0-100 -> 0.0-0.95 (capped below unity to prevent runaway)
    // See: GitHub issue #3
    feedback = fb / 100.0f * 0.95f;
}

void DubDelay::setDegradation(float degrad)
{
    // 0-100 -> 0.0-1.0
    degradation = degrad / 100.0f;
}

void DubDelay::setFilterType(bool is24dB)
{
    filter24dB = is24dB;
}

void DubDelay::setFilterFrequency(float freq)
{
    filterFreq = std::clamp(freq, 300.0f, 3000.0f);
    filterL1.setCutoffFrequency(filterFreq);
    filterR1.setCutoffFrequency(filterFreq);
    filterL2.setCutoffFrequency(filterFreq);
    filterR2.setCutoffFrequency(filterFreq);
}

void DubDelay::setFilterBandwidth(float q)
{
    // Q of 0.0-4.0 -> resonance 0.5-5.0
    filterQ = juce::jmap(q, 0.0f, 4.0f, 0.5f, 5.0f);
    filterL1.setResonance(filterQ);
    filterR1.setResonance(filterQ);
    filterL2.setResonance(filterQ);
    filterR2.setResonance(filterQ);
}

void DubDelay::setGain(float gainDb)
{
    // -12 to +12 dB
    outputGain = juce::Decibels::decibelsToGain(gainDb);
}

void DubDelay::setPanLR(float pan)
{
    // 0-100 -> 0.0-1.0
    panLR = pan / 100.0f;
}

void DubDelay::setPanRL(float pan)
{
    // 0-100 -> 0.0-1.0
    panRL = pan / 100.0f;
}

void DubDelay::setMix(float mix)
{
    // 0-100 -> 0.0-1.0
    wetMix = mix / 100.0f;
}
