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

#include "ui/vslider.hpp"

#include "dsp/math.hpp"
#include "ui/theme.hpp"


namespace JS80P
{

VSlider::VSlider(
        ParamBridge& bridge,
        std::vector<Synth::ParamId> value_targets,
        juce::String name,
        std::vector<Synth::ParamId> curve_targets
) : bridge(bridge),
    value_targets(std::move(value_targets)),
    curve_targets(std::move(curve_targets)),
    name(std::move(name)),
    ratio(0.0),
    min_ratio(0.0),
    curve_index(0),
    curve_falling(false),
    dragging_curve(false),
    drag_start_ratio(0.0),
    drag_start_curve(0)
{
    setWantsKeyboardFocus(false);
    ratio = bridge.get_ratio(this->value_targets.front());

    if (!this->curve_targets.empty()) {
        curve_index = bridge.get_discrete(this->curve_targets.front());
    }
}


void VSlider::set_min_ratio(double const r)
{
    min_ratio = juce::jlimit(0.0, 1.0, r);
}


void VSlider::set_curve_falling(bool const falling)
{
    curve_falling = falling;
}


void VSlider::refresh()
{
    bool changed = false;
    double const live = bridge.get_ratio(value_targets.front());

    if (std::fabs(live - ratio) > 1.0e-6) {
        ratio = live;
        changed = true;
    }

    if (!curve_targets.empty()) {
        int const ci = bridge.get_discrete(curve_targets.front());

        if (ci != curve_index) {
            curve_index = ci;
            changed = true;
        }
    }

    if (changed) {
        repaint();
    }
}


juce::Rectangle<int> VSlider::bar() const
{
    return juce::Rectangle<int>(getWidth() - BAR_W, 1, BAR_W, getHeight() - 2);
}


juce::Rectangle<int> VSlider::curve_rect() const
{
    if (curve_targets.empty()) {
        return {};
    }

    /* A tiny square, same width as the bar, under the name + value. */
    return juce::Rectangle<int>(1, 27, BAR_W, BAR_W);
}


void VSlider::write_ratio(double const r)
{
    ratio = juce::jlimit(min_ratio, 1.0, r);

    for (Synth::ParamId const p : value_targets) {
        bridge.set_ratio(p, ratio);
    }

    repaint();
}


void VSlider::set_curve(int const index)
{
    int const count = bridge.option_count(curve_targets.front());
    int const idx = juce::jlimit(0, juce::jmax(0, count - 1), index);

    curve_index = idx;

    for (Synth::ParamId const p : curve_targets) {
        bridge.set_discrete(p, idx);
    }

    repaint();
}


void VSlider::paint(juce::Graphics& g)
{
    juce::Rectangle<int> const b = bar();
    juce::Rectangle<int> const text = getLocalBounds().withTrimmedRight(BAR_W + 4);

    /* Name + value, top-aligned. */
    g.setColour(Theme::TEXT_DIM);
    g.setFont(10.0f);
    g.drawText(name, text.getX(), 1, text.getWidth(), 12, juce::Justification::topLeft, false);

    double const v = bridge.display_value(value_targets.front(), ratio);
    juce::String const vs = juce::String(v, std::fabs(v) >= 100.0 ? 0 : 2);
    g.setColour(Theme::TEXT);
    g.setFont(10.0f);
    g.drawText(vs, text.getX(), 13, text.getWidth(), 12, juce::Justification::topLeft, false);

    /* Curvature picker: the actual hardcoded envelope shape, drag/wheel cycles. */
    if (!curve_targets.empty()) {
        juce::Rectangle<int> const cr = curve_rect();
        g.setColour(Theme::INSET);
        g.fillRoundedRectangle(cr.toFloat(), 2.0f);

        int const linear = 12;   /* SHAPE_LINEAR; not in Math::EnvelopeShape (0-11) */
        juce::Rectangle<float> const in = cr.toFloat().reduced(2.0f);
        juce::Path curve;
        for (int i = 0; i <= 14; ++i) {
            double const x = (double)i / 14.0;
            double y = curve_index >= linear
                ? x
                : (double)Math::apply_envelope_shape((Math::EnvelopeShape)curve_index, (Number)x);
            if (curve_falling) {
                y = 1.0 - y;
            }
            float const px = in.getX() + (float)x * in.getWidth();
            float const py = in.getBottom() - (float)y * in.getHeight();
            if (i == 0) curve.startNewSubPath(px, py);
            else curve.lineTo(px, py);
        }
        g.setColour(Theme::ENV);
        g.strokePath(curve, juce::PathStrokeType(1.4f));
    }

    /* Bar. */
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
    if (!curve_targets.empty() && curve_rect().contains(event.getPosition())) {
        dragging_curve = true;
        drag_start_curve = curve_index;
        return;
    }

    dragging_curve = false;
    drag_start_ratio = ratio;
}


void VSlider::mouseDrag(juce::MouseEvent const& event)
{
    if (dragging_curve) {
        int const step = (int)std::lround(-(double)event.getDistanceFromDragStartY() / 12.0);
        set_curve(drag_start_curve + step);
        return;
    }

    /* Relative drag; Ctrl slows it down for fine editing. */
    double const sensitivity =
        event.mods.isCtrlDown() ? DRAG_PIXELS_FULL_RANGE * 5.0 : DRAG_PIXELS_FULL_RANGE;
    double const delta = -(double)event.getDistanceFromDragStartY() / sensitivity;
    write_ratio(drag_start_ratio + delta);
}


void VSlider::mouseDoubleClick(juce::MouseEvent const& event)
{
    if (!curve_targets.empty() && curve_rect().contains(event.getPosition())) {
        set_curve(bridge.get_discrete_default(curve_targets.front()));
        return;
    }

    write_ratio(bridge.get_default_ratio(value_targets.front()));
}


void VSlider::mouseWheelMove(
        juce::MouseEvent const& event, juce::MouseWheelDetails const& wheel
) {
    if (!curve_targets.empty() && curve_rect().contains(event.getPosition())) {
        set_curve(curve_index + (wheel.deltaY > 0.0f ? 1 : -1));
        return;
    }

    write_ratio(ratio + (double)wheel.deltaY * 0.04);
}

}
