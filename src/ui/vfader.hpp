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

#ifndef JS80P__UI__VFADER_HPP
#define JS80P__UI__VFADER_HPP

#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "js80p.hpp"


namespace JS80P
{

/**
 * \brief A minimal vertical fader over a plain [0, 1] value (not a Synth param):
 *        used for the envelope sustain fraction, which maps to a different
 *        absolute sustain per grouped destination. Reports drags via on_change.
 */
class VFader : public juce::Component
{
    public:
        explicit VFader(juce::String name);

        void set_value(double const v);

        std::function<void(double)> on_change;

        void paint(juce::Graphics& g) override;
        void mouseDown(juce::MouseEvent const& event) override;
        void mouseDrag(juce::MouseEvent const& event) override;

    private:
        juce::Rectangle<int> bar() const;
        void set_from_mouse(int const y);

        juce::String const name;
        double value;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VFader)
};

}

#endif
