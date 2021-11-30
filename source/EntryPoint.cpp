#include "pch.hpp"
#include "Synth.hpp"

template<class Ty = double>
struct Parameter : public Component
{
	struct Settings
	{
		Ty& link;

	} settings;

	Parameter(const Settings& s)
		: settings(s)
	{}
};


struct MySynth : public Synth
{
	struct MyVoice : Voice
	{
		Oscillator& osc = Add<Oscillator>();
		ADSR& adsr = Add<ADSR>();
		Chorus& chorus = Add<Chorus>();

		ChainFun Chain() override { return osc | adsr | chorus; }

		void NotePress(int n, int velocity) override
		{
			osc.frequency = noteToFreq(n);
			adsr.Gate(true);
			adsr.Trigger();
		}

		void NoteRelease(int n) override { adsr.Gate(false); }
		bool Done() override { return adsr.Done(); }
	};

	MySynth()
		: Synth({ .name = "MySynth" })
	{
		AddVoices<MyVoice>(8);
	}

	Sample Process(Sample s, Channel channel)
	{
		return s * 0.2;
	}
};

int main()
{
	Gui _gui;
	_gui.emplace<MySynth>().Create();

	while (_gui.Loop());
}
