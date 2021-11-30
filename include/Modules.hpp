#pragma once
#include "pch.hpp"
#include "Utils.hpp"
#include "Filter.hpp"

enum Polarity { Positive = 1, Negative = -1 };

using Wavetable = Function<Sample(double, double)>;
namespace Wavetables
{
    Sample sine(double phase, double wtpos);
    Sample saw(double phase, double wtpos);
    Sample square(double phase, double wtpos);
}

struct Range
{
    double middle = 0;
    double range = 1;
};

class Module
{
public:
    static inline double SAMPLE_RATE = 44100.;

    virtual Sample Apply(Sample sample = 0, Channel channel = 0) { return sample; };
    virtual void Generate(Channel channel = 0) {};
};

class Generator : public Module
{
public:
    auto operator()(Range range)
    {
        return [this, range]()
        {
            return sample * range.range + range.middle;
        };
    }

    template<std::invocable<Sample> T>
    auto operator()(T&& fun)
    {
        return [this, fun]()
        {
            return fun(sample);
        };
    }

    operator Sample&() { return sample; }

    Sample sample = 0;
};

template<std::invocable<Sample, Channel> T1, std::derived_from<Module> T2>
auto operator >>(T1&& t1, T2& t2) { return [t1 = std::move(t1), &t2](Sample s, Channel c) mutable { return t2.Apply(t1(s, c), c); }; }

template<std::invocable<Sample, Channel> T1, std::invocable<Sample, Channel> T2>
auto operator >>(T1&& t1, T2&& t2) { return [t1 = std::move(t1), t2 = std::move(t2)](Sample s, Channel c) mutable { return t2(t1(s, c), c); }; }

template<std::derived_from<Module> T1, std::derived_from<Module> T2>
auto operator >>(T1& t1, T2& t2) { return [&](Sample s, Channel c) { return t2.Apply(t1.Apply(s, c), c); }; }

class Envelope : public Generator
{
public:
    virtual void Trigger() = 0;
    virtual void Gate(bool) = 0;
    virtual bool Done() { return true; }
};

class ADSR : public Envelope
{
public:
    struct Settings
    {
        double attack = 0.02; // seconds
        double decay = 0.1;   // seconds
        double sustain = 1.0; // between 0 and 1
        double release = 0.1; // seconds

        double attackCurve = 0.5;
        double decayCurve = 0.5;
        double releaseCurve = 0.5;

        bool legato = false;
    } settings;

    ADSR(const Settings& s = {}) : settings(s) {};

    Sample Apply(Sample s, Channel) override { return sample * s; }
    void Generate(Channel) override;
    void Trigger() override;
    void Gate(bool g) override;
    bool Done() override { return m_Phase == -1; }

private:
    Sample m_Down = 0;
    double m_Phase = -1;
    bool m_Gate = false;
};

class LPF : public Module
{
public:
    struct Settings
    {
        double frequency = 2000;
        double resonance = 1;
        double mix = 1;
    } settings;
    
    LPF(const Settings& s = {}) : settings(s) {}

    void Generate(Channel) override;
    Sample Apply(Sample s, Channel) override;

private:
    BiquadParameters m_Params;
    StereoEqualizer<2, BiquadFilter<>> m_Filter{ m_Params };
};

class Oscillator : public Generator
{
public:
    struct Settings
    {
        float frequency = 440;
        float wtpos = 0;
        int oversample = 8;
        Function<Sample(double, double)> wavetable = Wavetables::saw;
    } settings;

    Oscillator(const Settings& s = {}) : settings(s) {}

    void Generate(Channel) override;
    Sample Apply(Sample s = 0, Channel = 0) override;
    Sample Offset(double phaseoffset);

private:
    BiquadParameters m_Params;
    BiquadFilter<> m_Filter[1];
    float m_Phase = 0;
};

class Chorus : public Module
{
public:
    struct Settings
    {
        Oscillator oscillator;
        double mix = 0.5; // Percent
        double amount = 1;
        double feedback = 0; // Percent
        double delay1 = 5; // milliseconds
        double delay2 = 5; // milliseconds
        bool stereo = true;
        bool enableDelay2 = true;
        Polarity polarity = Negative;
    } settings;

    Chorus(const Settings& s = {}) : settings(s) {}

    void Channels(int c);
    Sample Apply(Sample sin, Channel c) override;

private:
    constexpr static int BUFFER_SIZE = 2048;
    std::vector<std::vector<float>> m_Buffers;
    int m_Position = 0;
    int m_Delay1t = 5;
    int m_Delay2t = 5;
};

class Delay : public Module
{
public:
    struct Settings
    {
        double mix = 0.5; // Percent
        double delay = 500; // Milliseconds
        double feedback = 0.4; // Percent
        double gain = 0; // Input gain in decibel
        bool stereo = false;
        bool filter = true;
        struct {
            double amount = 0;
            double rate = 0.4;
        } mod;

    } settings;

    Delay(const Settings& s = {}) : settings(s) {}

    void Channels(int c);
    void Generate(Channel c) override;
    Sample Apply(Sample sin, Channel c) override;

private:
    std::vector<std::vector<float>> m_Buffers;
    int BUFFER_SIZE = SAMPLE_RATE * 10;
    int m_Position = 0;

    Oscillator m_Oscillator{ { .wavetable = Wavetables::sine } };

    SimpleFilterParameters m_Parameters;
    std::vector<ChannelEqualizer<2, BiquadFilter<>>> m_Equalizers;

    bool m_Dragging = false;
};

class Gain : public Module
{
public:
    struct Settings
    {
        double gain = 0;
    } settings;

    Gain(const Settings& s = {}) : settings(s) {}
    Sample Apply(Sample s, Channel) override { return db2lin(settings.gain) * s; }
};