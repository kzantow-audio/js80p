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

#include "ui/mix_knob.hpp"

#include "ui/theme.hpp"


namespace JS80P
{

static constexpr float MIX_START_ANGLE = juce::MathConstants<float>::pi * 1.2f;
static constexpr float MIX_END_ANGLE = juce::MathConstants<float>::pi * 2.8f;
static constexpr double MIX_DRAG_PIXELS = 220.0;   /* matches Knob's feel */
static constexpr float MIX_RING_BAND = 10.0f;

static float mix_angle_of(double const r)
{
    return MIX_START_ANGLE + (float)r * (MIX_END_ANGLE - MIX_START_ANGLE);
}


MixKnob::MixKnob(
        ParamBridge& bridge,
        Synth::ParamId const wet_id,
        Synth::ParamId const dry_id,
        juce::String label
) : bridge(bridge),
    wet_id(wet_id),
    dry_id(dry_id),
    label(std::move(label)),
    mix(0.5),
    drag_start(0.0),
    dragging(false)
{
    setWantsKeyboardFocus(false);
    mix = read_mix();
}


double MixKnob::read_mix() const
{
    double const wet = bridge.get_ratio(wet_id);
    double const dry = bridge.get_ratio(dry_id);

    /* Right half keeps WET at full and fades DRY; left half keeps DRY at full
     * and fades WET. Both full == centre. */
    if (wet >= dry) {
        return juce::jlimit(0.5, 1.0, 1.0 - dry * 0.5);
    }

    return juce::jlimit(0.0, 0.5, wet * 0.5);
}


void MixKnob::write_mix(double const m)
{
    mix = juce::jlimit(0.0, 1.0, m);
    bridge.set_ratio(wet_id, juce::jmin(1.0, 2.0 * mix));
    bridge.set_ratio(dry_id, juce::jmin(1.0, 2.0 * (1.0 - mix)));
    repaint();
}


void MixKnob::refresh()
{
    if (dragging) {
        return;
    }

    double const live = read_mix();

    if (std::fabs(live - mix) > 1.0e-6) {
        mix = live;
        repaint();
    }
}


juce::Rectangle<float> MixKnob::knob_circle() const
{
    juce::Rectangle<int> b = getLocalBounds().reduced(2);
    b.removeFromTop(14);
    b.removeFromBottom(14);

    float const size = (float)juce::jmin(b.getWidth(), b.getHeight());
    return b.toFloat().withSizeKeepingCentre(size, size).reduced(3.0f);
}


void MixKnob::paint(juce::Graphics& g)
{
    juce::Rectangle<int> b = getLocalBounds().reduced(2);
    juce::Rectangle<int> const label_area = b.removeFromTop(14);
    juce::Rectangle<int> const value_area = b.removeFromBottom(14);

    g.setColour(Theme::TEXT_DIM);
    g.setFont(11.0f);
    g.drawText(label, label_area, juce::Justification::centred, false);

    juce::Rectangle<float> const kb = knob_circle();
    float const cx = kb.getCentreX();
    float const cy = kb.getCentreY();
    float const r = kb.getWidth() * 0.5f;

    juce::PathStrokeType const stroke(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);

    juce::Path track;
    track.addCentredArc(cx, cy, r, r, 0.0f, MIX_START_ANGLE, MIX_END_ANGLE, true);
    g.setColour(Theme::TRACK);
    g.strokePath(track, stroke);

    float const pos_angle = mix_angle_of(mix);

    juce::Path value_arc;
    value_arc.addCentredArc(cx, cy, r, r, 0.0f, MIX_START_ANGLE, pos_angle, true);
    g.setColour(Theme::ACCENT);
    g.strokePath(value_arc, stroke);

    float const body = r - 4.0f;
    g.setColour(Theme::PANEL_2);
    g.fillEllipse(cx - body, cy - body, body * 2.0f, body * 2.0f);
    g.setColour(Theme::EDGE);
    g.drawEllipse(cx - body, cy - body, body * 2.0f, body * 2.0f, 1.0f);

    g.setColour(Theme::ACCENT);
    g.drawLine(cx, cy, cx + std::sin(pos_angle) * body, cy - std::cos(pos_angle) * body, 2.0f);

    /* Readout: WET / DRY percentages, whichever side is being attenuated. */
    int const wet_pct = (int)std::lround(juce::jmin(1.0, 2.0 * mix) * 100.0);
    int const dry_pct = (int)std::lround(juce::jmin(1.0, 2.0 * (1.0 - mix)) * 100.0);
    g.setColour(Theme::TEXT);
    g.setFont(11.0f);
    g.drawText(
        juce::String(wet_pct) + "/" + juce::String(dry_pct),
        value_area, juce::Justification::centred, false
    );
}


bool MixKnob::hitTest(int x, int y)
{
    juce::Rectangle<float> const kb = knob_circle();
    float const reach = kb.getWidth() * 0.5f + MIX_RING_BAND;
    return kb.getCentre().getDistanceFrom(juce::Point<float>((float)x, (float)y)) <= reach;
}


void MixKnob::mouseDown(juce::MouseEvent const& /* event */)
{
    dragging = true;
    drag_start = mix;
}


void MixKnob::mouseDrag(juce::MouseEvent const& event)
{
    bool const fine = event.mods.isCtrlDown();
    double const sensitivity = fine ? MIX_DRAG_PIXELS * 5.0 : MIX_DRAG_PIXELS;
    double const delta = -(double)event.getDistanceFromDragStartY() / sensitivity;
    write_mix(drag_start + delta);
}


void MixKnob::mouseDoubleClick(juce::MouseEvent const& /* event */)
{
    write_mix(0.5);
}


void MixKnob::mouseWheelMove(
        juce::MouseEvent const& /* event */,
        juce::MouseWheelDetails const& wheel
) {
    write_mix(mix + (double)wheel.deltaY * 0.04);
}

}
