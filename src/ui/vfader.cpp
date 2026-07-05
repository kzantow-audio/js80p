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

#include <cmath>
#include <utility>

#include "ui/vfader.hpp"

#include "ui/theme.hpp"


namespace JS80P
{

VFader::VFader(juce::String name) : name(std::move(name)), value(0.0)
{
    setWantsKeyboardFocus(false);
}


void VFader::set_value(double const v)
{
    double const c = juce::jlimit(0.0, 1.0, v);

    if (std::abs(c - value) > 1.0e-6) {
        value = c;
        repaint();
    }
}


juce::Rectangle<int> VFader::bar() const
{
    juce::Rectangle<int> b = getLocalBounds().reduced(2);
    b.removeFromTop(14);    /* name */
    b.removeFromBottom(14); /* value */
    int const w = 8;
    return juce::Rectangle<int>(b.getCentreX() - w / 2, b.getY(), w, b.getHeight());
}


void VFader::set_from_mouse(int const y)
{
    juce::Rectangle<int> const b = bar();
    double const v = juce::jlimit(0.0, 1.0, 1.0 - (double)(y - b.getY()) / (double)b.getHeight());
    value = v;

    if (on_change) {
        on_change(v);
    }

    repaint();
}


void VFader::paint(juce::Graphics& g)
{
    g.setColour(Theme::TEXT_DIM);
    g.setFont(10.0f);
    g.drawText(name, getLocalBounds().removeFromTop(14), juce::Justification::centred, false);

    juce::Rectangle<int> const b = bar();
    g.setColour(Theme::INSET);
    g.fillRoundedRectangle(b.toFloat(), 2.0f);

    float const fill = (float)value * (float)b.getHeight();
    g.setColour(Theme::ENV);
    g.fillRoundedRectangle((float)b.getX(), (float)b.getBottom() - fill,
                           (float)b.getWidth(), fill, 2.0f);

    g.setColour(Theme::TEXT);
    g.setFont(10.0f);
    g.drawText(juce::String(value, 2), getLocalBounds().removeFromBottom(14),
               juce::Justification::centred, false);
}


void VFader::mouseDown(juce::MouseEvent const& event)
{
    set_from_mouse(event.y);
}


void VFader::mouseDrag(juce::MouseEvent const& event)
{
    set_from_mouse(event.y);
}

}
