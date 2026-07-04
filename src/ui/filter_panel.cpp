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

#include "ui/filter_panel.hpp"

#include "ui/theme.hpp"


namespace JS80P
{

FilterPanel::FilterPanel(
        ParamBridge& bridge,
        ModulatorAllocator& allocator,
        Spec const& a,
        Spec const& b,
        juce::String label_a,
        juce::String label_b
) : bridge(bridge),
    label_a(std::move(label_a)),
    label_b(std::move(label_b)),
    active(0)
{
    setWantsKeyboardFocus(false);

    Spec const specs[2] = { a, b };

    for (int f = 0; f != 2; ++f) {
        FilterTypeSelector* const type = new FilterTypeSelector(bridge, specs[f].type);
        types.add(type);
        addChildComponent(type);

        knobs.add(new Knob(bridge, specs[f].freq, "FREQ"));
        knobs.add(new Knob(bridge, specs[f].q,    "Q"));
        knobs.add(new Knob(bridge, specs[f].gain, "GAIN"));

        knobs[f * 3]->set_center_value(1000.0);   /* 1 kHz at mid-travel */

        for (int k = 0; k != 3; ++k) {
            knobs[f * 3 + k]->set_allocator(&allocator);
            addChildComponent(knobs[f * 3 + k]);
        }
    }

    set_active(0);
}


void FilterPanel::set_active(int const index)
{
    active = index;

    for (int f = 0; f != 2; ++f) {
        bool const on = (f == active);
        types[f]->setVisible(on);

        for (int k = 0; k != 3; ++k) {
            knobs[f * 3 + k]->setVisible(on);
        }
    }

    repaint();
}


void FilterPanel::refresh()
{
    for (FilterTypeSelector* const type : types) {
        type->refresh();
    }

    for (Knob* const knob : knobs) {
        knob->refresh();
    }
}


void FilterPanel::resized()
{
    juce::Rectangle<int> b = getLocalBounds().reduced(9);

    juce::Rectangle<int> const header = b.removeFromTop(20);
    int const seg_w = 26;
    int const seg_h = 15;
    seg_b_bounds = juce::Rectangle<int>(header.getRight() - seg_w, header.getY() + 1, seg_w, seg_h);
    seg_a_bounds = juce::Rectangle<int>(seg_b_bounds.getX() - seg_w - 3, header.getY() + 1, seg_w, seg_h);

    b.removeFromTop(4);
    /* Match one row of the oscillator waveform selector (40px / 2 rows). */
    juce::Rectangle<int> const type_row = b.removeFromTop(20);
    b.removeFromTop(6);

    /* FREQ stays large (left, full height); Q and GAIN match the oscillator
     * knob size (a quarter of the panel width, 72px tall). */
    int const small_w = b.getWidth() / 4;
    int const small_h = 72;
    int const freq_w = b.getWidth() - 2 * small_w;
    int const small_y = b.getY() + (b.getHeight() - small_h) / 2;

    for (int f = 0; f != 2; ++f) {
        types[f]->setBounds(type_row);
        knobs[f * 3 + 0]->setBounds(b.getX(), b.getY(), freq_w, b.getHeight());
        knobs[f * 3 + 1]->setBounds(b.getX() + freq_w, small_y, small_w, small_h);
        knobs[f * 3 + 2]->setBounds(b.getX() + freq_w + small_w, small_y, small_w, small_h);
    }
}


void FilterPanel::mouseDown(juce::MouseEvent const& event)
{
    if (seg_a_bounds.contains(event.getPosition())) {
        set_active(0);
    } else if (seg_b_bounds.contains(event.getPosition())) {
        set_active(1);
    }
}


void FilterPanel::paint(juce::Graphics& g)
{
    juce::Rectangle<float> const box = getLocalBounds().toFloat().reduced(0.5f);
    g.setColour(Theme::PANEL);
    g.fillRoundedRectangle(box, Theme::RADIUS);
    g.setColour(Theme::EDGE);
    g.drawRoundedRectangle(box, Theme::RADIUS, 1.0f);

    g.setColour(Theme::TEXT_DIM);
    g.setFont(juce::Font(juce::FontOptions().withHeight(13.0f).withStyle("Bold")));
    g.drawText("FILTERS", 10, 8, 100, 16, juce::Justification::centredLeft, false);

    juce::Rectangle<int> const segs[2] = { seg_a_bounds, seg_b_bounds };
    juce::String const labels[2] = { label_a, label_b };

    for (int i = 0; i != 2; ++i) {
        bool const on = (i == active);
        g.setColour(on ? Theme::ACCENT : Theme::INSET);
        g.fillRoundedRectangle(segs[i].toFloat(), 2.0f);
        g.setColour(on ? Theme::ACCENT : Theme::EDGE);
        g.drawRoundedRectangle(segs[i].toFloat().reduced(0.5f), 2.0f, 1.0f);

        g.setColour(on ? Theme::BG : Theme::TEXT_DIM);
        g.setFont(11.0f);
        g.drawText(labels[i], segs[i], juce::Justification::centred, false);
    }
}

}
