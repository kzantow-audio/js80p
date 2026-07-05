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

#include "ui/macro_strip.hpp"

#include "ui/theme.hpp"


namespace JS80P
{

MacroStrip::MacroStrip(ParamBridge& bridge)
{
    setWantsKeyboardFocus(false);

    for (int m = 1; m <= COUNT; ++m) {
        MacroCell* const cell = new MacroCell(bridge, m);
        cells.add(cell);
        addAndMakeVisible(cell);
    }
}


void MacroStrip::refresh()
{
    for (MacroCell* const cell : cells) {
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
