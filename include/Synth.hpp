#pragma once
#include "pch.hpp"
#include "MenuButton.hpp"
#include "Modules.hpp"

struct Synth : public Frame
{
	struct Voice
	{
		using ChainFun = Function<Sample(Sample, Channel)>;

		virtual ChainFun Chain() = 0;
		virtual void NotePress(int note, int velocity) = 0;
		virtual void NoteRelease(int note) = 0;
		virtual bool Done() = 0;

		template<class Ty, class ...Args>
		Pointer<Ty> Add(Args&& ...args)
		{
			return m_Modules.emplace_back(new Ty{ std::forward<Args>(args)... });
		}

		void Init() { m_Chain = Chain(); }

	private:
		Sample m_Process(Sample sample, Channel channel);

		ChainFun m_Chain;
		std::list<Pointer<Module>> m_Modules;
		friend class Synth;
	};

	class VoiceBank
	{
	public:
		template<class Ty>
		void AddVoices(int voices)
		{
			m_Pressed.reserve(m_Pressed.size() + voices);
			m_Notes.reserve(m_Notes.size() + voices);
			for (int i = 0; i < voices; i++)
				m_Notes.push_back(-1),
				m_Available.push_back(i),
				m_GeneratorVoices.emplace_back(new Ty{});

			for (auto& i : m_GeneratorVoices)
				i->Init();
		}

		void NotePress(int note, int velocity);
		void NoteRelease(int note, int velocity);
		Sample Process(Sample sample, Channel channel);
		std::vector<Pointer<Voice>>& Voices() { return m_GeneratorVoices; }

	private:
		std::vector<Pointer<Voice>> m_GeneratorVoices;

		std::vector<int> m_Notes;
		std::vector<int> m_Pressed;
		std::vector<int> m_Available;
	};

	struct Settings
	{
		std::string name = "Synth";
	} settings;

	Synth(const Settings& s = {});

	template<class Ty>
	void AddVoices(int count) { m_Voices.AddVoices<Ty>(count); }
	virtual Sample Process(Sample s, Channel channel) { return s; }

private:
	Sample m_Process(Sample sample, Channel channel);

	VoiceBank m_Voices;
	MidiIn<Windows> m_Midi;
	Stream<Wasapi> m_Stream;
	Menu m_Menu;
	Menu m_Menu2;
};
