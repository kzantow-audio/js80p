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
    curve_index(0),
    dragging_curve(false),
    drag_start_curve(0)
{
    setWantsKeyboardFocus(false);
    ratio = bridge.get_ratio(this->value_targets.front());

    if (!this->curve_targets.empty()) {
        curve_index = bridge.get_discrete(this->curve_targets.front());
    }
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

    int const w = getWidth() - BAR_W - 5;
    int const top = 27;   /* below name + value rows */
    int const h = juce::jmin(30, getHeight() - top - 2);
    return juce::Rectangle<int>(1, top, w, juce::jmax(0, h));
}


void VSlider::set_value_from_mouse(int const y)
{
    juce::Rectangle<int> const b = bar();
    double const r = juce::jlimit(0.0, 1.0, 1.0 - (double)(y - b.getY()) / (double)b.getHeight());

    ratio = r;

    for (Synth::ParamId const p : value_targets) {
        bridge.set_ratio(p, r);
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

    /* Curvature picker glyph (drag up/down to cycle). */
    if (!curve_targets.empty()) {
        juce::Rectangle<int> const cr = curve_rect();
        g.setColour(Theme::INSET);
        g.fillRoundedRectangle(cr.toFloat(), 2.0f);

        int const count = juce::jmax(2, bridge.option_count(curve_targets.front()));
        double const t = (double)curve_index / (double)(count - 1);
        double const gamma = std::pow(4.0, (t - 0.5) * 2.0);

        juce::Rectangle<float> const in = cr.toFloat().reduced(3.0f);
        juce::Path curve;
        for (int i = 0; i <= 16; ++i) {
            double const x = (double)i / 16.0;
            double const yv = std::pow(x, gamma);
            float const px = in.getX() + (float)x * in.getWidth();
            float const py = in.getBottom() - (float)yv * in.getHeight();
            if (i == 0) curve.startNewSubPath(px, py);
            else curve.lineTo(px, py);
        }
        g.setColour(Theme::ENV);
        g.strokePath(curve, juce::PathStrokeType(1.5f));
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
    set_value_from_mouse(event.y);
}


void VSlider::mouseDrag(juce::MouseEvent const& event)
{
    if (dragging_curve) {
        int const step = (int)std::lround(-(double)event.getDistanceFromDragStartY() / 12.0);
        set_curve(drag_start_curve + step);
        return;
    }

    set_value_from_mouse(event.y);
}


void VSlider::mouseDoubleClick(juce::MouseEvent const& event)
{
    if (!curve_targets.empty() && curve_rect().contains(event.getPosition())) {
        set_curve(bridge.get_discrete_default(curve_targets.front()));
        return;
    }

    ratio = bridge.get_default_ratio(value_targets.front());

    for (Synth::ParamId const p : value_targets) {
        bridge.set_ratio(p, ratio);
    }

    repaint();
}

}
