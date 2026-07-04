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

#include "ui/knob.hpp"

#include "ui/theme.hpp"


namespace JS80P
{

Knob::Knob(
        ParamBridge& bridge,
        Synth::ParamId const param_id,
        juce::String const& label
) : bridge(bridge),
    param_id(param_id),
    label(label),
    ratio(bridge.get_ratio(param_id)),
    drag_start_ratio(0.0),
    dragging(false)
{
    setWantsKeyboardFocus(false);
}


void Knob::refresh()
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


juce::String Knob::format_value() const
{
    double const value = bridge.display_value(param_id, ratio);
    int const decimals = (
        bridge.is_discrete(param_id)
            ? 0
            : (std::fabs(value) >= 100.0 ? 0 : (std::fabs(value) >= 10.0 ? 1 : 2))
    );

    return juce::String(value, decimals);
}


void Knob::paint(juce::Graphics& g)
{
    juce::Rectangle<int> b = getLocalBounds().reduced(2);
    juce::Rectangle<int> const label_area = b.removeFromTop(14);
    juce::Rectangle<int> const value_area = b.removeFromBottom(14);

    g.setColour(Theme::TEXT_DIM);
    g.setFont(11.0f);
    g.drawText(label, label_area, juce::Justification::centred, false);

    float const size = (float)juce::jmin(b.getWidth(), b.getHeight());
    juce::Rectangle<float> const kb =
        b.toFloat().withSizeKeepingCentre(size, size).reduced(3.0f);

    float const cx = kb.getCentreX();
    float const cy = kb.getCentreY();
    float const r = kb.getWidth() * 0.5f;

    float const start_angle = juce::MathConstants<float>::pi * 1.2f;
    float const end_angle = juce::MathConstants<float>::pi * 2.8f;
    float const value_angle = start_angle + (float)ratio * (end_angle - start_angle);

    juce::PathStrokeType const stroke(
        3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded
    );

    juce::Path track;
    track.addCentredArc(cx, cy, r, r, 0.0f, start_angle, end_angle, true);
    g.setColour(Theme::TRACK);
    g.strokePath(track, stroke);

    juce::Colour const active = (
        bridge.controller(param_id) == Synth::ControllerId::NONE
            ? Theme::ACCENT
            : Theme::ENV
    );

    juce::Path value_arc;
    value_arc.addCentredArc(cx, cy, r, r, 0.0f, start_angle, value_angle, true);
    g.setColour(active);
    g.strokePath(value_arc, stroke);

    float const body = r - 4.0f;
    g.setColour(Theme::PANEL_2);
    g.fillEllipse(cx - body, cy - body, body * 2.0f, body * 2.0f);
    g.setColour(Theme::EDGE);
    g.drawEllipse(cx - body, cy - body, body * 2.0f, body * 2.0f, 1.0f);

    float const px = cx + std::sin(value_angle) * body;
    float const py = cy - std::cos(value_angle) * body;
    g.setColour(active);
    g.drawLine(cx, cy, px, py, 2.0f);

    g.setColour(Theme::TEXT);
    g.setFont(11.0f);
    g.drawText(format_value(), value_area, juce::Justification::centred, false);
}


void Knob::commit(double const new_ratio)
{
    ratio = juce::jlimit(0.0, 1.0, new_ratio);
    bridge.set_ratio(param_id, ratio);
    repaint();
}


void Knob::mouseDown(juce::MouseEvent const& /* event */)
{
    dragging = true;
    drag_start_ratio = ratio;
}


void Knob::mouseDrag(juce::MouseEvent const& event)
{
    double const sensitivity = (
        event.mods.isCtrlDown() ? DRAG_PIXELS_FULL_RANGE * 5.0
                                : DRAG_PIXELS_FULL_RANGE
    );
    double const delta = -(double)event.getDistanceFromDragStartY() / sensitivity;

    commit(drag_start_ratio + delta);
}


void Knob::mouseUp(juce::MouseEvent const& /* event */)
{
    dragging = false;
}


void Knob::mouseDoubleClick(juce::MouseEvent const& /* event */)
{
    commit(bridge.get_default_ratio(param_id));
}


void Knob::mouseWheelMove(
        juce::MouseEvent const& /* event */,
        juce::MouseWheelDetails const& wheel
) {
    commit(ratio + (double)wheel.deltaY * WHEEL_STEP);
}

}
