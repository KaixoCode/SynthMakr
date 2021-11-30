#pragma once
#include "pch.hpp"
#include "Utils.hpp"

using Wavetable = Function<Sample(double, double)>;
namespace Wavetables
{
	Sample sine(double phase, double wtpos);
	Sample saw(double phase, double wtpos);
	Sample square(double phase, double wtpos);
}

class Module
{
public:
	static inline double SAMPLE_RATE = 44100.;

	virtual Sample Apply(Sample sample = 0, int channel = 0) { return sample; };
	virtual void Generate(int channel = 0) {};
};

template<std::invocable<Sample, Channel> T1, class T2>
auto operator |(T1 t1, T2& t2) { return [t1, &t2](Sample s, Channel c) { return t2.Apply(t1(s, c), c); }; }

template<class T1, class T2>
auto operator |(T1& t1, T2& t2) { return [&](Sample s, Channel c) { return t2.Apply(t1.Apply(s, c), c); }; }

class Envelope : public Module
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
	} settings;

	ADSR(const Settings& s = {});

	Sample Apply(Sample s, Channel) override { return m_Sample * s; }
	void Generate(Channel) override;
	void Trigger() override;
	void Gate(bool g) override;
	bool Done() override { return m_Phase == -1; }

private:
	Sample m_Sample = 0;
	Sample m_Down = 0;
	double m_Phase = -1;
	bool m_Gate = false;
};

class Oscillator : public Module
{
public:
	float frequency = 440;
	float phase = 0;
	float wtpos = 0;
	Function<Sample(double, double)> wavetable = Wavetables::saw;

	void Generate(Channel) override;
	Sample Apply(Sample s = 0, Channel = 0) override;
	Sample Offset(double phaseoffset);

private:
	Sample m_Sample = 0;
};

class Chorus : public Module
{
	constexpr static int BUFFER_SIZE = 2048;
	Oscillator m_Oscillator;
	std::vector<std::vector<float>> m_Buffers;
	int m_Position = 0;
	int m_Delay1t = 5;
	int m_Delay2t = 5;

public:
	double mix = 0.5; // Percent
	double amount = 1;
	double feedback = 0; // Percent
	double frequency = 3; // Hz
	double delay1 = 5; // milliseconds
	double delay2 = 5; // milliseconds
	bool stereo = true;
	bool enableDelay2 = true;
	enum Polarity { Positive = 1, Negative = -1 } polarity = Negative;

	void Channels(int c);
	Sample Apply(Sample sin, Channel c) override;
};