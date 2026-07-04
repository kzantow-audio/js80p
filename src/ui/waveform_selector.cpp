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

#include "ui/waveform_selector.hpp"

#include "ui/theme.hpp"


namespace JS80P
{

/* One period of each waveform, y in [-1, 1]. Soft variants share the base
 * shape (marked separately with a dot); indices match Oscillator::Waveform. */
static double waveform_sample(int const shape, double const p)
{
    double const tau = 2.0 * juce::MathConstants<double>::pi;

    switch (shape) {
        case 0:  return std::sin(tau * p);                       /* SINE */
        case 1:  case 2:  return 2.0 * p - 1.0;                  /* SAW */
        case 3:  case 4:  return 1.0 - 2.0 * p;                  /* INV SAW */
        case 5:  case 6:  return p < 0.5 ? (4.0 * p - 1.0)
                                         : (3.0 - 4.0 * p);      /* TRIANGLE */
        case 7:  case 8:  return p < 0.5 ? 1.0 : -1.0;           /* SQUARE */
        case 9:  case 10: return p < 0.3 ? 1.0 : -1.0;           /* PULSE */
        case 11: case 12: return p < 0.25 ? 1.0
                               : (p < 0.5 ? -1.0 : 0.0);         /* BIPOLAR */
        default: return 0.6 * std::sin(tau * p)
                       + 0.4 * std::sin(3.0 * tau * p);          /* CUSTOM */
    }
}


static bool waveform_is_soft(int const shape)
{
    return shape == 2 || shape == 4 || shape == 6
        || shape == 8 || shape == 10 || shape == 12;
}


WaveformSelector::WaveformSelector(
        ParamBridge& bridge, Synth::ParamId const param_id
) : bridge(bridge),
    param_id(param_id),
    selected(bridge.get_discrete(param_id))
{
    setWantsKeyboardFocus(false);
}


void WaveformSelector::refresh()
{
    int const live = bridge.get_discrete(param_id);

    if (live != selected) {
        selected = live;
        repaint();
    }
}


juce::Rectangle<int> WaveformSelector::cell_bounds(int const index) const
{
    juce::Rectangle<int> const b = getLocalBounds();
    int const col = index % COLUMNS;
    int const row = index / COLUMNS;
    int const w = b.getWidth() / COLUMNS;
    int const h = b.getHeight() / ROWS;

    return juce::Rectangle<int>(b.getX() + col * w, b.getY() + row * h, w, h);
}


void WaveformSelector::draw_glyph(
        juce::Graphics& g, juce::Rectangle<float> area, int const shape
) const {
    juce::Path path;
    int const samples = 40;
    float const amp = area.getHeight() * 0.34f;
    float const cy = area.getCentreY();

    for (int i = 0; i != samples; ++i) {
        double const p = (double)i / (double)(samples - 1);
        float const x = area.getX() + (float)p * area.getWidth();
        float const y = cy - (float)waveform_sample(shape, p) * amp;

        if (i == 0) {
            path.startNewSubPath(x, y);
        } else {
            path.lineTo(x, y);
        }
    }

    g.strokePath(
        path,
        juce::PathStrokeType(1.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded)
    );

    if (waveform_is_soft(shape)) {
        g.fillEllipse(area.getRight() - 3.0f, area.getY() + 1.0f, 2.5f, 2.5f);
    }
}


void WaveformSelector::paint(juce::Graphics& g)
{
    for (int i = 0; i != SHAPE_COUNT; ++i) {
        juce::Rectangle<int> const cell = cell_bounds(i).reduced(2);
        bool const is_selected = (i == selected);

        g.setColour(is_selected ? Theme::PANEL_2 : Theme::INSET);
        g.fillRoundedRectangle(cell.toFloat(), 2.0f);

        if (is_selected) {
            g.setColour(Theme::ACCENT);
            g.drawRoundedRectangle(cell.toFloat().reduced(0.5f), 2.0f, 1.2f);
        }

        g.setColour(is_selected ? Theme::ACCENT : Theme::TEXT_DIM);
        draw_glyph(g, cell.toFloat().reduced(4.0f), i);
    }
}


void WaveformSelector::mouseDown(juce::MouseEvent const& event)
{
    int const col = event.x / juce::jmax(1, getWidth() / COLUMNS);
    int const row = event.y / juce::jmax(1, getHeight() / ROWS);
    int const index = row * COLUMNS + col;

    if (index >= 0 && index < SHAPE_COUNT) {
        selected = index;
        bridge.set_discrete(param_id, index);
        repaint();
    }
}

}
