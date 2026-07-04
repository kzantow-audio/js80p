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

#include "ui/vslider.hpp"

#include "ui/theme.hpp"


namespace JS80P
{

VSlider::VSlider(ParamBridge& bridge, Synth::ParamId const param_id, juce::String name)
    : bridge(bridge),
    param_id(param_id),
    name(std::move(name)),
    ratio(bridge.get_ratio(param_id))
{
    setWantsKeyboardFocus(false);
}


void VSlider::refresh()
{
    double const live = bridge.get_ratio(param_id);

    if (std::fabs(live - ratio) > 1.0e-6) {
        ratio = live;
        repaint();
    }
}


juce::Rectangle<int> VSlider::bar() const
{
    return juce::Rectangle<int>(getWidth() - BAR_W, 1, BAR_W, getHeight() - 2);
}


void VSlider::set_from_mouse(int const y)
{
    juce::Rectangle<int> const b = bar();
    double const r = juce::jlimit(0.0, 1.0, 1.0 - (double)(y - b.getY()) / (double)b.getHeight());

    ratio = r;
    bridge.set_ratio(param_id, r);
    repaint();
}


void VSlider::paint(juce::Graphics& g)
{
    juce::Rectangle<int> const b = bar();
    juce::Rectangle<int> const text = getLocalBounds().withTrimmedRight(BAR_W + 3);

    g.setColour(Theme::TEXT_DIM);
    g.setFont(10.0f);
    g.drawText(name, text.withTrimmedBottom(text.getHeight() / 2),
               juce::Justification::bottomRight, false);

    double const v = bridge.display_value(param_id, ratio);
    juce::String const vs = juce::String(v, std::fabs(v) >= 100.0 ? 0 : 2);
    g.setColour(Theme::TEXT);
    g.setFont(10.0f);
    g.drawText(vs, text.withTrimmedTop(text.getHeight() / 2),
               juce::Justification::topRight, false);

    g.setColour(Theme::INSET);
    g.fillRoundedRectangle(b.toFloat(), 2.0f);

    float const fill = (float)ratio * (float)b.getHeight();
    g.setColour(Theme::ACCENT);
    g.fillRoundedRectangle(
        (float)b.getX(), (float)b.getBottom() - fill, (float)b.getWidth(), fill, 2.0f
    );
}


void VSlider::mouseDown(juce::MouseEvent const& event)
{
    set_from_mouse(event.y);
}


void VSlider::mouseDrag(juce::MouseEvent const& event)
{
    set_from_mouse(event.y);
}


void VSlider::mouseDoubleClick(juce::MouseEvent const& /* event */)
{
    ratio = bridge.get_default_ratio(param_id);
    bridge.set_ratio(param_id, ratio);
    repaint();
}

}
