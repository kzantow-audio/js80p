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

#ifndef JS80P__UI__FILTER_TYPE_SELECTOR_HPP
#define JS80P__UI__FILTER_TYPE_SELECTOR_HPP

#include <juce_gui_basics/juce_gui_basics.h>

#include "js80p.hpp"
#include "synth.hpp"

#include "ui/param_bridge.hpp"


namespace JS80P
{

/**
 * \brief The 7 biquad filter types as a single row of vector-drawn icon
 *        buttons (frequency-response glyphs), like the waveform selector.
 *        Bound to a discrete filter-type parameter.
 *        Order matches SimpleBiquadFilter: LP HP BP Notch Bell LS HS.
 */
class FilterTypeSelector : public juce::Component
{
    public:
        static constexpr int COUNT = 7;

        /** \p columns lays the icons out in a grid (default: a single row). */
        FilterTypeSelector(
            ParamBridge& bridge,
            Synth::ParamId const param_id,
            int const columns = COUNT
        );

        void refresh();

        void paint(juce::Graphics& g) override;
        void mouseDown(juce::MouseEvent const& event) override;

    private:
        void draw_glyph(
            juce::Graphics& g, juce::Rectangle<float> area, int const type
        ) const;

        ParamBridge& bridge;
        Synth::ParamId const param_id;
        int const columns;
        int selected;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FilterTypeSelector)
};

}

#endif
