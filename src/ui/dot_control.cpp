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

#include "ui/dot_control.hpp"

#include "ui/knob.hpp"
#include "ui/theme.hpp"


namespace JS80P
{

DotControl::DotControl(ParamBridge& bridge, Synth::ParamId const param_id)
    : bridge(bridge),
    param_id(param_id),
    ratio(bridge.get_ratio(param_id)),
    drag_start_ratio(0.0),
    dragging(false)
{
    setWantsKeyboardFocus(false);
}


void DotControl::set_mirrors(std::vector<Synth::ParamId> m)
{
    mirrors = std::move(m);
}


void DotControl::commit(double const new_ratio)
{
    ratio = juce::jlimit(0.0, 1.0, new_ratio);
    bridge.set_ratio(param_id, ratio);
    for (Synth::ParamId const m : mirrors) {
        bridge.set_ratio(m, ratio);
    }
    repaint();
}


void DotControl::refresh()
{
    if (dragging) {
        return;
    }

    double const live = bridge.get_ratio(param_id);
    if (std::fabs(live - ratio) > 1.0e-6) {
        ratio = live;
        repaint();
    }
}


void DotControl::paint(juce::Graphics& g)
{
    juce::Rectangle<float> const b = getLocalBounds().toFloat();
    float const d = juce::jmax(4.0f, juce::jmin(b.getWidth(), b.getHeight()) - 2.0f);
    juce::Rectangle<float> const circle = b.withSizeKeepingCentre(d, d);
    float const cx = circle.getCentreX();
    float const cy = circle.getCentreY();
    float const r = d * 0.5f;

    double const v = juce::jlimit(0.0, 1.0, ratio);

    /* Radial pie fill, sweeping clockwise from 6 o'clock (bottom); a solid disc
     * at 1. */
    if (v > 1.0e-3) {
        float const start = juce::MathConstants<float>::pi;
        juce::Path pie;
        pie.addPieSegment(
            cx - r, cy - r, d, d,
            start, start + juce::MathConstants<float>::twoPi * (float)v, 0.0f
        );
        g.setColour(Theme::ACCENT);
        g.fillPath(pie);
    }

    /* Outline: the whole control at value 0, a subtle rim otherwise. */
    g.setColour(v > 1.0e-3 ? Theme::EDGE : Theme::TEXT_DIM);
    g.drawEllipse(circle, 1.0f);
}


void DotControl::mouseDown(juce::MouseEvent const& /* event */)
{
    dragging = true;
    drag_start_ratio = ratio;
}


void DotControl::mouseDrag(juce::MouseEvent const& event)
{
    bool const fine = event.mods.isCtrlDown();
    double const sensitivity = fine
        ? Knob::DRAG_PIXELS_FULL_RANGE * 5.0 : Knob::DRAG_PIXELS_FULL_RANGE;
    double const delta = -(double)event.getDistanceFromDragStartY() / sensitivity;
    commit(drag_start_ratio + delta);
}


void DotControl::mouseUp(juce::MouseEvent const& /* event */)
{
    dragging = false;
}


void DotControl::mouseDoubleClick(juce::MouseEvent const& /* event */)
{
    commit(bridge.get_default_ratio(param_id));
}

}
