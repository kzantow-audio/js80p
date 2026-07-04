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


namespace JS80P
{

/**
 * \brief The new simplified editor surface (work in progress). Opaque top-level
 *        component: a header strip + `Osc 1 -> Mix -> Osc 2` columns of
 *        vector knobs bound to the live Synth. Fills the editor; the original
 *        GUI stays available behind an editor-owned toggle.
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

        Knob& add(std::vector<Knob*>& column, Synth::ParamId const id, char const* const label);
        void lay_out(juce::Rectangle<int> panel, std::vector<Knob*>& column, int const columns);
        void draw_panel(juce::Graphics& g, juce::Rectangle<int> const& r, char const* const title) const;

        ParamBridge bridge;
        juce::OwnedArray<Knob> knobs;

        std::vector<Knob*> osc1;
        std::vector<Knob*> mix;
        std::vector<Knob*> osc2;

        juce::Rectangle<int> header_bounds;
        juce::Rectangle<int> osc1_bounds;
        juce::Rectangle<int> mix_bounds;
        juce::Rectangle<int> osc2_bounds;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NewGui)
};

}

#endif
