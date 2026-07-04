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

static juce::StringArray const MODES {
    "Mix&Mod",
    "Split C3", "Split Db3", "Split D3", "Split Eb3", "Split E3", "Split F3",
    "Split Gb3", "Split G3", "Split Ab3", "Split A3", "Split Bb3", "Split B3",
    "Split C4"
};


NewGui::NewGui(Synth& synth)
    : bridge(synth),
    osc1_wave(nullptr),
    osc2_wave(nullptr),
    mode_selector(nullptr),
    osc1_filters(nullptr),
    osc2_filters(nullptr)
{
    setOpaque(true);

    /* Osc 1 (modulator). */
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
    osc1_filters = add_filters(
        { Synth::ParamId::MF1TYP, Synth::ParamId::MF1FRQ, Synth::ParamId::MF1Q, Synth::ParamId::MF1G },
        { Synth::ParamId::MF2TYP, Synth::ParamId::MF2FRQ, Synth::ParamId::MF2Q, Synth::ParamId::MF2G },
        "1", "2"
    );

    /* Mix column. */
    mode_selector = add_selector(Synth::ParamId::MODE, MODES, "MODE");
    add_knob(mix, Synth::ParamId::MIX, "MIX");
    add_knob(mix, Synth::ParamId::PM,  "PM");
    add_knob(mix, Synth::ParamId::FM,  "FM");
    add_knob(mix, Synth::ParamId::AM,  "AM");

    /* Osc 2 (carrier). Its two filters are the 3rd and 4th overall. */
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
    osc2_filters = add_filters(
        { Synth::ParamId::CF1TYP, Synth::ParamId::CF1FRQ, Synth::ParamId::CF1Q, Synth::ParamId::CF1G },
        { Synth::ParamId::CF2TYP, Synth::ParamId::CF2FRQ, Synth::ParamId::CF2Q, Synth::ParamId::CF2G },
        "3", "4"
    );

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


Selector* NewGui::add_selector(
        Synth::ParamId const id, juce::StringArray options, juce::String caption
) {
    Selector* const selector = new Selector(
        bridge, id, std::move(options), std::move(caption)
    );
    selectors.add(selector);
    addAndMakeVisible(selector);

    return selector;
}


FilterPanel* NewGui::add_filters(
        FilterPanel::Spec const& a,
        FilterPanel::Spec const& b,
        juce::String label_a,
        juce::String label_b
) {
    FilterPanel* const panel = new FilterPanel(
        bridge, a, b, std::move(label_a), std::move(label_b)
    );
    filters.add(panel);
    addAndMakeVisible(panel);

    return panel;
}


void NewGui::timerCallback()
{
    for (Knob* const knob : knobs) {
        knob->refresh();
    }

    for (WaveformSelector* const wave : waves) {
        wave->refresh();
    }

    for (Selector* const selector : selectors) {
        selector->refresh();
    }

    for (FilterPanel* const filter : filters) {
        filter->refresh();
    }
}


void NewGui::resized()
{
    juce::Rectangle<int> area = getLocalBounds();

    header_bounds = area.removeFromTop(46);
    area.reduce(10, 10);

    int const gap = 10;

    int const mod_width = juce::jmax(220, area.getWidth() / 3);
    mod_bounds = area.removeFromRight(mod_width);
    area.removeFromRight(gap);

    int const mix_width = 84;
    int const osc_width = (area.getWidth() - mix_width - 2 * gap) / 2;

    osc1_bounds = area.removeFromLeft(osc_width);
    area.removeFromLeft(gap);
    mix_bounds = area.removeFromLeft(mix_width);
    area.removeFromLeft(gap);
    osc2_bounds = area;

    lay_out_osc(osc1_bounds, osc1_wave, osc1, osc1_filters);
    lay_out_mix(mix_bounds, mode_selector, mix);
    lay_out_osc(osc2_bounds, osc2_wave, osc2, osc2_filters);
}


void NewGui::lay_out_osc(
        juce::Rectangle<int> panel,
        WaveformSelector* wave,
        std::vector<Knob*>& main,
        FilterPanel* filter
) {
    juce::Rectangle<int> inner = panel.reduced(12);
    inner.removeFromTop(20);   /* title */

    if (wave != nullptr) {
        wave->setBounds(inner.removeFromTop(40));
    }

    inner.removeFromTop(8);

    int const columns = 4;
    int const cell_w = inner.getWidth() / columns;
    int const cell_h = 72;

    for (int i = 0; i != (int)main.size(); ++i) {
        main[(size_t)i]->setBounds(
            inner.getX() + (i % columns) * cell_w,
            inner.getY() + (i / columns) * cell_h,
            cell_w,
            cell_h
        );
    }

    int const main_rows = ((int)main.size() + columns - 1) / columns;
    inner.removeFromTop(main_rows * cell_h + 8);

    if (filter != nullptr) {
        filter->setBounds(inner.removeFromTop(140));
    }
}


void NewGui::lay_out_mix(
        juce::Rectangle<int> panel,
        Selector* mode,
        std::vector<Knob*>& knobs_
) {
    juce::Rectangle<int> inner = panel.reduced(12);
    inner.removeFromTop(20);

    if (mode != nullptr) {
        mode->setBounds(inner.removeFromTop(40));
        inner.removeFromTop(8);
    }

    int const cell_h = 72;

    for (int i = 0; i != (int)knobs_.size(); ++i) {
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
        header_bounds.withTrimmedRight(120).reduced(16, 0),
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
