#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

/**
 * FilmstripKnob - A slider that displays a filmstrip image
 *
 * The filmstrip image should contain multiple frames stacked vertically,
 * each frame showing the knob at a different rotation angle.
 */
class FilmstripKnob : public juce::Slider
{
public:
    FilmstripKnob(const juce::Image& filmstrip, int numFrames)
        : filmstripImage(filmstrip),
          frameCount(numFrames)
    {
        setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);

        if (filmstripImage.isValid() && frameCount > 0)
        {
            frameWidth = filmstripImage.getWidth();
            frameHeight = filmstripImage.getHeight() / frameCount;
        }
    }

    void paint(juce::Graphics& g) override
    {
        if (!filmstripImage.isValid() || frameCount <= 0)
        {
            // Fallback: draw a simple circle
            g.setColour(juce::Colours::grey);
            g.fillEllipse(getLocalBounds().toFloat());
            return;
        }

        // Calculate which frame to display based on current value
        double normalizedValue = (getValue() - getMinimum()) / (getMaximum() - getMinimum());
        int frameIndex = static_cast<int>(normalizedValue * (frameCount - 1));
        frameIndex = juce::jlimit(0, frameCount - 1, frameIndex);

        // Source rectangle in filmstrip
        int srcY = frameIndex * frameHeight;

        // Draw the frame scaled to fit the component
        g.drawImage(filmstripImage,
                    0, 0, getWidth(), getHeight(),              // Destination
                    0, srcY, frameWidth, frameHeight);          // Source
    }

private:
    juce::Image filmstripImage;
    int frameCount = 1;
    int frameWidth = 0;
    int frameHeight = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FilmstripKnob)
};

/**
 * FilmstripToggle - A toggle button using a filmstrip image
 * Expects 2 frames: off (top) and on (bottom)
 */
class FilmstripToggle : public juce::ToggleButton
{
public:
    FilmstripToggle(const juce::Image& filmstrip)
        : filmstripImage(filmstrip)
    {
        if (filmstripImage.isValid())
        {
            frameWidth = filmstripImage.getWidth();
            frameHeight = filmstripImage.getHeight() / 2;  // 2 states
        }
    }

    void paintButton(juce::Graphics& g, bool /*shouldDrawButtonAsHighlighted*/, bool /*shouldDrawButtonAsDown*/) override
    {
        if (!filmstripImage.isValid())
        {
            // Fallback
            g.setColour(getToggleState() ? juce::Colours::green : juce::Colours::grey);
            g.fillRect(getLocalBounds());
            return;
        }

        int srcY = getToggleState() ? frameHeight : 0;

        g.drawImage(filmstripImage,
                    0, 0, getWidth(), getHeight(),
                    0, srcY, frameWidth, frameHeight);
    }

private:
    juce::Image filmstripImage;
    int frameWidth = 0;
    int frameHeight = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FilmstripToggle)
};
