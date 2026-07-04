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

#include "ui/macro_strip.hpp"

#include "ui/modulation.hpp"
#include "ui/theme.hpp"


namespace JS80P
{

/* Curated MIDI CCs (label, CC number == ControllerId for a raw CC). */
static struct { char const* name; int cc; } const CCS[] = {
    { "-",       -1 },
    { "Mod 1",    1 },
    { "Breath 2", 2 },
    { "Foot 4",   4 },
    { "Vol 7",    7 },
    { "Pan 10",  10 },
    { "Expr 11", 11 },
    { "Reso 71", 71 },
    { "Cut 74",  74 },
    { "Rev 91",  91 },
    { "Cho 93",  93 },
};
static constexpr int CC_COUNT = (int)(sizeof(CCS) / sizeof(CCS[0]));


MacroStrip::MacroStrip(ParamBridge& bridge)
    : bridge(bridge)
{
    setWantsKeyboardFocus(false);

    for (int m = 1; m <= COUNT; ++m) {
        Knob* const knob = new Knob(bridge, Modulation::macro_in(m), juce::String("M") + juce::String(m));
        knobs.add(knob);
        addAndMakeVisible(knob);
    }
}


void MacroStrip::refresh()
{
    for (Knob* const knob : knobs) {
        knob->refresh();
    }
}


juce::Rectangle<int> MacroStrip::cc_area(int const macro) const
{
    int const cell = getWidth() / COUNT;
    return juce::Rectangle<int>(macro * cell + 6, getHeight() - 16, cell - 12, 14);
}


juce::String MacroStrip::cc_label(int const macro) const
{
    int const cc = (int)bridge.controller(Modulation::macro_in(macro + 1));

    if (cc == (int)Synth::ControllerId::NONE) {
        return "-";
    }

    for (int i = 0; i != CC_COUNT; ++i) {
        if (CCS[i].cc == cc) {
            return CCS[i].name;
        }
    }

    return juce::String("CC") + juce::String(cc);
}


void MacroStrip::resized()
{
    int const cell = getWidth() / COUNT;

    for (int i = 0; i != knobs.size(); ++i) {
        knobs[i]->setBounds(i * cell, 2, cell, getHeight() - 18);
    }
}


void MacroStrip::paint(juce::Graphics& g)
{
    g.setColour(Theme::PANEL);
    g.fillRect(getLocalBounds());
    g.setColour(Theme::EDGE_SOFT);
    g.fillRect(0, getHeight() - 1, getWidth(), 1);

    for (int m = 0; m != COUNT; ++m) {
        juce::Rectangle<int> const box = cc_area(m);
        g.setColour(Theme::INSET);
        g.fillRoundedRectangle(box.toFloat(), 2.0f);
        g.setColour(Theme::MIDI);
        g.setFont(9.5f);
        g.drawText(cc_label(m), box.reduced(4, 0), juce::Justification::centredLeft, false);
    }
}


void MacroStrip::open_cc_menu(int const macro)
{
    juce::PopupMenu menu;
    int const current = (int)bridge.controller(Modulation::macro_in(macro + 1));

    for (int i = 0; i != CC_COUNT; ++i) {
        menu.addItem(i + 1, CCS[i].name, true, CCS[i].cc == current);
    }

    MacroStrip* const self = this;
    menu.showMenuAsync(
        juce::PopupMenu::Options().withTargetComponent(this),
        [self, macro](int const result) {
            if (result <= 0) {
                return;
            }

            int const cc = CCS[result - 1].cc;
            self->bridge.assign_controller(
                Modulation::macro_in(macro + 1),
                cc < 0 ? Synth::ControllerId::NONE : (Synth::ControllerId)cc
            );
            self->repaint();
        }
    );
}


void MacroStrip::mouseDown(juce::MouseEvent const& event)
{
    for (int m = 0; m != COUNT; ++m) {
        if (cc_area(m).contains(event.getPosition())) {
            open_cc_menu(m);
            return;
        }
    }
}

}
