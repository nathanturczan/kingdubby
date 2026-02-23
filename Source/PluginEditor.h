#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "FilmstripKnob.h"

class KingDubbyAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit KingDubbyAudioProcessorEditor(KingDubbyAudioProcessor&);
    ~KingDubbyAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    KingDubbyAudioProcessor& audioProcessor;

    // Background images
    juce::Image backgroundClassic;
    juce::Image backgroundDub;
    bool useDubSkin = true;  // Start with dub skin (the colorful one)

    // Filmstrip images
    juce::Image knobBigFilmstrip;
    juce::Image knobSmallFilmstrip;
    juce::Image filterSwitchFilmstrip;

    // Number of frames in filmstrips
    static constexpr int BIG_KNOB_FRAMES = 57;
    static constexpr int SMALL_KNOB_FRAMES = 58;

    // Knobs - DELAY section
    std::unique_ptr<FilmstripKnob> timeKnob;
    std::unique_ptr<FilmstripKnob> feedbackKnob;
    std::unique_ptr<FilmstripKnob> degradKnob;

    // FILTER section
    std::unique_ptr<FilmstripToggle> filterTypeToggle;
    std::unique_ptr<FilmstripKnob> filterFreqKnob;
    std::unique_ptr<FilmstripKnob> filterBWKnob;

    // OUTPUT section
    std::unique_ptr<FilmstripKnob> gainKnob;
    std::unique_ptr<FilmstripKnob> panLRKnob;
    std::unique_ptr<FilmstripKnob> panRLKnob;
    std::unique_ptr<FilmstripKnob> mixKnob;

    // Parameter attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> timeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> feedbackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> degradAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> filterTypeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filterFreqAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filterBWAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> panLRAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> panRLAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;

    void loadImages();
    void createKnobs();
    void attachParameters();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KingDubbyAudioProcessorEditor)
};
