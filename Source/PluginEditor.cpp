#include "PluginEditor.h"
#include "BinaryData.h"

KingDubbyAudioProcessorEditor::KingDubbyAudioProcessorEditor(KingDubbyAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    loadImages();
    createKnobs();
    attachParameters();

    // Set fixed size - no resizing allowed
    setResizable(false, false);
    setSize(711, 348);
}

KingDubbyAudioProcessorEditor::~KingDubbyAudioProcessorEditor()
{
}

void KingDubbyAudioProcessorEditor::loadImages()
{
    // Load background images from binary data
    backgroundClassic = juce::ImageCache::getFromMemory(
        BinaryData::kingdubby_classicbg_png,
        BinaryData::kingdubby_classicbg_pngSize);

    backgroundDub = juce::ImageCache::getFromMemory(
        BinaryData::kingdubby_dubbg_png,
        BinaryData::kingdubby_dubbg_pngSize);

    // Load filmstrip images
    knobBigFilmstrip = juce::ImageCache::getFromMemory(
        BinaryData::kingdubby_bigdial_png,
        BinaryData::kingdubby_bigdial_pngSize);

    knobSmallFilmstrip = juce::ImageCache::getFromMemory(
        BinaryData::kingdubby_smalldial_png,
        BinaryData::kingdubby_smalldial_pngSize);

    filterSwitchFilmstrip = juce::ImageCache::getFromMemory(
        BinaryData::kingdubby_filterswitch_png,
        BinaryData::kingdubby_filterswitch_pngSize);
}

void KingDubbyAudioProcessorEditor::createKnobs()
{
    // DELAY section knobs (using big dial - 54x54, 57 frames)
    timeKnob = std::make_unique<FilmstripKnob>(knobBigFilmstrip, BIG_KNOB_FRAMES);
    timeKnob->setRange(1.0, 96.0, 1.0);
    addAndMakeVisible(*timeKnob);

    feedbackKnob = std::make_unique<FilmstripKnob>(knobBigFilmstrip, BIG_KNOB_FRAMES);
    feedbackKnob->setRange(0.0, 100.0, 0.1);
    addAndMakeVisible(*feedbackKnob);

    degradKnob = std::make_unique<FilmstripKnob>(knobBigFilmstrip, BIG_KNOB_FRAMES);
    degradKnob->setRange(0.0, 100.0, 0.1);
    addAndMakeVisible(*degradKnob);

    // FILTER section
    filterTypeToggle = std::make_unique<FilmstripToggle>(filterSwitchFilmstrip);
    addAndMakeVisible(*filterTypeToggle);

    filterFreqKnob = std::make_unique<FilmstripKnob>(knobBigFilmstrip, BIG_KNOB_FRAMES);
    filterFreqKnob->setRange(300.0, 3000.0, 1.0);
    filterFreqKnob->setSkewFactorFromMidPoint(1000.0);
    addAndMakeVisible(*filterFreqKnob);

    filterBWKnob = std::make_unique<FilmstripKnob>(knobBigFilmstrip, BIG_KNOB_FRAMES);
    filterBWKnob->setRange(0.0, 4.0, 0.01);
    addAndMakeVisible(*filterBWKnob);

    // OUTPUT section knobs (using small dial - 38x38, 58 frames)
    gainKnob = std::make_unique<FilmstripKnob>(knobSmallFilmstrip, SMALL_KNOB_FRAMES);
    gainKnob->setRange(-12.0, 12.0, 0.1);
    addAndMakeVisible(*gainKnob);

    panLRKnob = std::make_unique<FilmstripKnob>(knobSmallFilmstrip, SMALL_KNOB_FRAMES);
    panLRKnob->setRange(0.0, 100.0, 0.1);
    addAndMakeVisible(*panLRKnob);

    panRLKnob = std::make_unique<FilmstripKnob>(knobSmallFilmstrip, SMALL_KNOB_FRAMES);
    panRLKnob->setRange(0.0, 100.0, 0.1);
    addAndMakeVisible(*panRLKnob);

    mixKnob = std::make_unique<FilmstripKnob>(knobSmallFilmstrip, SMALL_KNOB_FRAMES);
    mixKnob->setRange(0.0, 100.0, 0.1);
    addAndMakeVisible(*mixKnob);
}

void KingDubbyAudioProcessorEditor::attachParameters()
{
    auto& apvts = audioProcessor.getAPVTS();

    timeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, KingDubbyAudioProcessor::PARAM_TIME, *timeKnob);

    feedbackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, KingDubbyAudioProcessor::PARAM_FEEDBACK, *feedbackKnob);

    degradAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, KingDubbyAudioProcessor::PARAM_DEGRAD, *degradKnob);

    filterTypeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts, KingDubbyAudioProcessor::PARAM_FILTER_TYPE, *filterTypeToggle);

    filterFreqAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, KingDubbyAudioProcessor::PARAM_FILTER_FREQ, *filterFreqKnob);

    filterBWAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, KingDubbyAudioProcessor::PARAM_FILTER_BW, *filterBWKnob);

    gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, KingDubbyAudioProcessor::PARAM_GAIN, *gainKnob);

    panLRAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, KingDubbyAudioProcessor::PARAM_PAN_LR, *panLRKnob);

    panRLAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, KingDubbyAudioProcessor::PARAM_PAN_RL, *panRLKnob);

    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, KingDubbyAudioProcessor::PARAM_MIX, *mixKnob);
}

void KingDubbyAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Draw background at fixed position
    auto& bg = useDubSkin ? backgroundDub : backgroundClassic;
    if (bg.isValid())
    {
        g.drawImageAt(bg, 0, 0);
    }
    else
    {
        g.fillAll(juce::Colours::darkgrey);
    }
}

void KingDubbyAudioProcessorEditor::resized()
{
    // Fixed layout for 711 x 348 window
    // Positions are absolute - no dynamic recalculation

    const int bigKnobSize = 54;
    const int smallKnobSize = 38;
    const int toggleWidth = 30;
    const int toggleHeight = 17;

    // Y positions - knobs sit in the colored track areas
    const int bigKnobY = 248;
    const int smallKnobY = 256;

    // DELAY section (red/orange area) - 3 knobs
    timeKnob->setBounds(53, bigKnobY, bigKnobSize, bigKnobSize);
    feedbackKnob->setBounds(121, bigKnobY, bigKnobSize, bigKnobSize);
    degradKnob->setBounds(191, bigKnobY, bigKnobSize, bigKnobSize);

    // FILTER section (yellow area) - toggle + 2 knobs
    filterTypeToggle->setBounds(280, 268, toggleWidth, toggleHeight);
    filterFreqKnob->setBounds(330, bigKnobY, bigKnobSize, bigKnobSize);
    filterBWKnob->setBounds(410, bigKnobY, bigKnobSize, bigKnobSize);

    // OUTPUT section (green area) - 4 small knobs
    gainKnob->setBounds(516, smallKnobY, smallKnobSize, smallKnobSize);
    panLRKnob->setBounds(566, smallKnobY, smallKnobSize, smallKnobSize);
    panRLKnob->setBounds(616, smallKnobY, smallKnobSize, smallKnobSize);
    mixKnob->setBounds(661, smallKnobY, smallKnobSize, smallKnobSize);
}
