#pragma once

#include <juce_graphics/juce_graphics.h>
#include <unordered_map>
#include <string>
#include <cmath>

struct LayoutPoint
{
    float nx = 0.0f;   // normalised [0..1]
    float ny = 0.0f;
    bool found = false;
};

class LayoutMap
{
public:
    explicit LayoutMap(int toleranceRGB = 2) : tol(toleranceRGB) {}

    void load(const juce::Image& layoutMapImage)
    {
        map = layoutMapImage;
        if (!map.isValid())
            return;
        w = map.getWidth();
        h = map.getHeight();
        cache.clear();
    }

    bool isLoaded() const { return map.isValid(); }

    // Register a control name -> target colour (RGB only; alpha ignored)
    void registerPoint(const juce::String& name, juce::Colour rgb)
    {
        targets.emplace(name.toStdString(), rgb);
    }

    // Scan everything once. Call after registerPoint().
    void scanAll()
    {
        cache.clear();
        for (const auto& kv : targets)
            cache[kv.first] = findBlobCentroidNormalised(kv.second);
    }

    LayoutPoint get(const juce::String& name) const
    {
        auto it = cache.find(name.toStdString());
        if (it == cache.end()) return {};
        return it->second;
    }

private:
    juce::Image map;
    int w = 0, h = 0;
    int tol = 2;

    std::unordered_map<std::string, juce::Colour> targets;
    std::unordered_map<std::string, LayoutPoint> cache;

    bool rgbClose(juce::Colour a, juce::Colour b) const
    {
        auto close = [this](uint8_t x, uint8_t y) {
            return std::abs(static_cast<int>(x) - static_cast<int>(y)) <= tol;
        };

        return close(a.getRed(), b.getRed())
            && close(a.getGreen(), b.getGreen())
            && close(a.getBlue(), b.getBlue());
    }

    LayoutPoint findBlobCentroidNormalised(juce::Colour targetRGB) const
    {
        double sumX = 0.0, sumY = 0.0, count = 0.0;

        for (int y = 0; y < h; ++y)
        {
            for (int x = 0; x < w; ++x)
            {
                auto c = map.getPixelAt(x, y);
                if (c.getAlpha() == 0) continue;

                if (rgbClose(c, targetRGB))
                {
                    sumX += static_cast<double>(x);
                    sumY += static_cast<double>(y);
                    count += 1.0;
                }
            }
        }

        LayoutPoint p;
        if (count <= 0.0)
            return p;

        const double cx = sumX / count;
        const double cy = sumY / count;

        p.nx = static_cast<float>(cx / static_cast<double>(w));
        p.ny = static_cast<float>(cy / static_cast<double>(h));
        p.found = true;
        return p;
    }
};
