#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <cmath>
#include <algorithm>

// Visual offset to compensate for asymmetric transparent padding in filmstrip frames
struct VisualOffset { int dx = 0; int dy = 0; };

/**
 * FilmstripKnob - A slider that displays a filmstrip image
 *
 * Scans the filmstrip for transparent row separators to find exact frame
 * positions. Computes visual-center offset to compensate for padding.
 */
class FilmstripKnob : public juce::Slider
{
public:
    FilmstripKnob(const juce::Image& filmstrip, int expectedFrames)
        : filmstripImage(filmstrip)
    {
        setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);

        if (filmstripImage.isValid())
        {
            frameWidth = filmstripImage.getWidth();
            frameStarts = findFrameStarts(filmstripImage);

            // If scanning found no frames, fall back to even division
            if (frameStarts.empty() && expectedFrames > 0)
            {
                int stepHeight = filmstripImage.getHeight() / expectedFrames;
                for (int i = 0; i < expectedFrames; ++i)
                    frameStarts.push_back(i * stepHeight);
            }

            // Calculate canonical frame height from median of first N-1 frames
            // This avoids the "last frame has extra padding" trap
            if (frameStarts.size() >= 2)
            {
                // Compute heights for all but the last frame
                std::vector<int> heights;
                for (size_t i = 0; i < frameStarts.size() - 1; ++i)
                    heights.push_back(frameStarts[i + 1] - frameStarts[i]);

                // Use median as canonical height
                std::sort(heights.begin(), heights.end());
                frameHeight = heights[heights.size() / 2];
            }
            else if (!frameStarts.empty())
                frameHeight = filmstripImage.getHeight() - frameStarts[0];
            else
                frameHeight = filmstripImage.getHeight();

            // Compute visual-center offset from a representative middle frame
            if (!frameStarts.empty())
            {
                const int midIdx = static_cast<int>(frameStarts.size()) / 2;
                int testSrcY = juce::jlimit(0, filmstripImage.getHeight() - frameHeight, frameStarts[midIdx]);
                juce::Image testFrame = filmstripImage.getClippedImage(
                    {0, testSrcY, frameWidth, frameHeight});
                visualOffset = computeVisualOffset(testFrame);
                juce::Logger::writeToLog("FilmstripKnob visualOffset: dx=" + juce::String(visualOffset.dx)
                    + " dy=" + juce::String(visualOffset.dy)
                    + " frameHeight=" + juce::String(frameHeight)
                    + " frames=" + juce::String(static_cast<int>(frameStarts.size())));
            }
        }
    }

    void paint(juce::Graphics& g) override
    {
        if (!filmstripImage.isValid() || frameStarts.empty())
        {
            g.setColour(juce::Colours::grey);
            g.fillEllipse(getLocalBounds().toFloat());
            return;
        }

        // Calculate which frame to display
        double normalizedValue = (getValue() - getMinimum()) / (getMaximum() - getMinimum());
        int frameIndex = juce::jlimit(
            0,
            static_cast<int>(frameStarts.size()) - 1,
            static_cast<int>(std::round(normalizedValue * (frameStarts.size() - 1)))
        );

        int srcY = frameStarts[frameIndex];

        // Clamp srcY to ensure we don't read past image bounds (handles last frame)
        srcY = juce::jlimit(0, filmstripImage.getHeight() - frameHeight, srcY);

        // Extract the frame using canonical height
        juce::Image frame = filmstripImage.getClippedImage({0, srcY, frameWidth, frameHeight});

        auto area = getLocalBounds();

        // Draw at native frame size (canonical), centered in component
        int destW = frameWidth;
        int destH = frameHeight;

        // Use rounded division to avoid 1px truncation bias (left/up)
        int destX = area.getX() + (area.getWidth() - destW + 1) / 2 + visualOffset.dx;
        int destY = area.getY() + (area.getHeight() - destH + 1) / 2 + visualOffset.dy;

        // Explicit drawImage - no implicit centering math
        g.drawImage(frame,
                    destX, destY, destW, destH,
                    0, 0, frameWidth, frameHeight);
    }

private:
    juce::Image filmstripImage;
    std::vector<int> frameStarts;
    int frameWidth = 0;
    int frameHeight = 0;
    VisualOffset visualOffset;

    // Compute offset to center the visual content (alpha bounds) within the frame
    static VisualOffset computeVisualOffset(const juce::Image& frame)
    {
        int minX = frame.getWidth(), maxX = -1;
        int minY = frame.getHeight(), maxY = -1;

        for (int y = 0; y < frame.getHeight(); ++y)
        {
            for (int x = 0; x < frame.getWidth(); ++x)
            {
                if (frame.getPixelAt(x, y).getAlpha() > 0)
                {
                    minX = std::min(minX, x);
                    maxX = std::max(maxX, x);
                    minY = std::min(minY, y);
                    maxY = std::max(maxY, y);
                }
            }
        }

        if (maxX < 0) return {}; // nothing found

        const float alphaCx = 0.5f * static_cast<float>(minX + maxX);
        const float alphaCy = 0.5f * static_cast<float>(minY + maxY);

        const float desiredCx = 0.5f * static_cast<float>(frame.getWidth() - 1);
        const float desiredCy = 0.5f * static_cast<float>(frame.getHeight() - 1);

        VisualOffset o;
        o.dx = static_cast<int>(std::round(desiredCx - alphaCx)); // +dx moves image right
        o.dy = static_cast<int>(std::round(desiredCy - alphaCy)); // +dy moves image down
        return o;
    }

    // Scan filmstrip for frame boundaries using transparent row detection
    static std::vector<int> findFrameStarts(const juce::Image& img)
    {
        std::vector<int> starts;
        const int w = img.getWidth();
        const int h = img.getHeight();
        bool inContent = false;

        for (int y = 0; y < h; ++y)
        {
            bool rowEmpty = true;
            for (int x = 0; x < w; ++x)
            {
                if (img.getPixelAt(x, y).getAlpha() > 0)
                {
                    rowEmpty = false;
                    break;
                }
            }

            if (!rowEmpty && !inContent)
            {
                starts.push_back(y);
                inContent = true;
            }
            else if (rowEmpty && inContent)
            {
                inContent = false;
            }
        }

        return starts;
    }

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
