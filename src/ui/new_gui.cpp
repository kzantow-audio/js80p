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

#include "ui/new_gui.hpp"

#include "ui/theme.hpp"


namespace JS80P
{

NewGui::NewGui(Synth& synth)
    : bridge(synth)
{
    setOpaque(true);

    add(osc1, Synth::ParamId::MAMP, "AMP");
    add(osc1, Synth::ParamId::MVS,  "VEL");
    add(osc1, Synth::ParamId::MWID, "WID");
    add(osc1, Synth::ParamId::MPAN, "PAN");
    add(osc1, Synth::ParamId::MFLD, "FOLD");
    add(osc1, Synth::ParamId::MDTN, "DTN");
    add(osc1, Synth::ParamId::MF1FRQ, "F.FRQ");
    add(osc1, Synth::ParamId::MF1Q,   "F.Q");

    add(mix, Synth::ParamId::MIX, "MIX");
    add(mix, Synth::ParamId::PM,  "PM");
    add(mix, Synth::ParamId::FM,  "FM");
    add(mix, Synth::ParamId::AM,  "AM");

    add(osc2, Synth::ParamId::CAMP, "AMP");
    add(osc2, Synth::ParamId::CVS,  "VEL");
    add(osc2, Synth::ParamId::CWID, "WID");
    add(osc2, Synth::ParamId::CPAN, "PAN");
    add(osc2, Synth::ParamId::CFLD, "FOLD");
    add(osc2, Synth::ParamId::CDTN, "DTN");
    add(osc2, Synth::ParamId::CF1FRQ, "F.FRQ");
    add(osc2, Synth::ParamId::CF1Q,   "F.Q");

    startTimerHz(30);
}


NewGui::~NewGui()
{
    stopTimer();
}


Knob& NewGui::add(
        std::vector<Knob*>& column,
        Synth::ParamId const id,
        char const* const label
) {
    Knob* const knob = new Knob(bridge, id, label);
    knobs.add(knob);
    column.push_back(knob);
    addAndMakeVisible(knob);

    return *knob;
}


void NewGui::timerCallback()
{
    for (Knob* const knob : knobs) {
        knob->refresh();
    }
}


void NewGui::resized()
{
    juce::Rectangle<int> area = getLocalBounds();

    header_bounds = area.removeFromTop(46);
    area.reduce(10, 10);

    int const gap = 10;
    int const mix_width = 92;
    int const osc_width = (area.getWidth() - mix_width - 2 * gap) / 2;

    osc1_bounds = area.removeFromLeft(osc_width);
    area.removeFromLeft(gap);
    mix_bounds = area.removeFromLeft(mix_width);
    area.removeFromLeft(gap);
    osc2_bounds = area;

    lay_out(osc1_bounds, osc1, 4);
    lay_out(mix_bounds, mix, 1);
    lay_out(osc2_bounds, osc2, 4);
}


void NewGui::lay_out(
        juce::Rectangle<int> panel,
        std::vector<Knob*>& column,
        int const columns
) {
    juce::Rectangle<int> inner = panel.reduced(12);
    inner.removeFromTop(20);   /* leave room for the panel title */

    int const rows = ((int)column.size() + columns - 1) / columns;

    if (rows < 1 || columns < 1) {
        return;
    }

    int const cell_w = inner.getWidth() / columns;
    int const cell_h = inner.getHeight() / rows;

    for (int i = 0; i != (int)column.size(); ++i) {
        int const row = i / columns;
        int const col = i % columns;

        column[(size_t)i]->setBounds(
            inner.getX() + col * cell_w,
            inner.getY() + row * cell_h,
            cell_w,
            cell_h
        );
    }
}


void NewGui::draw_panel(
        juce::Graphics& g,
        juce::Rectangle<int> const& r,
        char const* const title
) const {
    g.setColour(Theme::PANEL);
    g.fillRoundedRectangle(r.toFloat(), Theme::RADIUS);
    g.setColour(Theme::EDGE);
    g.drawRoundedRectangle(r.toFloat().reduced(0.5f), Theme::RADIUS, 1.0f);

    g.setColour(Theme::TEXT_DIM);
    g.setFont(juce::Font(juce::FontOptions().withHeight(13.0f).withStyle("Bold")));
    g.drawText(
        title,
        r.reduced(14, 10).removeFromTop(18),
        juce::Justification::centredLeft,
        false
    );
}


void NewGui::paint(juce::Graphics& g)
{
    g.fillAll(Theme::BG);

    /* Header. */
    g.setColour(Theme::PANEL_2);
    g.fillRect(header_bounds);
    g.setColour(Theme::EDGE_SOFT);
    g.fillRect(header_bounds.getX(), header_bounds.getBottom() - 1, header_bounds.getWidth(), 1);

    g.setColour(Theme::TEXT);
    g.setFont(juce::Font(juce::FontOptions().withHeight(20.0f).withStyle("Bold")));
    g.drawText(
        "JS80P",
        header_bounds.reduced(16, 0),
        juce::Justification::centredLeft,
        false
    );
    g.setColour(Theme::TEXT_FAINT);
    g.setFont(12.0f);
    g.drawText(
        "new GUI - work in progress",
        header_bounds.reduced(16, 0),
        juce::Justification::centredRight,
        false
    );

    draw_panel(g, osc1_bounds, "OSC 1  (modulator)");
    draw_panel(g, mix_bounds, "MIX");
    draw_panel(g, osc2_bounds, "OSC 2  (carrier)");
}

}
