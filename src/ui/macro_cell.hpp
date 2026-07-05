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

#ifndef JS80P__UI__MACRO_CELL_HPP
#define JS80P__UI__MACRO_CELL_HPP

#include <juce_gui_basics/juce_gui_basics.h>

#include "js80p.hpp"
#include "synth.hpp"

#include "ui/param_bridge.hpp"


namespace JS80P
{

/**
 * \brief One performance-macro cell (macros 1-8). The rotary sets the macro's
 *        minimum output; a source badge at the top-right sets the maximum on
 *        drag (the CC modulation range) and, on click, opens a source menu
 *        (MIDI Learn, well-known CCs, mod/pitch wheel, velocity, note, ...).
 */
class MacroCell : public juce::Component
{
    public:
        MacroCell(ParamBridge& bridge, int const macro);

        void refresh();

        void paint(juce::Graphics& g) override;
        void mouseDown(juce::MouseEvent const& event) override;
        void mouseDrag(juce::MouseEvent const& event) override;
        void mouseUp(juce::MouseEvent const& event) override;
        void mouseDoubleClick(juce::MouseEvent const& event) override;

    private:
        static constexpr double DRAG_PIXELS_FULL_RANGE = 220.0;

        juce::Rectangle<float> knob_circle() const;
        juce::Rectangle<float> badge_rect() const;
        juce::String source_label() const;
        bool has_source() const;
        void write_base(double const b);
        void write_depth(double const d);
        void open_source_menu();

        ParamBridge& bridge;
        int const macro;
        Synth::ParamId const min_p;
        Synth::ParamId const max_p;
        Synth::ParamId const in_p;

        double base;    /* min */
        double depth;   /* max - min (signed) */

        bool dragging_depth;
        bool badge_press;
        int press_distance;
        double drag_start_base;
        double drag_start_depth;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MacroCell)
};

}

#endif
