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

#ifndef JS80P__UI__DOT_CONTROL_HPP
#define JS80P__UI__DOT_CONTROL_HPP

#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "js80p.hpp"
#include "synth.hpp"

#include "ui/param_bridge.hpp"


namespace JS80P
{

/**
 * \brief A tiny "pie-fill" dot bound to one Synth parameter, sized to sit across
 *        from a section title (~12-13px). It draws an unfilled circle outline at
 *        value 0 and fills with a radial pie sweeping clockwise from the top as
 *        the value rises, becoming a solid disc at 1. Vertical drag changes the
 *        value (same sensitivity as Knob; Ctrl = fine); double-click resets to
 *        the parameter default. Optional mirrors receive the same value on every
 *        change (grouped copies, like Knob::set_mirrors).
 */
class DotControl : public juce::Component, public juce::SettableTooltipClient
{
    public:
        DotControl(ParamBridge& bridge, Synth::ParamId const param_id);

        /** Other parameters (grouped copies) that receive the same value. */
        void set_mirrors(std::vector<Synth::ParamId> params);

        void refresh();

        void paint(juce::Graphics& g) override;
        void mouseDown(juce::MouseEvent const& event) override;
        void mouseDrag(juce::MouseEvent const& event) override;
        void mouseUp(juce::MouseEvent const& event) override;
        void mouseDoubleClick(juce::MouseEvent const& event) override;

    private:
        void commit(double const new_ratio);

        ParamBridge& bridge;
        Synth::ParamId const param_id;
        std::vector<Synth::ParamId> mirrors;

        double ratio;
        double drag_start_ratio;
        bool dragging;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DotControl)
};

}

#endif
