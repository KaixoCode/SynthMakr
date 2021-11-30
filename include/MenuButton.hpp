#pragma once
#include "pch.hpp"

struct MenuButton : public Button::Graphics
{
	StateColors color{ {
		.base{ 255, 255, 255, 0 },
		.colors{ { { Hovering, { 255, 255, 255, 20 } }, { Pressed, { 255, 255, 255, 30 } } } }
	} };

	void Link(Button* b) override
	{
		color.Link(b);
	}

	void Update() override
	{
		button->min.height = 20;
		button->min.width = GraphicsBase::StringWidth(button->settings.name, GraphicsBase::DefaultFont, 12) + 10;
	}

	void Render(CommandCollection& d) const override
	{
		d.Fill(color.Current());
		d.Quad(button->dimensions);
		if (button->settings.type != Button::Radio || button->State(Selected))
			d.Fill({ 255, 255, 255 });

		else
			d.Fill({ 120, 120, 120 });

		d.FontSize(12);
		d.TextAlign(Align::CenterY | Align::Left);
		d.Font(GraphicsBase::DefaultFont);
		d.Text(button->settings.name, { button->x + 5, button->y + button->height / 2 });
	}
};
