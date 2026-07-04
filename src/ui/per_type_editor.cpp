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

#include "ui/per_type_editor.hpp"

#include "ui/theme.hpp"


namespace JS80P
{

/* Oscillator::Waveform: SQUARE=7 .. SOFT_BIPOLAR_PULSE=12 use pulse width;
 * CUSTOM=13 uses the harmonics. */
static bool waveform_uses_pulse_width(int const w)
{
    return w >= 7 && w <= 12;
}


PerTypeEditor::PerTypeEditor(
        ParamBridge& bridge,
        Synth::ParamId const waveform,
        Synth::ParamId const pulse_width,
        Synth::ParamId const first_harmonic
) : bridge(bridge),
    waveform(waveform),
    pulse_width(pulse_width),
    first_harmonic(first_harmonic),
    pw_knob(bridge, pulse_width, "PW"),
    mode(NONE)
{
    setWantsKeyboardFocus(false);
    addChildComponent(pw_knob);
}


PerTypeEditor::Mode PerTypeEditor::mode_for(int const w) const
{
    if (w == 13) {
        return CUSTOM;
    }

    if (waveform_uses_pulse_width(w)) {
        return PULSE;
    }

    return NONE;
}


void PerTypeEditor::refresh()
{
    Mode const now = mode_for(bridge.get_discrete(waveform));

    if (now != mode) {
        mode = now;
        pw_knob.setVisible(mode == PULSE);
        repaint();
    }

    if (mode == PULSE) {
        pw_knob.refresh();
    } else if (mode == CUSTOM) {
        repaint();   /* bars follow the live harmonic values */
    }
}


void PerTypeEditor::resized()
{
    /* Pulse width: a single knob on the left. */
    int const knob_w = juce::jmin(getWidth() / 4, 76);
    pw_knob.setBounds(4, 0, knob_w, getHeight());
}


juce::Rectangle<int> PerTypeEditor::harmonics_area() const
{
    return getLocalBounds().reduced(4, 6).withTrimmedTop(12);
}


void PerTypeEditor::paint(juce::Graphics& g)
{
    if (mode == CUSTOM) {
        juce::Rectangle<int> const area = harmonics_area();

        g.setColour(Theme::TEXT_DIM);
        g.setFont(10.0f);
        g.drawText("HARMONICS", getLocalBounds().reduced(4, 2).removeFromTop(11),
                   juce::Justification::centredLeft, false);

        float const bar_w = (float)area.getWidth() / (float)HARMONICS;
        float const mid_y = (float)area.getCentreY();

        for (int i = 0; i != HARMONICS; ++i) {
            double const r = bridge.get_ratio((Synth::ParamId)((int)first_harmonic + i));
            float const x = (float)area.getX() + (float)i * bar_w;
            juce::Rectangle<float> const cell(x + 1.0f, (float)area.getY(), bar_w - 2.0f, (float)area.getHeight());

            g.setColour(Theme::INSET);
            g.fillRect(cell);

            /* Custom harmonics are bipolar (0.5 = zero). */
            float const amp = (float)(r - 0.5) * (float)area.getHeight();
            juce::Rectangle<float> const bar(
                cell.getX(),
                amp >= 0.0f ? mid_y - amp : mid_y,
                cell.getWidth(),
                std::abs(amp)
            );
            g.setColour(Theme::ACCENT);
            g.fillRect(bar);
        }

        g.setColour(Theme::EDGE);
        g.drawHorizontalLine((int)mid_y, (float)area.getX(), (float)area.getRight());
    } else if (mode == NONE) {
        g.setColour(Theme::TEXT_FAINT);
        g.setFont(11.0f);
        g.drawText("-", getLocalBounds(), juce::Justification::centred, false);
    }
}


void PerTypeEditor::set_harmonic_from_mouse(juce::Point<int> const pos)
{
    juce::Rectangle<int> const area = harmonics_area();

    if (area.getWidth() <= 0) {
        return;
    }

    int const index = juce::jlimit(
        0, HARMONICS - 1, (pos.x - area.getX()) * HARMONICS / area.getWidth()
    );
    double const r = juce::jlimit(
        0.0, 1.0, 1.0 - (double)(pos.y - area.getY()) / (double)area.getHeight()
    );

    bridge.set_ratio((Synth::ParamId)((int)first_harmonic + index), r);
    repaint();
}


void PerTypeEditor::mouseDown(juce::MouseEvent const& event)
{
    if (mode == CUSTOM) {
        set_harmonic_from_mouse(event.getPosition());
    }
}


void PerTypeEditor::mouseDrag(juce::MouseEvent const& event)
{
    if (mode == CUSTOM) {
        set_harmonic_from_mouse(event.getPosition());
    }
}

}
