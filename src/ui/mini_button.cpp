/*
 * This file is part of JS80P, a synthesizer plugin.
 * Copyright (C) 2023, 2024, 2025, 2026  Attila M. Magyar
 *
 * JS80P is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * JS80P is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <utility>

#include "ui/mini_button.hpp"

#include "ui/theme.hpp"


namespace JS80P
{

MiniButton::MiniButton(
        ParamBridge& bridge,
        Synth::ParamId const param_id,
        juce::String label
) : bridge(bridge),
    param_id(param_id),
    label(std::move(label)),
    value(bridge.get_discrete(param_id)),
    hover(false)
{
    setWantsKeyboardFocus(false);
}


void MiniButton::set_option_labels(juce::StringArray labels)
{
    option_labels = std::move(labels);
}


int MiniButton::preferred_width() const
{
    juce::Font const f(juce::FontOptions().withHeight(9.0f));
    float w = juce::GlyphArrangement::getStringWidth(f, label);

    for (juce::String const& s : option_labels) {
        w = juce::jmax(w, juce::GlyphArrangement::getStringWidth(f, s));
    }

    return juce::jmax(28, (int)w + 12);
}


void MiniButton::refresh()
{
    int const live = bridge.get_discrete(param_id);

    if (live != value) {
        value = live;
        repaint();
    }
}


void MiniButton::paint(juce::Graphics& g)
{
    bool const on = value != 0;
    juce::Colour const c = Theme::ACCENT;
    juce::Rectangle<float> const r = getLocalBounds().toFloat().reduced(0.5f);
    float const radius = 2.0f;

    if (on) {
        g.setColour(hover ? c.brighter(0.15f) : c);
        g.fillRoundedRectangle(r, radius);
    } else if (hover) {
        g.setColour(c.withAlpha(0.18f));
        g.fillRoundedRectangle(r, radius);
    }

    g.setColour(on || !hover ? c : c.brighter(0.2f));
    g.drawRoundedRectangle(r, radius, 1.0f);

    juce::String text = label;
    if (!option_labels.isEmpty() && value >= 0 && value < option_labels.size()) {
        text = option_labels[value];
    }

    /* Solid fill needs dark text for contrast; the outline state keeps it orange. */
    g.setColour(on ? Theme::BG : c);
    g.setFont(9.0f);
    g.drawText(text, getLocalBounds(), juce::Justification::centred, false);
}


void MiniButton::mouseUp(juce::MouseEvent const& event)
{
    if (!getLocalBounds().contains(event.getPosition())) {
        return;
    }

    int const count = juce::jmax(2, bridge.option_count(param_id));
    value = (bridge.get_discrete(param_id) + 1) % count;
    bridge.set_discrete(param_id, value);
    repaint();
}


void MiniButton::mouseEnter(juce::MouseEvent const& /* event */)
{
    hover = true;
    repaint();
}


void MiniButton::mouseExit(juce::MouseEvent const& /* event */)
{
    hover = false;
    repaint();
}

}
