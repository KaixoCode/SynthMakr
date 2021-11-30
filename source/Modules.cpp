#include "Modules.hpp"

namespace Wavetables
{
    Sample sine(double phase, double wtpos)
    {
        assert(phase >= 0 && phase <= 1);
        return std::sin(phase * std::numbers::pi_v<double> *2);
    };

    Sample saw(double phase, double wtpos)
    {
        assert(phase >= 0 && phase <= 1);
        return phase * 2 - 1;
    };

    Sample square(double phase, double wtpos)
    {
        assert(phase >= 0 && phase <= 1);
        return std::floor(phase + 0.5);
    };
}

// ADSR

void ADSR::Generate(Channel)
{
    if (m_Phase >= 0 && (m_Phase < settings.attack + settings.decay || !m_Gate))
        m_Phase += 1.0 / (double)SAMPLE_RATE;

    else if (m_Gate)
        m_Phase = settings.attack + settings.decay;

    if (m_Phase > settings.attack + settings.decay + settings.release) m_Phase = -1;

    sample = m_Phase < 0 ? 0
        : m_Phase < settings.attack ? std::pow(m_Phase / settings.attack, settings.attackCurve)
        : m_Phase <= settings.attack + settings.decay ? 1 - (1 - settings.sustain) * std::pow((m_Phase - settings.attack) / settings.decay, settings.decayCurve)
        : m_Phase < settings.attack + settings.decay + settings.release ? m_Down - m_Down * std::pow((m_Phase - settings.attack - settings.decay) / settings.release, settings.releaseCurve)
        : 0;
}

void ADSR::Trigger()
{
    m_Down = settings.sustain;
    if (m_Gate && settings.legato)
        m_Phase = std::min(m_Phase, settings.attack + settings.decay);
    else
        m_Phase = 0;
}

void ADSR::Gate(bool g)
{
    if (m_Gate && !g)
    {
        m_Phase = settings.attack + settings.decay;
        m_Down = sample;
    }
    else if (!m_Gate && g)
    {
        Trigger();
    }
    else if (!settings.legato)
        m_Phase = 0;

    m_Gate = g;
}

// Oscillator

void Oscillator::Generate(Channel c)
{
    if (c != 0)
        return;
    Sample _avg = 0;
    m_Params.sampleRate = SAMPLE_RATE * settings.oversample;
    m_Params.f0 = SAMPLE_RATE * 0.4;
    m_Params.Q = 1;
    m_Params.type = FilterType::LowPass;
    m_Params.RecalculateParameters();

    for (int i = 0; i < settings.oversample; i++)
    {
        float _s = settings.wavetable(m_Phase, settings.wtpos);
        for (auto& i : m_Filter)
            _s = i.Apply(_s, m_Params);
        _avg += _s;
        double delta = settings.frequency / (SAMPLE_RATE * settings.oversample);
        m_Phase = std::fmod(1 + m_Phase + delta, 1);
    }

    sample = _avg /= settings.oversample;
}

Sample Oscillator::Offset(double phaseoffset)
{
    return settings.wavetable(std::fmod(1 + m_Phase + phaseoffset, 1), settings.wtpos);
}

Sample Oscillator::Apply(Sample s, Channel) 
{
    return sample + s;
}

// Chorus

void Chorus::Channels(int c)
{
    m_Buffers.reserve(c);
    while (m_Buffers.size() < c)
    {
        auto& a = m_Buffers.emplace_back();
        while (a.size() < BUFFER_SIZE)
            a.emplace_back(0);
    }
}

Sample Chorus::Apply(Sample sin, Channel c)
{
    Channels(c + 1);

    if (c == 0)
    {
        m_Position = (m_Position + 1) % BUFFER_SIZE;
        settings.oscillator.Generate(c);
    }

    m_Delay1t = ((settings.delay1 + settings.oscillator.Offset(settings.stereo ? (c % 2) * 0.5 : 0) * settings.amount) / 1000.0) * SAMPLE_RATE;
    m_Delay2t = ((settings.delay2 + settings.oscillator.Offset(settings.stereo ? (c % 2) * 0.5 : 0) * settings.amount) / 1000.0) * SAMPLE_RATE;

    m_Delay1t = (std::max(m_Delay1t, 1)) % BUFFER_SIZE;
    m_Delay2t = (std::max(m_Delay2t, 1)) % BUFFER_SIZE;

    auto& _buffer = m_Buffers[c];

    if (settings.enableDelay2)
    {
        int i1 = (m_Position - m_Delay1t + BUFFER_SIZE) % BUFFER_SIZE;
        int i2 = (m_Position - m_Delay2t + BUFFER_SIZE) % BUFFER_SIZE;

        float del1s = _buffer[i1];
        float del2s = _buffer[i2];

        float now = (del1s + del2s) / 2.0;

        _buffer[m_Position] = sin + settings.polarity * now * settings.feedback;

        return sin * (1.0 - settings.mix) + now * settings.mix;
    }
    else
    {
        int i1 = (m_Position - m_Delay1t + BUFFER_SIZE) % BUFFER_SIZE;

        float del1s = _buffer[i1];

        float now = del1s;

        _buffer[m_Position] = sin + settings.polarity * now * settings.feedback;

        return sin * (1.0 - settings.mix) + now * settings.mix;
    }
};

// LPF

void LPF::Generate(Channel) 
{
    m_Params.sampleRate = SAMPLE_RATE;
    m_Params.type = FilterType::LowPass;
    m_Params.f0 = settings.frequency;
    m_Params.Q = settings.resonance;
    m_Params.RecalculateParameters();
}

Sample LPF::Apply(Sample s, Channel c) 
{
    return m_Filter.Apply(s, c) * settings.mix + s * (1 - settings.mix);
}

// Delay

void Delay::Channels(int c)
{
    BUFFER_SIZE = SAMPLE_RATE * 10;
    m_Equalizers.reserve(c);
    while (m_Equalizers.size() < c)
        m_Equalizers.emplace_back(m_Parameters.Parameters());

    m_Buffers.reserve(c);
    while (m_Buffers.size() < c)
    {
        auto& a = m_Buffers.emplace_back();
        while (a.size() < BUFFER_SIZE)
            a.emplace_back(0);
    }
}

void Delay::Generate(Channel c)
{
    Channels(c + 1);
    m_Parameters.RecalculateParameters();
}

Sample Delay::Apply(Sample sin, Channel c)
{
    float in = sin * db2lin(settings.gain);
    if (c == 0)
    {
        m_Oscillator.settings.frequency = settings.mod.rate;
        m_Position = (m_Position + 1) % BUFFER_SIZE;
        m_Oscillator.Generate(c);
    }

    int s = settings.stereo ? ((c % 2) * 0.5) : 0;
    int delayt = ((settings.delay + settings.delay * m_Oscillator * settings.mod.amount * 0.01 * 0.9) / 1000.0) * SAMPLE_RATE;
    delayt = (std::max(delayt, 1)) % BUFFER_SIZE;

    auto& _buffer = m_Buffers[c];
    int i1 = (int)(m_Position - (settings.stereo ? (delayt + delayt * (c % 2) * 0.5) : delayt) + 3 * BUFFER_SIZE) % BUFFER_SIZE;

    float del1s = _buffer[i1];

    float now = settings.filter ? m_Equalizers[c].Apply(del1s) : del1s;

    int next = settings.stereo ? ((int)(m_Position - (c % 2) * delayt * 0.5 - 1 + 3 * BUFFER_SIZE) % BUFFER_SIZE) : m_Position;
    _buffer[m_Position] = in;
    _buffer[next] += now * settings.feedback;

    return sin * (1.0 - settings.mix) + now * settings.mix;
}
