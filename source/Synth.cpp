#include "Synth.hpp"

Sample Synth::VoiceBase::m_Process(Sample sample, Channel channel)
{
	for (auto& i : m_Modules)
		i->Generate(channel);

	return m_Chain(sample, channel);
}

void Synth::VoiceBank::NotePress(int note, int velocity)
{
	// Release the longest held note
	if (m_Available.size() == 0)
	{
		int longestheld = m_Pressed.back();
		m_Pressed.pop_back();

		// Set note to -1 and emplace to available.
		m_Notes[longestheld] = -1;
		m_Available.emplace(m_Available.begin(), longestheld);
	}

	// Get an available voice
	if (!m_Available.empty())
	{
		int voice = m_Available.back();
		m_Available.pop_back();

		// Emplace it to pressed voices queue
		m_Pressed.emplace(m_Pressed.begin(), voice);

		// Set voice to note
		m_Notes[voice] = note;
		m_GeneratorVoices[voice]->NotePress(note, velocity);
	}
}

void Synth::VoiceBank::NoteRelease(int note, int velocity)
{
	// Find the note in the pressed notes per voice
	while (true)
	{
		auto it = std::find(m_Notes.begin(), m_Notes.end(), note);
		if (it != m_Notes.end())
		{
			// If it was pressed, get the voice index
			int voice = std::distance(m_Notes.begin(), it);

			// Set note to -1 and emplace to available.
			m_GeneratorVoices[voice]->NoteRelease(note);
			m_Notes[voice] = -1;
			m_Available.emplace(m_Available.begin(), voice);

			// Erase it from the pressed queue
			auto it2 = std::find(m_Pressed.begin(), m_Pressed.end(), voice);
			if (it2 != m_Pressed.end())
				m_Pressed.erase(it2);
		}
		else break;
	}
}

Sample Synth::VoiceBank::Process(Sample sample, Channel channel)
{
	Sample out = 0;
	for (auto& voice : m_GeneratorVoices)
	{
		if (!voice->Done())
		{
			for (auto& j : voice->m_Modules)
				j->Generate(channel);

			voice->Mod();
			if (voice->m_Chain)
				out += voice->m_Chain(sample, channel);
		}
	}
	return out;
}

Synth::Synth(const Settings& s)
	: settings(s), m_Stream(), Frame{ {.name = s.name } }
{
	titlebar.close.color.base.a = 0;
	titlebar.minimize.color.base.a = 0;
	titlebar.maximize.color.base.a = 0;

	*this += [this](const KeyPress& e) {
		if (!e.repeat && keyboard2midi.contains(e.keycode))
			m_Voices.NotePress(keyboard2midi[e.keycode] + 48, 127);
	};

	*this += [this](const KeyRelease& e) {
		if (keyboard2midi.contains(e.keycode))
			m_Voices.NoteRelease(keyboard2midi[e.keycode] + 48, 127);
	};

	m_Stream.Callback([&](Buffer<Sample>&, Buffer<Sample>& out, CallbackInfo info)
	{
		Module::SAMPLE_RATE = info.sampleRate;
		for (auto& i : out)
		{
			int channel = 0;
			for (auto& j : i)
				j = this->m_Process(0, channel++);
		}
	});

	m_Midi.Callback([this](const NoteOn& e) {
		m_Voices.NotePress(e.RawNote(), e.Velocity());
	});

	m_Midi.Callback([this](const NoteOff& e) {
		m_Voices.NoteRelease(e.RawNote(), e.Velocity());
	});

	GuiCode::Button& _b1 = titlebar.menu.emplace_back<GuiCode::Button>({
		.name = "Audio",
		.graphics = new MenuButton
	});

	GuiCode::Button& _b3 = titlebar.menu.emplace_back<GuiCode::Button>({
		.name = "Midi",
		.graphics = new MenuButton
	});

	_b1.settings.callback = [&](bool v) {
		v ? ContextMenu::Open(m_Menu, { _b1.x, _b1.y + _b1.height })
			: ContextMenu::Close(m_Menu);
	};

	_b1 += [this](const Unfocus&) { ContextMenu::Close(m_Menu); };

	_b3.settings.callback = [&](bool v) {
		v ? ContextMenu::Open(m_Menu2, { _b3.x, _b3.y + _b3.height })
			: ContextMenu::Close(m_Menu2);
	};

	_b3 += [this](const Unfocus&) { ContextMenu::Close(m_Menu2); };

	GuiCode::Button::Group group;
	for (auto& i : m_Stream.Devices())
	{
		if (!i.outputChannels)
			continue;

		GuiCode::Button& _b2 = m_Menu.emplace_back<GuiCode::Button>({
			.group = group,
			.type = GuiCode::Button::Radio,
			.name = i.name,
			.graphics = new MenuButton
			});

		_b2.settings.callback = [&](bool b) {
			if (b)
			{
				m_Stream.Close();
				m_Stream.Open({
					.input = NoDevice,
					.output = i.id,
					});
				m_Stream.Start();
			}
		};

		if (i.name.find("System") != std::string::npos)
			_b2.State(Selected) = true, _b2.settings.callback(true);
	}

	GuiCode::Button::Group group2;
	for (auto& i : m_Midi.Devices())
	{
		GuiCode::Button& _b4 = m_Menu2.emplace_back<GuiCode::Button>({
			.group = group2,
			.type = GuiCode::Button::Radio,
			.name = i.name,
			.graphics = new MenuButton
		});

		_b4.settings.callback = [&](bool b) {
			if (b)
			{
				m_Midi.Close();
				if (m_Midi.Open({
					.device = i.id,
				}) != Midijo::NoError) _b4.State(Selected) = false;
			}
		};

		if (i.id == 0)
			_b4.State(Selected) = true, _b4.settings.callback(true);
	}
}

Sample Synth::m_Process(Sample sample, Channel channel)
{
	return Process(m_Voices.Process(sample, channel), channel);
}