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

#ifndef JS80P__UI__PER_TYPE_EDITOR_HPP
#define JS80P__UI__PER_TYPE_EDITOR_HPP

#include <juce_gui_basics/juce_gui_basics.h>

#include "js80p.hpp"
#include "synth.hpp"

#include "ui/knob.hpp"
#include "ui/param_bridge.hpp"


namespace JS80P
{

/**
 * \brief The waveform-dependent editor slot beneath an oscillator: a Pulse
 *        Width knob for pulse-family shapes, a 10-bar custom-harmonics editor
 *        for the Custom shape, and nothing otherwise. Follows the oscillator's
 *        waveform selection.
 */
class PerTypeEditor : public juce::Component
{
    public:
        PerTypeEditor(
            ParamBridge& bridge,
            Synth::ParamId const waveform,
            Synth::ParamId const pulse_width,
            Synth::ParamId const first_harmonic
        );

        void refresh();

        void paint(juce::Graphics& g) override;
        void resized() override;
        void mouseDown(juce::MouseEvent const& event) override;
        void mouseDrag(juce::MouseEvent const& event) override;

    private:
        static constexpr int HARMONICS = 10;

        enum Mode { NONE, PULSE, CUSTOM };

        Mode mode_for(int const waveform) const;
        juce::Rectangle<int> harmonics_area() const;
        void set_harmonic_from_mouse(juce::Point<int> const pos);

        ParamBridge& bridge;
        Synth::ParamId const waveform;
        Synth::ParamId const pulse_width;
        Synth::ParamId const first_harmonic;

        Knob pw_knob;
        Mode mode;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PerTypeEditor)
};

}

#endif
