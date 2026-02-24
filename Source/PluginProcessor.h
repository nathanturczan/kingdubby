#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "DubDelay.h"

class KingDubbyAudioProcessor : public juce::AudioProcessor
{
public:
    KingDubbyAudioProcessor();
    ~KingDubbyAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Parameter tree
    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

    // Parameter IDs
    static const juce::String PARAM_TIME;
    static const juce::String PARAM_FEEDBACK;
    static const juce::String PARAM_DEGRAD;
    static const juce::String PARAM_FILTER_TYPE;
    static const juce::String PARAM_FILTER_FREQ;
    static const juce::String PARAM_FILTER_BW;
    static const juce::String PARAM_GAIN;
    static const juce::String PARAM_PAN_LR;
    static const juce::String PARAM_PAN_RL;
    static const juce::String PARAM_MIX;

private:
    juce::AudioProcessorValueTreeState apvts;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    DubDelay dubDelay;

    // Atomic pointers to parameters
    std::atomic<float>* timeParam = nullptr;
    std::atomic<float>* feedbackParam = nullptr;
    std::atomic<float>* degradParam = nullptr;
    std::atomic<float>* filterTypeParam = nullptr;
    std::atomic<float>* filterFreqParam = nullptr;
    std::atomic<float>* filterBWParam = nullptr;
    std::atomic<float>* gainParam = nullptr;
    std::atomic<float>* panLRParam = nullptr;
    std::atomic<float>* panRLParam = nullptr;
    std::atomic<float>* mixParam = nullptr;

    // State tracking for buffer clearing
    bool wasPlaying = false;
    bool wasBypassed = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KingDubbyAudioProcessor)
};
