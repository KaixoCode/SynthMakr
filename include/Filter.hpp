#pragma once
#include <algorithm>
#include <cmath>
#include <vector>

#define constrain(x, y, z) (x < y ? y : x > z ? z : x)

enum class FilterType
{
	Off, LowPass, HighPass, BandPass, Notch, AllPass, PeakingEQ, LowShelf, HighShelf, ITEMS
};

template<typename T>
class Filter
{
public:
	using Params = T;
	virtual float Apply(float s, Params& p) = 0;
};

class FilterParameters
{
public:
	virtual void RecalculateParameters() = 0;
};

template<size_t M>
class IIRFilterParameters : public FilterParameters
{
public:

	// Coefficients
	double A[M + 1];
	double B[M + 1];
};

class BiquadParameters
{
public:
	union { double Q, BW, S = 1; };
	double f0 = 22000, dbgain = 0;
	double sampleRate = 48000;
	FilterType type = FilterType::Off;

	void RecalculateParameters()
	{
		w0 = 6.28318530718 * (constrain(f0, 10, sampleRate / 2.1) / sampleRate);
		cosw0 = std::cos(w0), sinw0 = std::sin(w0);

		switch (type) {
		case FilterType::Off:
		{
			b0 = 1, a0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
		} break;
		case FilterType::LowPass:
		{
			alpha = sinw0 / (2.0 * Q);
			//alpha = sinw0 * std::sinh((log2 / 2.0) * BW * (w0 / sinw0));
			b0 = (1.0 - cosw0) / 2.0, b1 = 1.0 - cosw0, b2 = b0;
			a0 = 1.0 + alpha, a1 = -2.0 * cosw0, a2 = 1.0 - alpha;
		} break;
		case FilterType::HighPass:
		{
			alpha = sinw0 / (2.0 * Q);
			b0 = (1.0 + cosw0) / 2.0, b1 = -(1.0 + cosw0), b2 = b0;
			a0 = 1.0 + alpha, a1 = -2.0 * cosw0, a2 = 1.0 - alpha;
		} break;
		case FilterType::BandPass:
		{
			alpha = sinw0 * std::sinh((log2 / 2.0) * BW * (w0 / sinw0));
			b0 = sinw0 / 2.0, b1 = 0.0, b2 = -b0;
			a0 = 1.0 + alpha, a1 = -2.0 * cosw0, a2 = 1.0 - alpha;
		} break;
		case FilterType::Notch:
		{
			alpha = sinw0 * std::sinh((log2 / 2.0) * BW * (w0 / sinw0));
			b0 = 1, b1 = -2.0 * cosw0, b2 = 1.0;
			a0 = 1.0 + alpha, a1 = -2.0 * cosw0, a2 = 1.0 - alpha;
		} break;
		case FilterType::AllPass:
		{
			alpha = sinw0 / (2.0 * Q);
			b0 = 1.0 - alpha, b1 = -2.0 * cosw0, b2 = 1.0 + alpha;
			a0 = 1.0 + alpha, a1 = -2.0 * cosw0, a2 = 1.0 - alpha;
		} break;
		case FilterType::PeakingEQ:
		{
			A = std::pow(10, dbgain / 40.0);
			alpha = sinw0 * std::sinh((log2 / 2.0) * BW * (w0 / sinw0));
			b0 = 1.0 + alpha * A, b1 = -2.0 * cosw0, b2 = 1.0 - alpha * A;
			a0 = 1.0 + alpha / A, a1 = -2.0 * cosw0, a2 = 1.0 - alpha / A;
		} break;
		case FilterType::LowShelf:
		{
			A = std::pow(10, dbgain / 40.0);
			double t = std::max((A + 1.0 / A) * (1.0 / S - 1.0) + 2, 0.0);
			alpha = (sinw0 / 2.0) * std::sqrt(t);
			double sqrtAa = std::sqrt(A) * alpha;
			b0 = A * ((A + 1.0) - (A - 1.0) * cosw0 + 2.0 * sqrtAa);
			b1 = 2.0 * A * ((A - 1.0) - (A + 1.0) * cosw0);
			b2 = A * ((A + 1.0) - (A - 1.0) * cosw0 - 2.0 * sqrtAa);
			a0 = (A + 1.0) + (A - 1.0) * cosw0 + 2.0 * sqrtAa;
			a1 = -2.0 * ((A - 1.0) + (A + 1.0) * cosw0);
			a2 = (A + 1.0) + (A - 1.0) * cosw0 - 2.0 * sqrtAa;
		} break;
		case FilterType::HighShelf:
		{
			A = std::pow(10, dbgain / 40.0);
			double t = std::max((A + 1.0 / A) * (1.0 / S - 1.0) + 2, 0.0);
			alpha = (sinw0 / 2.0) * std::sqrt(t);
			double sqrtAa = std::sqrt(A) * alpha;
			b0 = A * ((A + 1.0) + (A - 1.0) * cosw0 + 2.0 * sqrtAa);
			b1 = -2.0 * A * ((A - 1.0) + (A + 1.0) * cosw0);
			b2 = A * ((A + 1.0) + (A - 1.0) * cosw0 - 2.0 * sqrtAa);
			a0 = (A + 1.0) - (A - 1.0) * cosw0 + 2.0 * sqrtAa;
			a1 = 2.0 * ((A - 1.0) - (A + 1) * cosw0);
			a2 = (A + 1.0) - (A - 1.0) * cosw0 - 2.0 * sqrtAa;
		}
		}
		b0a0 = b0 / a0, b1a0 = b1 / a0, b2a0 = b2 / a0, a1a0 = a1 / a0, a2a0 = a2 / a0;
	}

	constexpr static double log2 = 0.30102999566;
	double b0 = 1, b1 = 0, b2 = 0, a0 = 1, a1 = 0, a2 = 0;
	double b0a0 = 0, b1a0 = 0, b2a0 = 0, a1a0 = 0, a2a0 = 0;
	double w0 = 0, cosw0 = 0, sinw0 = 0, A = 0, alpha = 0;
};

template<typename P = BiquadParameters>
class BiquadFilter : public Filter<P>
{
public:
	float Apply(float s, P& p) override
	{
		x[0] = s;
		y[0] = p.b0a0 * x[0] + p.b1a0 * x[1] + p.b2a0 * x[2] - p.a1a0 * y[1] - p.a2a0 * y[2];

		for (int i = sizeof(y) / sizeof(double) - 2; i >= 0; i--)
			y[i + 1] = y[i];

		for (int i = sizeof(x) / sizeof(double) - 2; i >= 0; i--)
			x[i + 1] = x[i];

		return y[0];
	}

private:
	double y[3]{ 0, 0, 0 }, x[3]{ 0, 0, 0 };
};

template<size_t N, class F, class P = F::Params>
class ChannelEqualizer
{
public:
	ChannelEqualizer(std::vector<P>& a)
		: m_Params(a), m_Filters()
	{}

	float Apply(float s)
	{
		for (int i = 0; i < N; i++)
			if (m_Params[i].type != FilterType::Off)
				s = m_Filters[i].Apply(s, m_Params[i]);

		return s;
	}

	std::vector<P>& m_Params;
	F m_Filters[N];
};


template<size_t N, class F, class P = F::Params>
class StereoEqualizer
{
public:
	StereoEqualizer(P& a)
		: m_Params(a), m_Filters()
	{}

	float Apply(float s, int c)
	{
		int channel = c % N;
		if (m_Params.type != FilterType::Off)
			s = m_Filters[channel].Apply(s, m_Params);

		return s;
	}

	P& m_Params;
	F m_Filters[N];
};