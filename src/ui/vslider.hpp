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

#ifndef JS80P__UI__VSLIDER_HPP
#define JS80P__UI__VSLIDER_HPP

#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "js80p.hpp"
#include "synth.hpp"

#include "ui/param_bridge.hpp"


namespace JS80P
{

/**
 * \brief A compact vertical slider bound to one or more mirrored parameters.
 *        Writes go to *every* target (so a grouped modulator's duplicate slots
 *        stay in sync); the display reads the first. The name and value sit
 *        top-left; an optional curvature picker (also multi-target, discrete)
 *        sits underneath, left of the bar, and cycles on vertical drag.
 */
class VSlider : public juce::Component
{
    public:
        VSlider(
            ParamBridge& bridge,
            std::vector<Synth::ParamId> value_targets,
            juce::String name,
            std::vector<Synth::ParamId> curve_targets = {}
        );

        void refresh();

        /** Clamp the editable minimum (e.g. attack floored to 0.001). */
        void set_min_ratio(double const r);

        /** Draw the curve glyph falling (decay/release) rather than rising. */
        void set_curve_falling(bool const falling);

        void paint(juce::Graphics& g) override;
        void mouseDown(juce::MouseEvent const& event) override;
        void mouseDrag(juce::MouseEvent const& event) override;
        void mouseDoubleClick(juce::MouseEvent const& event) override;
        void mouseWheelMove(
            juce::MouseEvent const& event, juce::MouseWheelDetails const& wheel
        ) override;

    private:
        static constexpr int BAR_W = 18;
        static constexpr double DRAG_PIXELS_FULL_RANGE = 200.0;

        juce::Rectangle<int> bar() const;
        juce::Rectangle<int> curve_rect() const;
        void write_ratio(double const r);
        void set_curve(int const index);

        ParamBridge& bridge;
        std::vector<Synth::ParamId> value_targets;
        std::vector<Synth::ParamId> curve_targets;
        juce::String const name;
        double ratio;
        double min_ratio;
        int curve_index;
        bool curve_falling;

        bool dragging_curve;
        double drag_start_ratio;
        int drag_start_curve;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VSlider)
};

}

#endif
