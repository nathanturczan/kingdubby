// Offline regression test for DubDelay. No JUCE needed.
// Run with scripts/run-dsp-tests.sh

#include "../Source/DubDelay.h"

#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <memory>
#include <random>
#include <vector>

namespace
{
    int failures = 0;

    void check(bool ok, const char* fmt, ...) __attribute__((format(printf, 2, 3)));
    void check(bool ok, const char* fmt, ...)
    {
        if (ok) return;
        ++failures;
        va_list args;
        va_start(args, fmt);
        std::printf("  FAIL: ");
        std::vprintf(fmt, args);
        std::printf("\n");
        va_end(args);
    }

    double toDb(double x) { return 20.0 * std::log10(x + 1e-30); }

    struct Settings
    {
        float time = 24, feedback = 50, degrad = 10, freq = 800, bw = 1, gain = 0;
        float panLR = 0, panRL = 0, mix = 100;
        bool twice = false;
        double bpm = 120;
    };

    std::unique_ptr<DubDelay> make(const Settings& s, double sr)
    {
        auto d = std::make_unique<DubDelay>();
        d->prepare(sr);
        d->setDelayTime(s.time, true, s.bpm);
        d->setFeedback(s.feedback);
        d->setDegradation(s.degrad);
        d->setFilterType(s.twice);
        d->setFilterFrequency(s.freq);
        d->setFilterBandwidth(s.bw);
        d->setGain(s.gain);
        d->setPanLR(s.panLR);
        d->setPanRL(s.panRL);
        d->setMix(s.mix);
        return d;
    }

    struct Levels { std::vector<double> rmsPerSecond; double peak = 0; bool finite = true; };

    enum class Input { noiseBurst, tinyClick, sustainedNoise, sustainedSine };

    // Runs `seconds` of audio in 512-sample blocks and reports the wet level.
    Levels run(DubDelay& d, double sr, int seconds, Input input, double sineHz = 0)
    {
        Levels out;
        std::mt19937 rng(1);
        std::uniform_real_distribution<float> noise(-1.0f, 1.0f);
        const int block = 512;
        std::vector<float> l(block), r(block);
        long n = 0;

        for (int s = 0; s < seconds; ++s)
        {
            double acc = 0;
            long count = 0;
            while (count < static_cast<long>(sr))
            {
                for (int i = 0; i < block; ++i, ++n)
                {
                    float x = 0;
                    switch (input)
                    {
                        case Input::noiseBurst:     x = n < static_cast<long>(0.05 * sr) ? 0.5f * noise(rng) : 0; break;
                        case Input::tinyClick:      x = n == 0 ? 1e-6f : 0; break;
                        case Input::sustainedNoise: x = noise(rng); break;
                        case Input::sustainedSine:  x = static_cast<float>(std::sin(6.283185307179586 * sineHz * n / sr)); break;
                    }
                    l[i] = r[i] = x;
                }

                d.process(l.data(), r.data(), block);

                for (int i = 0; i < block; ++i)
                {
                    for (float v : { l[i], r[i] })
                    {
                        if (! std::isfinite(v)) out.finite = false;
                        out.peak = std::max(out.peak, static_cast<double>(std::abs(v)));
                    }
                    acc += static_cast<double>(l[i]) * l[i];
                }
                count += block;
            }
            out.rmsPerSecond.push_back(std::sqrt(acc / static_cast<double>(count)));
        }
        return out;
    }

    // The bug this test exists for: repeats must decay at every setting.
    void testNoRunaway()
    {
        std::printf("no runaway: repeats decay at every corner of the controls\n");
        int cases = 0;

        for (double sr : { 44100.0, 48000.0, 96000.0 })
        for (float fb : { 50.0f, 100.0f })
        for (float degrad : { 0.0f, 10.0f, 50.0f, 100.0f })
        for (float freq : { 300.0f, 800.0f, 3000.0f })
        for (float bw : { 0.0f, 1.0f, 4.0f })
        for (bool twice : { false, true })
        for (float time : { 1.0f, 24.0f })
        {
            Settings s;
            s.feedback = fb; s.degrad = degrad; s.freq = freq; s.bw = bw; s.twice = twice; s.time = time;
            auto d = make(s, sr);
            const auto lv = run(*d, sr, 12, Input::noiseBurst);
            ++cases;

            // A very narrow filter takes a few seconds to ring up, so judge the
            // trend over the second half. Short delays overlap the burst with its
            // own repeats, so the peak may pass 1 while input is present; it must
            // stay far from the old behaviour (+19 dBFS).
            const double first = lv.rmsPerSecond.front(), mid = lv.rmsPerSecond[6], last = lv.rmsPerSecond.back();

            check(lv.finite && lv.peak < 2.0 && last < first && (last <= mid || last < 1e-12),
                  "sr=%.0f fb=%.0f degrad=%.0f freq=%.0f bw=%.2f twice=%d time=%.0f: first %.1f dB, mid %.1f dB, last %.1f dB, peak %.2f",
                  sr, fb, degrad, freq, bw, twice, time, toDb(first), toDb(mid), toDb(last), lv.peak);
        }
        std::printf("  %d settings checked\n", cases);
    }

    void testSilenceStaysSilent()
    {
        std::printf("near-silence does not grow into a tone\n");
        for (bool twice : { false, true })
        for (float degrad : { 0.0f, 50.0f, 100.0f })
        {
            Settings s;
            s.feedback = 100; s.degrad = degrad; s.freq = 3000; s.bw = 0.01f; s.twice = twice;
            auto d = make(s, 48000);
            const auto lv = run(*d, 48000, 20, Input::tinyClick);
            check(lv.finite && lv.peak <= 2e-6, "degrad=%.0f twice=%d: peak %.3g from a 1e-6 click", degrad, twice, lv.peak);
        }
    }

    void testBoundedUnderSustainedInput()
    {
        std::printf("output stays bounded with full-scale input held at feedback 100\n");
        for (float degrad : { 0.0f, 50.0f, 100.0f })
        {
            Settings s;
            s.feedback = 100; s.degrad = degrad; s.gain = 0;
            // 2 Hz spacing of the comb at 1/4 note, 120 bpm: 800 Hz sits on a tooth
            for (Input in : { Input::sustainedNoise, Input::sustainedSine })
            {
                auto fresh = make(s, 48000);
                const auto lv = run(*fresh, 48000, 30, in, 800.0);
                // The delay line is limited to input + 2 and the DEGRAD blend peaks at
                // sqrt(2), so about 4.25 plus filter overshoot
                check(lv.finite && lv.peak < 5.0, "degrad=%.0f input=%d: peak %.2f", degrad, static_cast<int>(in), lv.peak);
            }
        }
    }

    void testImpulseTiming()
    {
        std::printf("clean repeats land on the beat at the right level\n");
        const double sr = 48000;
        Settings s;
        s.feedback = 100; s.degrad = 0;            // clean path only
        auto d = make(s, sr);
        const int delay = 24000;                    // 1/4 note at 120 bpm
        std::vector<float> l(static_cast<size_t>(delay * 2 + 16), 0.0f), r(l.size(), 0.0f);
        l[0] = r[0] = 1.0f;
        d->process(l.data(), r.data(), static_cast<int>(l.size()));

        check(std::abs(l[static_cast<size_t>(delay)] - 1.0f) < 1e-4f, "first repeat is %.5f at the delay time, expected 1", l[static_cast<size_t>(delay)]);
        check(std::abs(l[static_cast<size_t>(delay * 2)] - DubDelay::MAX_LOOP_GAIN) < 1e-4f,
              "second repeat is %.5f, expected %.2f", l[static_cast<size_t>(delay * 2)], DubDelay::MAX_LOOP_GAIN);

        double stray = 0;
        for (size_t i = 0; i < l.size(); ++i)
            if (i != static_cast<size_t>(delay) && i != static_cast<size_t>(delay * 2))
                stray = std::max(stray, static_cast<double>(std::abs(l[i])));
        check(stray < 1e-4, "energy between the repeats: %.5f", stray);
    }

    void testFeedbackZeroGivesOneRepeat()
    {
        std::printf("feedback 0 gives exactly one repeat\n");
        Settings s;
        s.feedback = 0; s.degrad = 0;
        auto d = make(s, 48000);
        const auto lv = run(*d, 48000, 3, Input::noiseBurst);
        check(lv.rmsPerSecond[0] > 1e-3 && lv.rmsPerSecond[1] < 1e-9 && lv.rmsPerSecond[2] < 1e-9,
              "levels %.1f / %.1f / %.1f dB", toDb(lv.rmsPerSecond[0]), toDb(lv.rmsPerSecond[1]), toDb(lv.rmsPerSecond[2]));
    }

    void testFilterPeakIsUnity()
    {
        std::printf("bandpass peaks at 0 dB at every bandwidth\n");
        const double sr = 48000;
        for (float bw : { 0.0f, 0.5f, 1.0f, 2.0f, 4.0f })
        for (bool twice : { false, true })
        for (float freq : { 300.0f, 800.0f, 3000.0f })
        {
            Settings s;
            s.feedback = 0; s.degrad = 100; s.bw = bw; s.twice = twice; s.freq = freq;
            auto d = make(s, sr);
            const auto lv = run(*d, sr, 6, Input::sustainedSine, freq);
            const double amp = lv.rmsPerSecond.back() * std::sqrt(2.0);
            const double tol = bw < 0.1f ? 0.05 : 0.01;    // the narrowest setting is still ringing up
            check(std::abs(amp - 1.0) < tol, "bw=%.2f twice=%d freq=%.0f: settled amplitude %.4f", bw, twice, freq, amp);
        }
    }

    void testMixAndReset()
    {
        std::printf("mix 0 passes the input untouched; reset empties the line without muting dry\n");
        const double sr = 48000;
        Settings s;
        s.mix = 0; s.feedback = 100;
        auto d = make(s, sr);

        std::mt19937 rng(7);
        std::uniform_real_distribution<float> noise(-1.0f, 1.0f);
        std::vector<float> in(48000), l, r;
        for (auto& v : in) v = noise(rng);
        l = r = in;
        d->process(l.data(), r.data(), static_cast<int>(l.size()));
        double diff = 0;
        for (size_t i = 0; i < in.size(); ++i) diff = std::max(diff, static_cast<double>(std::abs(l[i] - in[i])));
        check(diff < 1e-6, "mix 0 changed the signal by %.3g", diff);

        // The line is now full of noise. Reset, go fully wet, feed silence: must be silent.
        d->reset();
        d->setMix(100);
        std::vector<float> zl(96000, 0.0f), zr(96000, 0.0f);
        d->process(zl.data(), zr.data(), static_cast<int>(zl.size()));
        double peak = 0;
        for (float v : zl) peak = std::max(peak, static_cast<double>(std::abs(v)));
        check(peak == 0.0, "tail survived reset: peak %.3g", peak);

        // Reset followed by audio in the same block: dry comes straight through at mix 50
        d->reset();
        d->setMix(50);
        l = r = in;
        d->process(l.data(), r.data(), 512);
        diff = 0;
        for (size_t i = 0; i < 512; ++i) diff = std::max(diff, static_cast<double>(std::abs(l[i] - in[i] * std::sqrt(0.5f))));
        check(diff < 1e-6, "dry signal altered in the block after reset: %.3g", diff);
    }

    void testPans()
    {
        std::printf("pans move repeats between the outputs\n");
        const double sr = 48000;
        const int n = 48000;

        auto measure = [&](float panLR, float panRL, double& outL, double& outR)
        {
            Settings s;
            s.feedback = 0; s.degrad = 0; s.panLR = panLR; s.panRL = panRL;
            auto d = make(s, sr);
            std::vector<float> l(static_cast<size_t>(n), 0.0f), r(static_cast<size_t>(n), 0.0f);
            l[0] = 1.0f;                                 // left input only
            d->process(l.data(), r.data(), n);
            outL = std::abs(l[24000]);
            outR = std::abs(r[24000]);
        };

        double l, r;
        measure(0, 0, l, r);
        check(std::abs(l - 1.0) < 1e-4 && r < 1e-6, "pans 0: L %.4f R %.4f, expected 1 / 0", l, r);
        measure(100, 0, l, r);
        check(l < 1e-6 && std::abs(r - 1.0) < 1e-4, "pan L-R 100: L %.4f R %.4f, expected 0 / 1", l, r);
        measure(50, 0, l, r);
        check(std::abs(l - std::sqrt(0.5)) < 1e-4 && std::abs(r - std::sqrt(0.5)) < 1e-4, "pan L-R 50: L %.4f R %.4f, expected 0.7071 each", l, r);
        measure(0, 100, l, r);
        check(std::abs(l - 1.0) < 1e-4 && r < 1e-6, "pan R-L 100 with left-only input: L %.4f R %.4f, expected 1 / 0", l, r);
    }

    void testMonoAndSlowTempo()
    {
        std::printf("mono works; a whole note at 30 bpm (8 s) is not clamped\n");
        Settings s;
        s.feedback = 0; s.degrad = 0;
        auto d = make(s, 48000);
        std::vector<float> m(48000, 0.0f);
        m[0] = 1.0f;
        d->process(m.data(), nullptr, static_cast<int>(m.size()));
        check(std::abs(m[24000] - 1.0f) < 1e-4f, "mono repeat is %.4f", m[24000]);

        s.time = 96; s.bpm = 30;
        auto slow = make(s, 48000);
        check(std::abs(slow->getDelayTimeSamples() - 384000.0f) < 1.0f, "delay is %.0f samples, expected 384000", slow->getDelayTimeSamples());
    }
}

int main()
{
    testNoRunaway();
    testSilenceStaysSilent();
    testBoundedUnderSustainedInput();
    testImpulseTiming();
    testFeedbackZeroGivesOneRepeat();
    testFilterPeakIsUnity();
    testMixAndReset();
    testPans();
    testMonoAndSlowTempo();

    if (failures == 0)
        std::printf("\nALL PASSED\n");
    else
        std::printf("\n%d FAILED\n", failures);

    return failures == 0 ? 0 : 1;
}
