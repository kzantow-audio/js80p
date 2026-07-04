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

#include <juce_gui_basics/juce_gui_basics.h>

#include "js80p.hpp"
#include "synth.hpp"

#include "ui/param_bridge.hpp"


namespace JS80P
{

/**
 * \brief A compact vertical slider bound to one parameter, with the name and
 *        value drawn to the left of the bar so the bar spans (almost) the full
 *        component height. Used for the modulator cards' A/H/D/R etc.
 */
class VSlider : public juce::Component
{
    public:
        VSlider(ParamBridge& bridge, Synth::ParamId const param_id, juce::String name);

        void refresh();

        void paint(juce::Graphics& g) override;
        void mouseDown(juce::MouseEvent const& event) override;
        void mouseDrag(juce::MouseEvent const& event) override;
        void mouseDoubleClick(juce::MouseEvent const& event) override;

    private:
        static constexpr int BAR_W = 9;

        juce::Rectangle<int> bar() const;
        void set_from_mouse(int const y);

        ParamBridge& bridge;
        Synth::ParamId const param_id;
        juce::String const name;
        double ratio;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VSlider)
};

}

#endif
