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
    : bridge(synth),
    osc1_wave(nullptr),
    osc2_wave(nullptr)
{
    setOpaque(true);

    /* Osc 1 (modulator): waveform, then amp / tuning / type rows. */
    osc1_wave = add_wave(Synth::ParamId::MWFM);
    add_knob(osc1, Synth::ParamId::MAMP, "AMP");
    add_knob(osc1, Synth::ParamId::MVS,  "VEL");
    add_knob(osc1, Synth::ParamId::MWID, "WID");
    add_knob(osc1, Synth::ParamId::MPAN, "PAN");
    add_knob(osc1, Synth::ParamId::MDTN, "DTN");
    add_knob(osc1, Synth::ParamId::MFIN, "FIN");
    add_knob(osc1, Synth::ParamId::MPRD, "PRD");
    add_knob(osc1, Synth::ParamId::MPRT, "PRT");
    add_knob(osc1, Synth::ParamId::MN,   "NOISE");
    add_knob(osc1, Synth::ParamId::MFLD, "FOLD");
    add_knob(osc1, Synth::ParamId::MSUB, "SUB");
    add_knob(osc1, Synth::ParamId::MF1FRQ, "F.FRQ");

    /* Mix column. */
    add_knob(mix, Synth::ParamId::MIX, "MIX");
    add_knob(mix, Synth::ParamId::PM,  "PM");
    add_knob(mix, Synth::ParamId::FM,  "FM");
    add_knob(mix, Synth::ParamId::AM,  "AM");

    /* Osc 2 (carrier). */
    osc2_wave = add_wave(Synth::ParamId::CWFM);
    add_knob(osc2, Synth::ParamId::CAMP, "AMP");
    add_knob(osc2, Synth::ParamId::CVS,  "VEL");
    add_knob(osc2, Synth::ParamId::CWID, "WID");
    add_knob(osc2, Synth::ParamId::CPAN, "PAN");
    add_knob(osc2, Synth::ParamId::CDTN, "DTN");
    add_knob(osc2, Synth::ParamId::CFIN, "FIN");
    add_knob(osc2, Synth::ParamId::CPRD, "PRD");
    add_knob(osc2, Synth::ParamId::CPRT, "PRT");
    add_knob(osc2, Synth::ParamId::CN,   "NOISE");
    add_knob(osc2, Synth::ParamId::CFLD, "FOLD");
    add_knob(osc2, Synth::ParamId::CDL,  "DIST");
    add_knob(osc2, Synth::ParamId::CF1FRQ, "F.FRQ");

    startTimerHz(30);
}


NewGui::~NewGui()
{
    stopTimer();
}


Knob& NewGui::add_knob(
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


WaveformSelector* NewGui::add_wave(Synth::ParamId const id)
{
    WaveformSelector* const wave = new WaveformSelector(bridge, id);
    waves.add(wave);
    addAndMakeVisible(wave);

    return wave;
}


void NewGui::timerCallback()
{
    for (Knob* const knob : knobs) {
        knob->refresh();
    }

    for (WaveformSelector* const wave : waves) {
        wave->refresh();
    }
}


void NewGui::resized()
{
    juce::Rectangle<int> area = getLocalBounds();

    header_bounds = area.removeFromTop(46);
    area.reduce(10, 10);

    int const gap = 10;

    /* Right third: Modulators panel (placeholder for now). */
    int const mod_width = juce::jmax(220, area.getWidth() / 3);
    mod_bounds = area.removeFromRight(mod_width);
    area.removeFromRight(gap);

    /* Left two thirds: Osc 1 -> Mix -> Osc 2. */
    int const mix_width = 82;
    int const osc_width = (area.getWidth() - mix_width - 2 * gap) / 2;

    osc1_bounds = area.removeFromLeft(osc_width);
    area.removeFromLeft(gap);
    mix_bounds = area.removeFromLeft(mix_width);
    area.removeFromLeft(gap);
    osc2_bounds = area;

    lay_out_osc(osc1_bounds, osc1_wave, osc1);
    lay_out_mix(mix_bounds, mix);
    lay_out_osc(osc2_bounds, osc2_wave, osc2);
}


void NewGui::lay_out_osc(
        juce::Rectangle<int> panel,
        WaveformSelector* wave,
        std::vector<Knob*>& knobs_
) {
    juce::Rectangle<int> inner = panel.reduced(12);
    inner.removeFromTop(20);   /* title */

    if (wave != nullptr) {
        wave->setBounds(inner.removeFromTop(40));
    }

    inner.removeFromTop(6);

    /* Compact fixed-size cells, top-packed, leaving room below for the filter
     * areas and per-type editor. */
    int const columns = 4;
    int const cell_w = inner.getWidth() / columns;
    int const cell_h = 74;

    for (int i = 0; i != (int)knobs_.size(); ++i) {
        int const row = i / columns;
        int const col = i % columns;

        knobs_[(size_t)i]->setBounds(
            inner.getX() + col * cell_w,
            inner.getY() + row * cell_h,
            cell_w,
            cell_h
        );
    }
}


void NewGui::lay_out_mix(
        juce::Rectangle<int> panel,
        std::vector<Knob*>& knobs_
) {
    juce::Rectangle<int> inner = panel.reduced(12);
    inner.removeFromTop(20);

    int const n = (int)knobs_.size();

    if (n < 1) {
        return;
    }

    int const cell_h = inner.getHeight() / n;

    for (int i = 0; i != n; ++i) {
        knobs_[(size_t)i]->setBounds(
            inner.getX(), inner.getY() + i * cell_h, inner.getWidth(), cell_h
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

    g.setColour(Theme::PANEL_2);
    g.fillRect(header_bounds);
    g.setColour(Theme::EDGE_SOFT);
    g.fillRect(header_bounds.getX(), header_bounds.getBottom() - 1, header_bounds.getWidth(), 1);

    g.setColour(Theme::TEXT);
    g.setFont(juce::Font(juce::FontOptions().withHeight(20.0f).withStyle("Bold")));
    g.drawText("JS80P", header_bounds.reduced(16, 0), juce::Justification::centredLeft, false);

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

    draw_panel(g, mod_bounds, "MODULATORS");
    g.setColour(Theme::TEXT_FAINT);
    g.setFont(12.0f);
    g.drawText(
        "envelope / LFO cards - coming soon",
        mod_bounds.reduced(14, 0),
        juce::Justification::centred,
        false
    );
}

}
