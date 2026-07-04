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

#ifndef JS80P__UI__NEW_GUI_HPP
#define JS80P__UI__NEW_GUI_HPP

#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "js80p.hpp"
#include "synth.hpp"

#include "ui/knob.hpp"
#include "ui/param_bridge.hpp"
#include "ui/selector.hpp"
#include "ui/waveform_selector.hpp"


namespace JS80P
{

/**
 * \brief The new simplified editor surface (work in progress). Opaque top-level
 *        component: header + left two-thirds (Osc 1 / Mix / Osc 2) + right-third
 *        Modulators panel. Each oscillator has a waveform selector over amp /
 *        tuning / type knob rows and a filter area; the Mix column has the mode
 *        selector over MIX/PM/FM/AM. All controls bound to the live Synth; the
 *        original GUI stays available behind an editor-owned toggle.
 */
class NewGui : public juce::Component, private juce::Timer
{
    public:
        explicit NewGui(Synth& synth);
        ~NewGui() override;

        void paint(juce::Graphics& g) override;
        void resized() override;

    private:
        void timerCallback() override;

        Knob& add_knob(std::vector<Knob*>& column, Synth::ParamId const id, char const* const label);
        WaveformSelector* add_wave(Synth::ParamId const id);
        Selector* add_selector(Synth::ParamId const id, juce::StringArray options, juce::String caption);

        void lay_out_osc(
            juce::Rectangle<int> panel,
            WaveformSelector* wave,
            std::vector<Knob*>& main,
            Selector* filter,
            std::vector<Knob*>& filter_knobs
        );
        void lay_out_mix(juce::Rectangle<int> panel, Selector* mode, std::vector<Knob*>& knobs);
        void draw_panel(juce::Graphics& g, juce::Rectangle<int> const& r, char const* const title) const;

        ParamBridge bridge;
        juce::OwnedArray<Knob> knobs;
        juce::OwnedArray<WaveformSelector> waves;
        juce::OwnedArray<Selector> selectors;

        WaveformSelector* osc1_wave;
        WaveformSelector* osc2_wave;
        Selector* osc1_filter_type;
        Selector* osc2_filter_type;
        Selector* mode_selector;

        std::vector<Knob*> osc1;
        std::vector<Knob*> osc2;
        std::vector<Knob*> osc1_filter;
        std::vector<Knob*> osc2_filter;
        std::vector<Knob*> mix;

        juce::Rectangle<int> header_bounds;
        juce::Rectangle<int> osc1_bounds;
        juce::Rectangle<int> mix_bounds;
        juce::Rectangle<int> osc2_bounds;
        juce::Rectangle<int> mod_bounds;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NewGui)
};

}

#endif
