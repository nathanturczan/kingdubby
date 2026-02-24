#include "PluginProcessor.h"
#include "PluginEditor.h"

// Parameter IDs
const juce::String KingDubbyAudioProcessor::PARAM_TIME = "time";
const juce::String KingDubbyAudioProcessor::PARAM_FEEDBACK = "feedback";
const juce::String KingDubbyAudioProcessor::PARAM_DEGRAD = "degrad";
const juce::String KingDubbyAudioProcessor::PARAM_FILTER_TYPE = "filterType";
const juce::String KingDubbyAudioProcessor::PARAM_FILTER_FREQ = "filterFreq";
const juce::String KingDubbyAudioProcessor::PARAM_FILTER_BW = "filterBW";
const juce::String KingDubbyAudioProcessor::PARAM_GAIN = "gain";
const juce::String KingDubbyAudioProcessor::PARAM_PAN_LR = "panLR";
const juce::String KingDubbyAudioProcessor::PARAM_PAN_RL = "panRL";
const juce::String KingDubbyAudioProcessor::PARAM_MIX = "mix";

KingDubbyAudioProcessor::KingDubbyAudioProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    // Get atomic parameter pointers
    timeParam = apvts.getRawParameterValue(PARAM_TIME);
    feedbackParam = apvts.getRawParameterValue(PARAM_FEEDBACK);
    degradParam = apvts.getRawParameterValue(PARAM_DEGRAD);
    filterTypeParam = apvts.getRawParameterValue(PARAM_FILTER_TYPE);
    filterFreqParam = apvts.getRawParameterValue(PARAM_FILTER_FREQ);
    filterBWParam = apvts.getRawParameterValue(PARAM_FILTER_BW);
    gainParam = apvts.getRawParameterValue(PARAM_GAIN);
    panLRParam = apvts.getRawParameterValue(PARAM_PAN_LR);
    panRLParam = apvts.getRawParameterValue(PARAM_PAN_RL);
    mixParam = apvts.getRawParameterValue(PARAM_MIX);
}

KingDubbyAudioProcessor::~KingDubbyAudioProcessor()
{
}

juce::AudioProcessorValueTreeState::ParameterLayout KingDubbyAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // TIME: 1-96 (note divisions)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(PARAM_TIME, 1),
        "Time",
        juce::NormalisableRange<float>(1.0f, 96.0f, 1.0f),
        24.0f  // Default: quarter note
    ));

    // FEEDBACK: 0-100
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(PARAM_FEEDBACK, 1),
        "Feedback",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        50.0f
    ));

    // DEGRAD: 0-100
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(PARAM_DEGRAD, 1),
        "Degradation",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f
    ));

    // FILTER TYPE: 12/24 dB (bool)
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID(PARAM_FILTER_TYPE, 1),
        "Filter 24dB",
        false  // Default: 12dB
    ));

    // FILTER FREQUENCY: 300-3000 Hz
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(PARAM_FILTER_FREQ, 1),
        "Filter Frequency",
        juce::NormalisableRange<float>(300.0f, 3000.0f, 1.0f, 0.5f),  // Skewed
        1000.0f
    ));

    // FILTER BANDWIDTH: 0.0-4.0
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(PARAM_FILTER_BW, 1),
        "Filter Bandwidth",
        juce::NormalisableRange<float>(0.0f, 4.0f, 0.01f),
        2.0f
    ));

    // GAIN: -12 to +12 dB
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(PARAM_GAIN, 1),
        "Gain",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f),
        0.0f
    ));

    // PAN L-R: 0-100
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(PARAM_PAN_LR, 1),
        "Pan L-R",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f
    ));

    // PAN R-L: 0-100
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(PARAM_PAN_RL, 1),
        "Pan R-L",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f
    ));

    // MIX: 0-100
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(PARAM_MIX, 1),
        "Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        50.0f
    ));

    return { params.begin(), params.end() };
}

const juce::String KingDubbyAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool KingDubbyAudioProcessor::acceptsMidi() const
{
    return false;
}

bool KingDubbyAudioProcessor::producesMidi() const
{
    return false;
}

bool KingDubbyAudioProcessor::isMidiEffect() const
{
    return false;
}

double KingDubbyAudioProcessor::getTailLengthSeconds() const
{
    return 4.0;  // Max delay time
}

int KingDubbyAudioProcessor::getNumPrograms()
{
    return 1;
}

int KingDubbyAudioProcessor::getCurrentProgram()
{
    return 0;
}

void KingDubbyAudioProcessor::setCurrentProgram(int /*index*/)
{
}

const juce::String KingDubbyAudioProcessor::getProgramName(int /*index*/)
{
    return {};
}

void KingDubbyAudioProcessor::changeProgramName(int /*index*/, const juce::String& /*newName*/)
{
}

void KingDubbyAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    dubDelay.prepare(sampleRate, samplesPerBlock);
}

void KingDubbyAudioProcessor::releaseResources()
{
    dubDelay.reset();
}

bool KingDubbyAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // Support mono or stereo
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // Input must match output
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

void KingDubbyAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;

    // Get host transport state
    double bpm = 120.0;
    bool isPlaying = false;

    if (auto* playHead = getPlayHead())
    {
        if (auto posInfo = playHead->getPosition())
        {
            if (posInfo->getBpm().hasValue())
                bpm = *posInfo->getBpm();
            if (posInfo->getIsPlaying())
                isPlaying = true;
        }
    }

    // Reset on transport start (stopped -> playing)
    // Clears delay buffers AND all filter states to prevent:
    // - Old feedback bleeding through
    // - Ghost tones from filter state
    // See: GitHub issue #7, domain.md
    if (isPlaying && !wasPlaying)
    {
        dubDelay.reset();
    }
    wasPlaying = isPlaying;

    // Update delay parameters
    dubDelay.setDelayTime(timeParam->load(), true, bpm);
    dubDelay.setFeedback(feedbackParam->load());
    dubDelay.setDegradation(degradParam->load());
    dubDelay.setFilterType(filterTypeParam->load() > 0.5f);
    dubDelay.setFilterFrequency(filterFreqParam->load());
    dubDelay.setFilterBandwidth(filterBWParam->load());
    dubDelay.setGain(gainParam->load());
    dubDelay.setPanLR(panLRParam->load());
    dubDelay.setPanRL(panRLParam->load());
    dubDelay.setMix(mixParam->load());

    // Process audio
    dubDelay.process(buffer);
}

bool KingDubbyAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* KingDubbyAudioProcessor::createEditor()
{
    return new KingDubbyAudioProcessorEditor(*this);
}

void KingDubbyAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void KingDubbyAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr)
    {
        if (xmlState->hasTagName(apvts.state.getType()))
        {
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new KingDubbyAudioProcessor();
}
