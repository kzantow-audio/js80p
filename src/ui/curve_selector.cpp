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

#include "ui/curve_selector.hpp"

#include "dsp/math.hpp"
#include "ui/theme.hpp"


namespace JS80P
{

CurveSelector::CurveSelector(
        ParamBridge& bridge,
        std::vector<Synth::ParamId> targets,
        bool const falling,
        bool const distortion
) : bridge(bridge),
    targets(std::move(targets)),
    falling(falling),
    distortion(distortion),
    index(0),
    drag_start(0)
{
    setWantsKeyboardFocus(false);
    index = bridge.get_discrete(this->targets.front());
}


void CurveSelector::refresh()
{
    int const live = bridge.get_discrete(targets.front());

    if (live != index) {
        index = live;
        repaint();
    }
}


void CurveSelector::set_index(int const idx)
{
    int const count = bridge.option_count(targets.front());
    index = juce::jlimit(0, juce::jmax(0, count - 1), idx);

    for (Synth::ParamId const p : targets) {
        bridge.set_discrete(p, index);
    }

    repaint();
}


void CurveSelector::paint(juce::Graphics& g)
{
    juce::Rectangle<float> const box = getLocalBounds().toFloat();
    g.setColour(Theme::INSET);
    g.fillRoundedRectangle(box, 2.0f);

    int const linear = 12;   /* SHAPE_LINEAR; not in Math::EnvelopeShape (0-11) */
    juce::Rectangle<float> const in = box.reduced(2.0f);
    juce::Path curve;

    for (int i = 0; i <= 14; ++i) {
        double const x = (double)i / 14.0;
        double y = distortion
            ? (double)Math::distort(1.0, (Number)x, (Math::DistortionCurve)index)
            : (index >= linear
                ? x
                : (double)Math::apply_envelope_shape((Math::EnvelopeShape)index, (Number)x));
        if (falling) {
            y = 1.0 - y;
        }
        float const px = in.getX() + (float)x * in.getWidth();
        float const py = in.getBottom() - (float)y * in.getHeight();
        if (i == 0) curve.startNewSubPath(px, py);
        else curve.lineTo(px, py);
    }

    g.setColour(distortion ? Theme::MACRO : Theme::ENV);
    g.strokePath(curve, juce::PathStrokeType(1.4f));
}


void CurveSelector::mouseDown(juce::MouseEvent const& /* event */)
{
    drag_start = index;
}


void CurveSelector::mouseDrag(juce::MouseEvent const& event)
{
    int const step = (int)std::lround(-(double)event.getDistanceFromDragStartY() / 12.0);
    set_index(drag_start + step);
}


void CurveSelector::mouseWheelMove(
        juce::MouseEvent const& /* event */, juce::MouseWheelDetails const& wheel
) {
    set_index(index + (wheel.deltaY > 0.0f ? 1 : -1));
}


void CurveSelector::mouseDoubleClick(juce::MouseEvent const& /* event */)
{
    set_index(bridge.get_discrete_default(targets.front()));
}

}
