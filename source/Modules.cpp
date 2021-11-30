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

ADSR::ADSR(const Settings& s)
	: settings(s)
{}

void ADSR::Generate(Channel)
{
	if (m_Phase >= 0 && (m_Phase < settings.attack + settings.decay || !m_Gate))
		m_Phase += 1.0 / (double)SAMPLE_RATE;

	else if (m_Gate)
		m_Phase = settings.attack + settings.decay;

	if (m_Phase > settings.attack + settings.decay + settings.release) m_Phase = -1;

	m_Sample = m_Phase < 0 ? 0
		: m_Phase < settings.attack ? std::pow(m_Phase / settings.attack, settings.attackCurve)
		: m_Phase <= settings.attack + settings.decay ? 1 - (1 - settings.sustain) * std::pow((m_Phase - settings.attack) / settings.decay, settings.decayCurve)
		: m_Phase < settings.attack + settings.decay + settings.release ? m_Down - m_Down * std::pow((m_Phase - settings.attack - settings.decay) / settings.release, settings.releaseCurve)
		: 0;
}

void ADSR::Trigger()
{
	m_Down = settings.sustain;
	m_Phase = 0;
}
void ADSR::Gate(bool g)
{
	if (m_Gate && !g) {
		m_Phase = settings.attack + settings.decay;
		m_Down = m_Sample;
	}
	m_Gate = g;
}

// Oscillator

void Oscillator::Generate(Channel)
{
	m_Sample = wavetable(phase, wtpos);

	double delta = frequency / SAMPLE_RATE;
	phase = std::fmod(1 + phase + delta, 1);
}

Sample Oscillator::Offset(double phaseoffset)
{
	return wavetable(std::fmod(1 + phase + phaseoffset, 1), wtpos);
}

Sample Oscillator::Apply(Sample s, Channel) 
{
	return m_Sample + s;
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
	m_Oscillator.frequency = frequency;
	Channels(c + 1);

	if (c == 0)
	{
		m_Position = (m_Position + 1) % BUFFER_SIZE;
		m_Oscillator.Generate(c);
	}

	m_Delay1t = ((delay1 + m_Oscillator.Offset(stereo ? (c % 2) * 0.5 : 0) * amount) / 1000.0) * SAMPLE_RATE;
	m_Delay2t = ((delay2 + m_Oscillator.Offset(stereo ? (c % 2) * 0.5 : 0) * amount) / 1000.0) * SAMPLE_RATE;

	m_Delay1t = (std::max(m_Delay1t, 1)) % BUFFER_SIZE;
	m_Delay2t = (std::max(m_Delay2t, 1)) % BUFFER_SIZE;

	auto& _buffer = m_Buffers[c];

	if (enableDelay2)
	{
		int i1 = (m_Position - m_Delay1t + BUFFER_SIZE) % BUFFER_SIZE;
		int i2 = (m_Position - m_Delay2t + BUFFER_SIZE) % BUFFER_SIZE;

		float del1s = _buffer[i1];
		float del2s = _buffer[i2];

		float now = (del1s + del2s) / 2.0;

		_buffer[m_Position] = sin + polarity * now * feedback;

		return sin * (1.0 - mix) + now * mix;
	}
	else
	{
		int i1 = (m_Position - m_Delay1t + BUFFER_SIZE) % BUFFER_SIZE;

		float del1s = _buffer[i1];

		float now = del1s;

		_buffer[m_Position] = sin + polarity * now * feedback;

		return sin * (1.0 - mix) + now * mix;
	}
};