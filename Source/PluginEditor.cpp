#include "PluginEditor.h"
#include "BinaryData.h"

KingDubbyAudioProcessorEditor::KingDubbyAudioProcessorEditor(KingDubbyAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    loadImages();
    setupLayoutMap();
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

    // Try to load layout map if it exists in binary data
    // Once kingdubby_layout_map_png is added, uncomment:
    // layoutMapImage = juce::ImageCache::getFromMemory(
    //     BinaryData::kingdubby_layout_map_png,
    //     BinaryData::kingdubby_layout_map_pngSize);
}

void KingDubbyAudioProcessorEditor::setupLayoutMap()
{
    // Color codes for each control (must match the layout map PNG)
    // TIME:        #FF0000 (red)
    // FEEDBACK:    #00FF00 (green)
    // DEGRAD:      #0000FF (blue)
    // FILTER_TYPE: #FFFF00 (yellow)
    // FREQ:        #FF00FF (magenta)
    // BANDW:       #00FFFF (cyan)
    // GAIN:        #FFA500 (orange)
    // PAN_LR:      #8000FF (purple)
    // PAN_RL:      #0080FF (light blue)
    // MIX:         #00FF80 (spring green)

    layoutMap.registerPoint("TIME",        juce::Colour(0xFF, 0x00, 0x00));
    layoutMap.registerPoint("FEEDBACK",    juce::Colour(0x00, 0xFF, 0x00));
    layoutMap.registerPoint("DEGRAD",      juce::Colour(0x00, 0x00, 0xFF));
    layoutMap.registerPoint("FILTER_TYPE", juce::Colour(0xFF, 0xFF, 0x00));
    layoutMap.registerPoint("FREQ",        juce::Colour(0xFF, 0x00, 0xFF));
    layoutMap.registerPoint("BANDW",       juce::Colour(0x00, 0xFF, 0xFF));
    layoutMap.registerPoint("GAIN",        juce::Colour(0xFF, 0xA5, 0x00));
    layoutMap.registerPoint("PAN_LR",      juce::Colour(0x80, 0x00, 0xFF));
    layoutMap.registerPoint("PAN_RL",      juce::Colour(0x00, 0x80, 0xFF));
    layoutMap.registerPoint("MIX",         juce::Colour(0x00, 0xFF, 0x80));

    // Load layout map and scan for control positions
    juce::Image layoutMapImage = juce::ImageCache::getFromMemory(
        BinaryData::kingdubby_layout_map_png,
        BinaryData::kingdubby_layout_map_pngSize);

    if (layoutMapImage.isValid())
    {
        // Sanity check: layout map must match background dimensions
        jassert(backgroundDub.getWidth() == layoutMapImage.getWidth());
        jassert(backgroundDub.getHeight() == layoutMapImage.getHeight());

        layoutMap.load(layoutMapImage);
        layoutMap.scanAll();

        // Log and verify all control dots
        auto logPoint = [&](const char* name)
        {
            auto p = layoutMap.get(name);
            DBG(juce::String(name) + ": found=" + (p.found ? "1" : "0")
                + " nx=" + juce::String(p.nx, 4)
                + " ny=" + juce::String(p.ny, 4));
            jassert(p.found);
        };

        DBG("=== Layout Map Points ===");
        DBG("Background: " + juce::String(backgroundDub.getWidth()) + "x" + juce::String(backgroundDub.getHeight()));
        DBG("LayoutMap: " + juce::String(layoutMapImage.getWidth()) + "x" + juce::String(layoutMapImage.getHeight()));
        logPoint("TIME");
        logPoint("FEEDBACK");
        logPoint("DEGRAD");
        logPoint("FILTER_TYPE");
        logPoint("FREQ");
        logPoint("BANDW");
        logPoint("GAIN");
        logPoint("PAN_LR");
        logPoint("PAN_RL");
        logPoint("MIX");
        DBG("=========================");

        useLayoutMap = true;
    }
}

void KingDubbyAudioProcessorEditor::createKnobs()
{
    // DELAY section - all small dials per original design
    timeKnob = std::make_unique<FilmstripKnob>(knobSmallFilmstrip, SMALL_KNOB_FRAMES);
    timeKnob->setRange(1.0, 96.0, 1.0);
    addAndMakeVisible(*timeKnob);

    feedbackKnob = std::make_unique<FilmstripKnob>(knobSmallFilmstrip, SMALL_KNOB_FRAMES);
    feedbackKnob->setRange(0.0, 100.0, 0.1);
    addAndMakeVisible(*feedbackKnob);

    degradKnob = std::make_unique<FilmstripKnob>(knobSmallFilmstrip, SMALL_KNOB_FRAMES);
    degradKnob->setRange(0.0, 100.0, 0.1);
    addAndMakeVisible(*degradKnob);

    // FILTER section - toggle + small dials
    filterTypeToggle = std::make_unique<FilmstripToggle>(filterSwitchFilmstrip);
    addAndMakeVisible(*filterTypeToggle);

    filterFreqKnob = std::make_unique<FilmstripKnob>(knobSmallFilmstrip, SMALL_KNOB_FRAMES);
    filterFreqKnob->setRange(300.0, 3000.0, 1.0);
    filterFreqKnob->setSkewFactorFromMidPoint(1000.0);
    addAndMakeVisible(*filterFreqKnob);

    filterBWKnob = std::make_unique<FilmstripKnob>(knobSmallFilmstrip, SMALL_KNOB_FRAMES);
    filterBWKnob->setRange(0.0, 4.0, 0.01);
    addAndMakeVisible(*filterBWKnob);

    // OUTPUT section - GAIN is the only BIG dial, rest are small
    gainKnob = std::make_unique<FilmstripKnob>(knobBigFilmstrip, BIG_KNOB_FRAMES);
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
    // Store bgRect for consistent placement in resized()
    bgRect = getLocalBounds();

    // Draw background scaled to fill the component
    auto& bg = useDubSkin ? backgroundDub : backgroundClassic;
    if (bg.isValid())
    {
        g.drawImage(bg, bgRect.toFloat());
    }
    else
    {
        g.fillAll(juce::Colours::darkgrey);
    }

}

void KingDubbyAudioProcessorEditor::resized()
{
    // Use bgRect for consistent placement (matches where background is drawn)
    // If bgRect not yet set (first call), use getLocalBounds()
    auto bounds = bgRect.isEmpty() ? getLocalBounds() : bgRect;

    // Design-space sizes (these are fixed regardless of layout method)
    const int bigKnobSize = 54;
    const int smallKnobSize = 38;
    const int toggleWidth = 30;
    const int toggleHeight = 17;

    if (useLayoutMap)
    {
        // Use layout map for precise positioning
        auto placeKnob = [this, &bounds](juce::Component& knob, const juce::String& key, int knobW, int knobH)
        {
            auto p = layoutMap.get(key);
            if (!p.found) return;

            const int cx = bounds.getX() + static_cast<int>(std::round(p.nx * static_cast<float>(bounds.getWidth())));
            const int cy = bounds.getY() + static_cast<int>(std::round(p.ny * static_cast<float>(bounds.getHeight())));
            knob.setBounds(cx - knobW / 2, cy - knobH / 2, knobW, knobH);
        };

        // All small dials except GAIN which is the only big dial
        placeKnob(*timeKnob, "TIME", smallKnobSize, smallKnobSize);
        placeKnob(*feedbackKnob, "FEEDBACK", smallKnobSize, smallKnobSize);
        placeKnob(*degradKnob, "DEGRAD", smallKnobSize, smallKnobSize);
        placeKnob(*filterTypeToggle, "FILTER_TYPE", toggleWidth, toggleHeight);
        placeKnob(*filterFreqKnob, "FREQ", smallKnobSize, smallKnobSize);
        placeKnob(*filterBWKnob, "BANDW", smallKnobSize, smallKnobSize);
        placeKnob(*gainKnob, "GAIN", bigKnobSize, bigKnobSize);  // Only big dial
        placeKnob(*panLRKnob, "PAN_LR", smallKnobSize, smallKnobSize);
        placeKnob(*panRLKnob, "PAN_RL", smallKnobSize, smallKnobSize);
        placeKnob(*mixKnob, "MIX", smallKnobSize, smallKnobSize);
    }
    else
    {
        // Fallback: manual coordinates until layout map is available
        constexpr float designWidth = 711.0f;
        constexpr float designHeight = 348.0f;

        const float scaleX = static_cast<float>(getWidth()) / designWidth;
        const float scaleY = static_cast<float>(getHeight()) / designHeight;

        auto scaledBounds = [scaleX, scaleY](int x, int y, int w, int h) {
            return juce::Rectangle<int>(
                static_cast<int>(x * scaleX),
                static_cast<int>(y * scaleY),
                static_cast<int>(w * scaleX),
                static_cast<int>(h * scaleY)
            );
        };

        const int knobY = 256;
        const int gainY = 248;  // GAIN is slightly higher (big dial)

        // DELAY section - all small dials
        timeKnob->setBounds(scaledBounds(53, knobY, smallKnobSize, smallKnobSize));
        feedbackKnob->setBounds(scaledBounds(121, knobY, smallKnobSize, smallKnobSize));
        degradKnob->setBounds(scaledBounds(191, knobY, smallKnobSize, smallKnobSize));

        // FILTER section - toggle + small dials
        filterTypeToggle->setBounds(scaledBounds(280, 262, toggleWidth, toggleHeight));
        filterFreqKnob->setBounds(scaledBounds(323, knobY, smallKnobSize, smallKnobSize));
        filterBWKnob->setBounds(scaledBounds(391, knobY, smallKnobSize, smallKnobSize));

        // OUTPUT section - GAIN is only big dial
        gainKnob->setBounds(scaledBounds(485, gainY, bigKnobSize, bigKnobSize));
        panLRKnob->setBounds(scaledBounds(540, knobY, smallKnobSize, smallKnobSize));
        panRLKnob->setBounds(scaledBounds(595, knobY, smallKnobSize, smallKnobSize));
        mixKnob->setBounds(scaledBounds(650, knobY, smallKnobSize, smallKnobSize));
    }
}

void KingDubbyAudioProcessorEditor::paintOverChildren(juce::Graphics& g)
{
    // Build stamp - only shown in debug builds
    // Uncomment for debugging:
    // static constexpr const char* kBuildStamp = "KingDubby " __DATE__ " " __TIME__;
    // g.setColour(juce::Colours::white.withAlpha(0.7f));
    // g.setFont(10.0f);
    // g.drawText(kBuildStamp, 6, 4, getWidth() - 12, 14, juce::Justification::left);

    // Debug overlays - only shown when kShowUiDebug is true
    if (kShowUiDebug && useLayoutMap)
    {
        // Helper to draw a point
        auto drawPoint = [&](int x, int y, juce::Colour c)
        {
            g.setColour(c);
            g.drawLine(static_cast<float>(x - 8), static_cast<float>(y),
                       static_cast<float>(x + 8), static_cast<float>(y), 2.0f);
            g.drawLine(static_cast<float>(x), static_cast<float>(y - 8),
                       static_cast<float>(x), static_cast<float>(y + 8), 2.0f);
        };

        // Helper to draw knob's actual center (white) and bounds rect
        auto drawKnobCenter = [&](juce::Component& knob)
        {
            auto b = knob.getBounds();
            drawPoint(b.getCentreX(), b.getCentreY(), juce::Colours::white);
            g.setColour(juce::Colours::white.withAlpha(0.3f));
            g.drawRect(b);
        };

        // Draw crosshairs at layout map positions AND actual knob centers
        auto drawCross = [&](const juce::String& key, juce::Colour color, juce::Component& knob)
        {
            auto p = layoutMap.get(key);
            if (!p.found) return;

            const int cx = bgRect.getX() + static_cast<int>(std::round(p.nx * static_cast<float>(bgRect.getWidth())));
            const int cy = bgRect.getY() + static_cast<int>(std::round(p.ny * static_cast<float>(bgRect.getHeight())));
            drawPoint(cx, cy, color);
            drawKnobCenter(knob);
        };

        drawCross("TIME", juce::Colours::red, *timeKnob);
        drawCross("FEEDBACK", juce::Colours::green, *feedbackKnob);
        drawCross("DEGRAD", juce::Colours::blue, *degradKnob);
        drawCross("FILTER_TYPE", juce::Colours::yellow, *filterTypeToggle);
        drawCross("FREQ", juce::Colours::magenta, *filterFreqKnob);
        drawCross("BANDW", juce::Colours::cyan, *filterBWKnob);
        drawCross("GAIN", juce::Colours::orange, *gainKnob);
        drawCross("PAN_LR", juce::Colours::purple, *panLRKnob);
        drawCross("PAN_RL", juce::Colours::lightblue, *panRLKnob);
        drawCross("MIX", juce::Colours::springgreen, *mixKnob);
    }

    // Footer disabled for now
    // TODO: Re-enable when DSP is refined
    /*
    const auto area = getLocalBounds();
    const juce::String prefixText = "revived in 2026 by ";
    const juce::String linkText = "Scale Navigator";

    juce::Font footerFont(11.0f);
    g.setFont(footerFont);

    const int prefixWidth = footerFont.getStringWidth(prefixText);
    const int linkWidth = footerFont.getStringWidth(linkText);
    const int totalWidth = prefixWidth + linkWidth;
    const int pad = 8;
    const int textHeight = 18;

    const int startX = area.getRight() - totalWidth - pad;
    const int textY = area.getBottom() - textHeight - pad;

    g.setColour(juce::Colours::white.withAlpha(0.4f));
    g.drawText(prefixText, startX, textY, prefixWidth, textHeight, juce::Justification::left, false);

    g.setColour(juce::Colour(0xFF, 0xD7, 0x00).withAlpha(0.85f));
    g.drawText(linkText, startX + prefixWidth, textY, linkWidth, textHeight, juce::Justification::left, false);

    footerBounds = juce::Rectangle<int>(startX + prefixWidth, textY, linkWidth, textHeight);
    */
    footerBounds = juce::Rectangle<int>();  // Empty bounds, no clickable area
}

void KingDubbyAudioProcessorEditor::mouseUp(const juce::MouseEvent& e)
{
    if (footerBounds.contains(e.getPosition()))
    {
        juce::URL("https://scalenavigator.com").launchInDefaultBrowser();
    }
}

void KingDubbyAudioProcessorEditor::mouseMove(const juce::MouseEvent& e)
{
    if (footerBounds.contains(e.getPosition()))
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
    else
        setMouseCursor(juce::MouseCursor::NormalCursor);
}
