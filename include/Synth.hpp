#pragma once
#include "pch.hpp"
#include "MenuButton.hpp"
#include "Modules.hpp"
#include "Parameter.hpp"

struct Synth : public Frame
{
	struct VoiceBase
	{
		using ChainFun = Function<Sample(Sample, Channel)>;

		virtual ChainFun Chain() = 0;
		virtual void Mod() { };
		virtual void NotePress(int note, int velocity) = 0;
		virtual void NoteRelease(int note) = 0;
		virtual bool Done() = 0;

		template<std::derived_from<Module> Ty, class ...Args>
		Pointer<Ty> Add(Args&& ...args)
		{
			return m_Modules.emplace_back(new Ty{ std::forward<Args>(args)... });
		}

		template<std::derived_from<Module> Ty>
		Pointer<Ty> Add(const typename Ty::Settings& settings)
		{
			return m_Modules.emplace_back(new Ty{ settings });
		}

		void Init() { m_Chain = Chain(); }

	private:
		ChainFun m_Chain;
		std::list<Pointer<Module>> m_Modules;

		Sample m_Process(Sample sample, Channel channel);
		friend class Synth;
	};

	template<class Parent>
	struct Voice : VoiceBase
	{
		Voice(Synth* p) : synth(*dynamic_cast<Parent*>(p)) { }
		Parent& synth;
	};

	class VoiceBank
	{
	public:
		template<class Ty>
		void AddVoices(int voices, Synth* parent)
		{
			m_Pressed.reserve(m_Pressed.size() + voices);
			m_Notes.reserve(m_Notes.size() + voices);
			for (int i = 0; i < voices; i++)
				m_Notes.push_back(-1),
				m_Available.push_back(i),
				m_GeneratorVoices.emplace_back(new Ty{ parent });

			for (auto& i : m_GeneratorVoices)
				i->Init();
		}

		void NotePress(int note, int velocity);
		void NoteRelease(int note, int velocity);
		Sample Process(Sample sample, Channel channel);
		std::vector<Pointer<VoiceBase>>& Voices() { return m_GeneratorVoices; }

	private:
		std::vector<Pointer<VoiceBase>> m_GeneratorVoices;

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
	void AddVoices(int count) { m_Voices.AddVoices<Ty>(count, this); }
	virtual Sample Process(Sample s, Channel channel) { return s; }

private:
	Sample m_Process(Sample sample, Channel channel);

	VoiceBank m_Voices;
	MidiIn<Windows> m_Midi;
	Stream<Wasapi> m_Stream;
	Menu m_Menu;
	Menu m_Menu2;
};
