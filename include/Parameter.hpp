#pragma once
#include "pch.hpp"
#include "Unit.hpp"

struct Parameter : public Component
{
    struct Settings
    {
        double value{}; // Initial value
        double reset = value; // Value to reset to
        Vec2<double> range{ -24, 24 };  // Range of the parameter

        std::string name = "Param"; // Name of the parameter

        Function<double(double)> scaling = [](double in) { return in; }; // Scaling of the mouse dragging
        Function<double(double)> inverse = [](double in) { return in; }; // Inverse of the scaling of the mouse dragging
        int unit = Units::DECIBEL; // Unit to display after value
        int decimals = 1; // Amount of decimals to display

        float shift = 0.25; // Multiplier for speed of dragging when shift is held

        int fontSize = 14; // Font size for name and value
        std::string font = GraphicsBase::DefaultFont; // Font for name and value

        bool displayName = true; // Display the name
        bool displayValue = true; // Display the value

        StateColors border{ {
            .base { 30, 30, 30, 255 },
        } };

        StateColors line{ {
            .base { 109, 215, 255, 255 },
            .colors { {
                { Hovering, { 109, 215, 255, 255 } },
                { Pressed, { 109, 215, 255, 255 } },
                { Disabled, { 130, 130, 130, 255 } },
            } }
        } };

        StateColors handle{ {
            .base { 30, 30, 30, 255 },
            .colors { {
                { Disabled, { 50, 50, 50, 255 } },
            } }
        } };

        StateColors text{ {
            .base { 210, 210, 210, 255 },
        } };

        StateColors background{ {
            .base { 65, 65, 65, 255 }
        } };

        void Link(Component* p) { border.Link(p); line.Link(p); handle.Link(p); text.Link(p); background.Link(p); }
    } settings;

    Parameter(const Settings& s = {})
        : settings(s)
    {
        size = { 50, 65 };

        settings.Link(this);

        *this += [this](const MousePress& e)
        {
            m_PressVal = settings.inverse((settings.value - settings.range.start) / (settings.range.end - settings.range.start));
            m_PrevPos = e.pos.y;
        };

        *this += [this](const MouseClick& e)
        {
            if (e.button != MouseButton::Left)
                return;
            auto _now = std::chrono::steady_clock::now();
            auto _duration = std::chrono::duration_cast<std::chrono::milliseconds>(_now - m_ChangeTime).count();
            if (_duration < 500)
                settings.value = settings.reset;

            m_ChangeTime = _now;
        };

        *this += [this](const MouseDrag& e)
        {
            if (~e.buttons & MouseButton::Left)
                return;

            float _value = (m_PrevPos - e.pos.y) / height;
            m_PrevPos = e.pos.y;

            if (e.mod & Mods::Shift)
                _value *= settings.shift;

            m_PressVal += _value;

            if (settings.scaling)
                settings.value = settings.scaling(constrain(m_PressVal, 0.f, 1.f)) * (settings.range.end - settings.range.start) + settings.range.start;
        };

        m_ValueBox += [this](const Unfocus&)
        {
            auto content = m_ValueBox.content;
            std::regex reg{ "[^\\d\\.\\-]+" };
            std::string out = std::regex_replace(content, reg, "");
            try {
                double i = std::stod(out);
                settings.value = i;
                settings.value = constrain(settings.value, settings.range.start, settings.range.end);
            }
            catch (std::invalid_argument const& e) {
            }
            catch (std::out_of_range const& e) {
            }
        };
    }

    bool Hitbox(const Vec2<float>& p) const override { return Component::Hitbox(p) || m_ValueBox.Hitbox(p); }

    void Update() override
    {
        m_ValueBox.State(Visible) = settings.displayValue;
        m_ValueBox.dimensions = { x, y + height - m_ValueBox.height, width, 20 };
        m_ValueBox.textColor = settings.text.Current();
        m_ValueBox.align = Align::Center;
        m_ValueBox.overflow = Overflow::Show;
        if (!m_ValueBox.State(Focused))
        {
            m_ValueBox.content = Units::units[settings.unit].Format(settings.value, settings.decimals);
            m_ValueBox.displayer.RecalculateLines();
        }
    }

    void Render(CommandCollection& d) const override
    {
        int _p = 6;

        bool _double = settings.range.start < 0 && settings.range.end > 0 && settings.range.start == -settings.range.end;

        float _width = width * 0.6;
        float _height = width * 0.6;
        float _yoff = 4;

        float _pi = M_PI;
        float _v = 1.0 - Normalized();
        float _a = _v * _pi * 1.49 + _pi * 0.25 - _pi * 0.5;

        int _w = Normalized() * (_width * 0.5 - 1);

        int _h = height - _p * 2;
        int _we = _w - _p * 2;
        d.Fill(settings.border.Current());
        d.Ellipse({ x + width / 2, y + height / 2 + _yoff, _width, _height }, { _pi * 1.75f - _pi / 2, _pi * 0.25f - _pi / 2 });
        d.Fill(settings.line.Current());
        d.Ellipse({ x + width / 2, y + height / 2 + _yoff, _width, _height }, { _double ? _a > _pi / 2.f ? _a : _pi / 2.f : -_pi * 0.75f, _double ? _a > _pi / 2.f ? _pi / 2.f : _a : _a });
        d.Fill(settings.background.Current());
        d.Ellipse({ x + width / 2, y + height / 2 + _yoff, _width - 5, _height - 5 }, { _pi * 1.75f - _pi / 2.0f, _pi * 0.25f - _pi / 2.0f });

        float _x = std::cos(-_a) * (_width / 2.0);
        float _y = std::sin(-_a) * (_height / 2.0);
        d.Line({ x + width / 2.0f, y + height / 2.0f + _yoff, x + width / 2.0f + _x, y + height / 2.0f + _y + _yoff }, 6.0f);
        d.Fill(settings.handle.Current());
        d.Line({ x + width / 2.0f, y + height / 2.0f + _yoff, x + width / 2.0f + _x, y + height / 2.0f + _y + _yoff }, 3.0f);
        if (_double)
        {
            if (settings.value == 0)
                d.Fill(settings.border.Current());
            else
                d.Fill(settings.line.Current());

            d.Triangle({ x + width / 2, y + height / 2 - _height / 2 - 2 + _yoff, 7, 4 }, -90.0f);
        }

        if (settings.displayName)
        {
            d.Font(settings.font);
            d.FontSize(settings.fontSize);
            d.Fill(settings.text.Current());

            d.TextAlign(Align::CenterX | Align::TextTop);
            d.Text(settings.name, { x + width / 2, y });
        }
    }

    float Normalized() const
    {
        return (settings.value - settings.range.start) / (float)(settings.range.end - settings.range.start);
    }

    operator double& () { return settings.value; }

private:
    std::chrono::steady_clock::time_point m_ChangeTime;
    TextBox& m_ValueBox = emplace_back<TextBox>();
    float m_PressVal = 0;
    float m_PrevPos = 0;
    bool m_Linking = false;
};