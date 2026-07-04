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
    skew(1.0),
    drag_start_visual(0.0),
    dragging(false)
{
    setWantsKeyboardFocus(false);
}


double Knob::ratio_to_visual(double const r) const
{
    if (skew == 1.0 || r <= 0.0) {
        return r;
    }

    return std::pow(r, 1.0 / skew);
}


double Knob::visual_to_ratio(double const v) const
{
    if (skew == 1.0 || v <= 0.0) {
        return v;
    }

    return std::pow(v, skew);
}


void Knob::set_center_value(double const display_value)
{
    double const at_min = bridge.display_value(param_id, 0.0);
    double const at_max = bridge.display_value(param_id, 1.0);
    double const lo_value = juce::jmin(at_min, at_max);
    double const hi_value = juce::jmax(at_min, at_max);

    if (display_value <= lo_value || display_value >= hi_value) {
        return;   /* out of range: keep the linear mapping */
    }

    bool const increasing = at_max >= at_min;
    double lo = 0.0;
    double hi = 1.0;

    for (int i = 0; i != 40; ++i) {
        double const mid = 0.5 * (lo + hi);

        if ((bridge.display_value(param_id, mid) < display_value) == increasing) {
            lo = mid;
        } else {
            hi = mid;
        }
    }

    double const center_ratio = 0.5 * (lo + hi);

    if (center_ratio > 1.0e-4 && center_ratio < 1.0 - 1.0e-4) {
        skew = juce::jlimit(0.1, 10.0, std::log(center_ratio) / std::log(0.5));
    }
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
    float const visual = (float)ratio_to_visual(ratio);
    float const value_angle = start_angle + visual * (end_angle - start_angle);

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
    drag_start_visual = ratio_to_visual(ratio);
}


void Knob::mouseDrag(juce::MouseEvent const& event)
{
    double const sensitivity = (
        event.mods.isCtrlDown() ? DRAG_PIXELS_FULL_RANGE * 5.0
                                : DRAG_PIXELS_FULL_RANGE
    );
    double const delta = -(double)event.getDistanceFromDragStartY() / sensitivity;

    commit(visual_to_ratio(juce::jlimit(0.0, 1.0, drag_start_visual + delta)));
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
    double const v = ratio_to_visual(ratio) + (double)wheel.deltaY * WHEEL_STEP;
    commit(visual_to_ratio(juce::jlimit(0.0, 1.0, v)));
}

}
