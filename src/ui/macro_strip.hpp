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

#ifndef JS80P__UI__MACRO_STRIP_HPP
#define JS80P__UI__MACRO_STRIP_HPP

#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "js80p.hpp"
#include "synth.hpp"

#include "ui/knob.hpp"
#include "ui/param_bridge.hpp"


namespace JS80P
{

/**
 * \brief The top performance strip: macros 1-8 as always-visible knobs, each
 *        with a MIDI-CC input selector (assigns the macro's M{n}IN param to a
 *        CC controller). No other modulation options here. See doc/z-gui.md 5.4.
 */
class MacroStrip : public juce::Component
{
    public:
        static constexpr int COUNT = 8;

        explicit MacroStrip(ParamBridge& bridge);

        void refresh();

        void paint(juce::Graphics& g) override;
        void resized() override;
        void mouseDown(juce::MouseEvent const& event) override;

    private:
        juce::Rectangle<int> cc_area(int const macro) const;
        juce::String cc_label(int const macro) const;
        void open_cc_menu(int const macro);

        ParamBridge& bridge;
        juce::OwnedArray<Knob> knobs;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MacroStrip)
};

}

#endif
