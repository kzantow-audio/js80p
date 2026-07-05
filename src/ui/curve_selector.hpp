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

#ifndef JS80P__UI__CURVE_SELECTOR_HPP
#define JS80P__UI__CURVE_SELECTOR_HPP

#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "js80p.hpp"
#include "synth.hpp"

#include "ui/param_bridge.hpp"


namespace JS80P
{

/**
 * \brief A tiny square that draws the current hardcoded envelope shape and
 *        cycles through the shapes on vertical drag or mouse wheel. Writes to
 *        every mirrored target (one per grouped envelope member).
 */
class CurveSelector : public juce::Component
{
    public:
        CurveSelector(
            ParamBridge& bridge, std::vector<Synth::ParamId> targets, bool const falling
        );

        void refresh();

        void paint(juce::Graphics& g) override;
        void mouseDown(juce::MouseEvent const& event) override;
        void mouseDrag(juce::MouseEvent const& event) override;
        void mouseWheelMove(
            juce::MouseEvent const& event, juce::MouseWheelDetails const& wheel
        ) override;
        void mouseDoubleClick(juce::MouseEvent const& event) override;

    private:
        void set_index(int const index);

        ParamBridge& bridge;
        std::vector<Synth::ParamId> targets;
        bool const falling;
        int index;
        int drag_start;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CurveSelector)
};

}

#endif
