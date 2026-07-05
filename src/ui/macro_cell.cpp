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

#include "ui/macro_cell.hpp"

#include "ui/modulation.hpp"
#include "ui/theme.hpp"


namespace JS80P
{

/* Selectable input sources: label + ControllerId. Learn arms MIDI-learn; the
 * well-known CCs and the non-CC sources follow. */
static struct { char const* name; int id; } const SOURCES[] = {
    { "Learn", 156 },        /* MIDI_LEARN */
    { "-",     0 },          /* NONE */
    { "Mod",   1 },
    { "Breath",2 },
    { "Foot",  4 },
    { "Vol",   7 },
    { "Pan",   10 },
    { "Expr",  11 },
    { "Sus",   64 },
    { "Reso",  71 },
    { "Cut",   74 },
    { "Rev",   91 },
    { "Cho",   93 },
    { "Pitch", 128 },        /* PITCH_WHEEL */
    { "Note",  129 },        /* TRIGGERED_NOTE */
    { "Vel",   130 },        /* TRIGGERED_VELOCITY */
    { "AT",    155 },        /* CHANNEL_PRESSURE */
};
static constexpr int SOURCE_COUNT = (int)(sizeof(SOURCES) / sizeof(SOURCES[0]));

static constexpr float START_ANGLE = juce::MathConstants<float>::pi * 1.2f;
static constexpr float END_ANGLE = juce::MathConstants<float>::pi * 2.8f;

static float angle_of(double const r)
{
    return START_ANGLE + (float)r * (END_ANGLE - START_ANGLE);
}


MacroCell::MacroCell(ParamBridge& bridge, int const macro)
    : bridge(bridge),
    macro(macro),
    min_p(Modulation::macro_min(macro)),
    max_p(Modulation::macro_max(macro)),
    in_p(Modulation::macro_in(macro)),
    base(0.0),
    depth(0.0),
    dragging_depth(false),
    badge_press(false),
    press_distance(0),
    drag_start_base(0.0),
    drag_start_depth(0.0)
{
    setWantsKeyboardFocus(false);
    base = bridge.get_ratio(min_p);
    depth = bridge.get_ratio(max_p) - base;
}


void MacroCell::refresh()
{
    double const b = bridge.get_ratio(min_p);
    double const d = bridge.get_ratio(max_p) - b;

    if (std::fabs(b - base) > 1.0e-6 || std::fabs(d - depth) > 1.0e-6) {
        base = b;
        depth = d;
        repaint();
    }
}


juce::Rectangle<float> MacroCell::knob_circle() const
{
    juce::Rectangle<int> b = getLocalBounds().reduced(2);
    b.removeFromTop(13);
    b.removeFromBottom(13);
    float const size = (float)juce::jmin(b.getWidth(), b.getHeight());
    return b.toFloat().withSizeKeepingCentre(size, size).reduced(3.0f);
}


juce::Rectangle<float> MacroCell::badge_rect() const
{
    juce::Rectangle<float> const kb = knob_circle();
    float const w = 26.0f;
    float x = juce::jmin(kb.getRight() + 6.0f, (float)getWidth() - w - 1.0f);
    float const y = juce::jmax(0.0f, kb.getY() - 3.0f);
    return juce::Rectangle<float>(x, y, w, 13.0f);
}


juce::String MacroCell::source_label() const
{
    int const id = (int)bridge.controller(in_p);

    for (int i = 0; i != SOURCE_COUNT; ++i) {
        if (SOURCES[i].id == id) {
            return SOURCES[i].name;
        }
    }

    return juce::String("CC") + juce::String(id);
}


void MacroCell::write_base(double const b)
{
    base = juce::jlimit(0.0, 1.0, b);
    bridge.set_ratio(min_p, base);
    bridge.set_ratio(max_p, juce::jlimit(0.0, 1.0, base + depth));
    repaint();
}


void MacroCell::write_depth(double const d)
{
    depth = juce::jlimit(-1.0, 1.0, d);
    bridge.set_ratio(max_p, juce::jlimit(0.0, 1.0, base + depth));
    repaint();
}


void MacroCell::paint(juce::Graphics& g)
{
    juce::Rectangle<int> b = getLocalBounds().reduced(2);
    juce::Rectangle<int> const label_area = b.removeFromTop(13);
    juce::Rectangle<int> const value_area = b.removeFromBottom(13);

    g.setColour(Theme::TEXT_DIM);
    g.setFont(10.0f);
    g.drawText("M" + juce::String(macro), label_area, juce::Justification::centred, false);

    juce::Rectangle<float> const kb = knob_circle();
    float const cx = kb.getCentreX();
    float const cy = kb.getCentreY();
    float const r = kb.getWidth() * 0.5f;
    juce::Colour const active = Theme::MIDI;

    juce::PathStrokeType const stroke(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);

    juce::Path track;
    track.addCentredArc(cx, cy, r, r, 0.0f, START_ANGLE, END_ANGLE, true);
    g.setColour(Theme::TRACK);
    g.strokePath(track, stroke);

    float const base_angle = angle_of(base);
    juce::Path value_arc;
    value_arc.addCentredArc(cx, cy, r, r, 0.0f, START_ANGLE, base_angle, true);
    g.setColour(active.withAlpha(0.55f));
    g.strokePath(value_arc, stroke);

    float const body = r - 4.0f;
    g.setColour(Theme::PANEL_2);
    g.fillEllipse(cx - body, cy - body, body * 2.0f, body * 2.0f);
    g.setColour(active);
    g.drawEllipse(cx - body, cy - body, body * 2.0f, body * 2.0f, 1.2f);
    g.drawLine(cx, cy, cx + std::sin(base_angle) * body, cy - std::cos(base_angle) * body, 2.0f);

    /* Reach ring: min -> max (the modulation range). */
    float const target = (float)juce::jlimit(0.0, 1.0, base + depth);
    float const target_angle = angle_of(target);
    float const rr = r + 3.0f;
    juce::Path reach;
    reach.addCentredArc(cx, cy, rr, rr, 0.0f, juce::jmin(base_angle, target_angle),
                        juce::jmax(base_angle, target_angle), true);
    g.setColour(active);
    g.strokePath(reach, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    /* Source badge. */
    juce::Rectangle<float> const badge = badge_rect();
    g.setColour(active.withAlpha(dragging_depth ? 0.35f : 0.18f));
    g.fillRoundedRectangle(badge, 3.0f);
    g.setColour(active);
    g.drawRoundedRectangle(badge, 3.0f, 1.0f);
    g.setFont(juce::Font(juce::FontOptions().withHeight(10.0f).withStyle("Bold")));
    g.drawText(source_label(), badge, juce::Justification::centred, false);

    double const shown = dragging_depth ? juce::jlimit(0.0, 1.0, base + depth) : base;
    g.setColour(dragging_depth ? active : Theme::TEXT);
    g.setFont(10.0f);
    g.drawText(juce::String(bridge.display_value(min_p, shown), 0),
               value_area, juce::Justification::centred, false);
}


void MacroCell::mouseDown(juce::MouseEvent const& event)
{
    if (badge_rect().expanded(3.0f).contains(event.position)) {
        badge_press = true;
        dragging_depth = true;
        press_distance = 0;
        drag_start_depth = depth;
        return;
    }

    badge_press = false;
    dragging_depth = false;
    drag_start_base = base;
}


void MacroCell::mouseDrag(juce::MouseEvent const& event)
{
    double const sensitivity =
        event.mods.isCtrlDown() ? DRAG_PIXELS_FULL_RANGE * 5.0 : DRAG_PIXELS_FULL_RANGE;
    double const delta = -(double)event.getDistanceFromDragStartY() / sensitivity;

    if (dragging_depth) {
        press_distance = juce::jmax(press_distance, event.getDistanceFromDragStart());
        write_depth(drag_start_depth + delta);
        return;
    }

    write_base(drag_start_base + delta);
}


void MacroCell::mouseUp(juce::MouseEvent const& /* event */)
{
    bool const was_depth = dragging_depth;
    bool const click = badge_press && press_distance < 4;

    dragging_depth = false;
    badge_press = false;

    if (click) {
        open_source_menu();
    } else if (was_depth) {
        repaint();
    }
}


void MacroCell::mouseDoubleClick(juce::MouseEvent const& event)
{
    if (badge_rect().expanded(3.0f).contains(event.position)) {
        write_depth(0.0);
    } else {
        write_base(bridge.get_default_ratio(min_p));
    }
}


void MacroCell::open_source_menu()
{
    juce::PopupMenu menu;
    int const current = (int)bridge.controller(in_p);

    for (int i = 0; i != SOURCE_COUNT; ++i) {
        menu.addItem(i + 1, SOURCES[i].name, true, SOURCES[i].id == current);
        if (i == 0 || i == 12) {
            menu.addSeparator();
        }
    }

    MacroCell* const self = this;
    menu.showMenuAsync(
        juce::PopupMenu::Options().withTargetComponent(this),
        [self](int const result) {
            if (result <= 0) {
                return;
            }

            self->bridge.assign_controller(self->in_p, (Synth::ControllerId)SOURCES[result - 1].id);
            self->repaint();
        }
    );
}

}
