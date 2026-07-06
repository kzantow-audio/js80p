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

#ifndef JS80P__UI__MIX_KNOB_HPP
#define JS80P__UI__MIX_KNOB_HPP

#include <juce_gui_basics/juce_gui_basics.h>

#include "synth.hpp"

#include "ui/param_bridge.hpp"


namespace JS80P
{

/**
 * \brief A single rotary that folds an effect's WET and DRY volumes into one
 *        "MIX" control. At mid-travel both are 100%; turning clockwise holds WET
 *        at 100% while fading DRY to 0, and counter-clockwise holds DRY at 100%
 *        while fading WET to 0. Styled to match the plain (unassigned) Knob.
 */
class MixKnob : public juce::Component
{
    public:
        MixKnob(
            ParamBridge& bridge,
            Synth::ParamId const wet_id,
            Synth::ParamId const dry_id,
            juce::String label
        );

        void refresh();

        void paint(juce::Graphics& g) override;
        bool hitTest(int x, int y) override;
        void mouseDown(juce::MouseEvent const& event) override;
        void mouseDrag(juce::MouseEvent const& event) override;
        void mouseDoubleClick(juce::MouseEvent const& event) override;
        void mouseWheelMove(
            juce::MouseEvent const& event,
            juce::MouseWheelDetails const& wheel
        ) override;

    private:
        juce::Rectangle<float> knob_circle() const;
        double read_mix() const;
        void write_mix(double const m);

        ParamBridge& bridge;
        Synth::ParamId const wet_id;
        Synth::ParamId const dry_id;
        juce::String const label;
        double mix;   /* 0 = dry only, 0.5 = both full, 1 = wet only */
        double drag_start;
        bool dragging;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixKnob)
};

}

#endif
