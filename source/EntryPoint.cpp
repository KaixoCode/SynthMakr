#include "pch.hpp"
#include "Synth.hpp"

struct MySynth : public Synth
{
	struct MyVoice : Voice<MySynth>
	{
		using Voice<MySynth>::Voice;

		Oscillator& osc = Add<Oscillator>();

		Oscillator& lfo = Add<Oscillator>({ 
			.frequency = 0.5, 
			.wavetable = Wavetables::sine 
		});

		ADSR& gain = Add<ADSR>();

		ADSR& filter = Add<ADSR>({
			.attack = 0.5,
			.decay = 5.5, 
			.sustain = 0, 
			.attackCurve = 0.9,
			.decayCurve = 0.2,
			.legato = true
		});

		Chorus& chorus = Add<Chorus>({ 
			.oscillator{ {
				.frequency = 3, 
				.wavetable = Wavetables::sine 
			} } 
		});

		LPF& lowpass = Add<LPF>({
			.resonance = 1 
		});

		ChainFun Chain() override 
		{
			return osc >> gain >> lowpass >> chorus;
		}

		void Mod() override
		{
			chorus.settings.mix = synth.chorusMix / 100.;
			lowpass.settings.mix = synth.filterMix / 100.;
			lfo.settings.frequency = 3 - filter * 3;
			lowpass.settings.frequency = filter * filter * 16000 + 200;
			lowpass.settings.frequency += lfo * 400 + 300;
		}

		void NotePress(int n, int velocity) override
		{
			osc.settings.frequency = noteToFreq(n);
			gain.Gate(true);
			filter.Gate(true);
		}

		void NoteRelease(int n) override { gain.Gate(false); filter.Gate(false); }
		bool Done() override { return gain.Done() && filter.Done(); }
	};

	Parameter& chorusMix = emplace_back<Parameter>({
		.value = 50,
		.reset = 50,
		.range = { 0, 100 },
		.name = "Chorus",
		.unit = Units::PERCENT,
		.decimals = 1,
	});
	
	Parameter& filterMix = emplace_back<Parameter>({
		.value = 100,
		.reset = 100,
		.range = { 0, 100 },
		.name = "Filter",
		.unit = Units::PERCENT,
		.decimals = 1,
	});

	Parameter& gain = emplace_back<Parameter>({
		.value = 0,
		.reset = 0,
		.range = { -24, 24 },
		.name = "Gain",
		.unit = Units::DECIBEL,
		.decimals = 1,
	});

	MySynth()
		: Synth({ .name = "MySynth" })
	{
		AddVoices<MyVoice>(8);
		background = { 40, 40, 40, 255 };
		titlebar.background = { 40, 40, 40, 255 };
		panel = Panel{ {.ratio = 1, .padding{ 8, 8, 8, 8 }, .margin{ 8, 8, 8, 8 }, .background{{.base{ 64, 64, 64, 255 }}} },
			{ { 
				new Panel{ {.size{ Auto, Auto } }, gain },
				new Panel{ {.size{ Auto, Auto } }, chorusMix },
				new Panel{ {.size{ Auto, Auto } }, filterMix },
			} }
		};
	}

	Sample Process(Sample s, Channel channel) { return db2lin(gain) * s * 0.2; }
};



int main()
{
	Gui _gui;
	_gui.emplace<MySynth>().Create();

	while (_gui.Loop());
}
