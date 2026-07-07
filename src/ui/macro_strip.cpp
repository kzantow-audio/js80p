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

#include <memory>
#include <vector>

#include "ui/macro_strip.hpp"

#include "ui/curve_selector.hpp"
#include "ui/modulation.hpp"
#include "ui/theme.hpp"


namespace JS80P
{

MacroStrip::MacroStrip(ParamBridge& bridge)
{
    setWantsKeyboardFocus(false);

    for (int m = 1; m <= COUNT; ++m) {
        /* The macro rotary edits its MIN (base) / MAX (depth) directly; the
         * badge picks the input source and only then does it read as modulated.
         * A distortion-curve square sits at the dial's bottom-right. */
        Control* const cell = new Control(
            bridge, Modulation::macro_min(m), "M" + juce::String(m),
            Control::Style::ROTARY, Control::Size::SMALL
        );
        cell->set_macro(m);
        cell->set_label_placement(Control::LabelPos::LEFT);
        cell->set_value_display(Control::ValueDisplay::POPOVER);
        auto curve = std::make_unique<CurveSelector>(
            bridge, std::vector<Synth::ParamId>{ Modulation::macro_dcv(m) }, false, true
        );
        CurveSelector* const curve_ptr = curve.get();
        cell->set_sub_control(std::move(curve), [curve_ptr]() { curve_ptr->refresh(); });
        cells.add(cell);
        addAndMakeVisible(cell);
    }
}


void MacroStrip::refresh()
{
    for (Control* const cell : cells) {
        cell->refresh();
    }
}


void MacroStrip::resized()
{
    int const cell = getWidth() / COUNT;

    for (int i = 0; i != cells.size(); ++i) {
        cells[i]->setBounds(i * cell, 2, cell, getHeight() - 4);
    }
}


void MacroStrip::paint(juce::Graphics& g)
{
    g.setColour(Theme::PANEL);
    g.fillRect(getLocalBounds());
    g.setColour(Theme::EDGE_SOFT);
    g.fillRect(0, getHeight() - 1, getWidth(), 1);
}

}
