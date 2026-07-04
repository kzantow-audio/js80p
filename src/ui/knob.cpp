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
    allocator(nullptr),
    ratio(bridge.get_ratio(param_id)),
    skew(1.0),
    drag_start_visual(0.0),
    dragging(false),
    semitone_snap(false),
    assigned(false),
    mod_type(Modulation::LFO),
    mod_index(0),
    depth(DEFAULT_DEPTH),
    drag_start_depth(0.0)
{
    setWantsKeyboardFocus(false);
}


void Knob::set_allocator(ModulatorAllocator* const a)
{
    allocator = a;
}


void Knob::set_semitone_snap(bool const on)
{
    semitone_snap = on;
}


double Knob::ratio_for_display(double const target) const
{
    double const at_min = bridge.display_value(param_id, 0.0);
    double const at_max = bridge.display_value(param_id, 1.0);
    bool const increasing = at_max >= at_min;
    double lo = 0.0;
    double hi = 1.0;

    for (int i = 0; i != 40; ++i) {
        double const mid = 0.5 * (lo + hi);

        if ((bridge.display_value(param_id, mid) < target) == increasing) {
            lo = mid;
        } else {
            hi = mid;
        }
    }

    return 0.5 * (lo + hi);
}


double Knob::ratio_to_visual(double const r) const
{
    return (skew == 1.0 || r <= 0.0) ? r : std::pow(r, 1.0 / skew);
}


double Knob::visual_to_ratio(double const v) const
{
    return (skew == 1.0 || v <= 0.0) ? v : std::pow(v, skew);
}


void Knob::set_center_value(double const display_value)
{
    double const at_min = bridge.display_value(param_id, 0.0);
    double const at_max = bridge.display_value(param_id, 1.0);
    double const lo_value = juce::jmin(at_min, at_max);
    double const hi_value = juce::jmax(at_min, at_max);

    if (display_value <= lo_value || display_value >= hi_value) {
        return;
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


void Knob::update_assignment()
{
    Modulation::Type type;
    int index;
    bool const now = Modulation::decode(bridge.controller(param_id), type, index);

    if (now != assigned || (now && (type != mod_type || index != mod_index))) {
        assigned = now;

        if (now) {
            mod_type = type;
            mod_index = index;
        }

        repaint();
    }
}


double Knob::read_depth() const
{
    switch (mod_type) {
        case Modulation::LFO:
            return juce::jlimit(0.0, 1.0,
                bridge.get_ratio(Modulation::lfo_max(mod_index))
                - bridge.get_ratio(Modulation::lfo_min(mod_index)));
        case Modulation::MACRO:
            return juce::jlimit(0.0, 1.0,
                bridge.get_ratio(Modulation::macro_max(mod_index))
                - bridge.get_ratio(Modulation::macro_min(mod_index)));
        default:
            return juce::jlimit(0.0, 1.0, bridge.get_ratio(Modulation::env_peak(mod_index)));
    }
}


void Knob::write_depth(double const d)
{
    double const c = juce::jlimit(0.0, 1.0, d);

    switch (mod_type) {
        case Modulation::LFO:
            bridge.set_ratio(Modulation::lfo_min(mod_index), 0.5 - c * 0.5);
            bridge.set_ratio(Modulation::lfo_max(mod_index), 0.5 + c * 0.5);
            break;
        case Modulation::MACRO:
            bridge.set_ratio(Modulation::macro_min(mod_index), 0.5 - c * 0.5);
            bridge.set_ratio(Modulation::macro_max(mod_index), 0.5 + c * 0.5);
            break;
        default:
            bridge.set_ratio(Modulation::env_init(mod_index), 0.0);
            bridge.set_ratio(Modulation::env_peak(mod_index), c);
            bridge.set_ratio(Modulation::env_sus(mod_index), c);
            break;
    }
}


void Knob::refresh()
{
    update_assignment();

    if (dragging) {
        return;
    }

    if (assigned) {
        double const live = read_depth();

        if (std::fabs(live - depth) > 1.0e-6) {
            depth = live;
            repaint();
        }
    } else {
        double const live = bridge.get_ratio(param_id);

        if (std::fabs(live - ratio) > 1.0e-6) {
            ratio = live;
            repaint();
        }
    }
}


juce::String Knob::format_value() const
{
    double const value = bridge.display_value(param_id, ratio);

    if (semitone_snap) {
        double const semitones = value / 100.0;

        return std::fabs(semitones - std::round(semitones)) < 0.005
            ? juce::String((int)std::round(semitones))
            : juce::String(semitones, 2);
    }

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

    juce::PathStrokeType const stroke(
        3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded
    );

    juce::Path track;
    track.addCentredArc(cx, cy, r, r, 0.0f, start_angle, end_angle, true);
    g.setColour(Theme::TRACK);
    g.strokePath(track, stroke);

    juce::Colour const active = assigned ? Modulation::colour(mod_type) : Theme::ACCENT;
    float const visual = (float)(assigned ? depth : ratio_to_visual(ratio));
    float const value_angle = start_angle + visual * (end_angle - start_angle);

    juce::Path value_arc;
    value_arc.addCentredArc(cx, cy, r, r, 0.0f, start_angle, value_angle, true);
    g.setColour(active);
    g.strokePath(value_arc, stroke);

    float const body = r - 4.0f;
    g.setColour(Theme::PANEL_2);
    g.fillEllipse(cx - body, cy - body, body * 2.0f, body * 2.0f);
    g.setColour(assigned ? active : Theme::EDGE);
    g.drawEllipse(cx - body, cy - body, body * 2.0f, body * 2.0f, assigned ? 1.5f : 1.0f);

    float const px = cx + std::sin(value_angle) * body;
    float const py = cy - std::cos(value_angle) * body;
    g.setColour(active);
    g.drawLine(cx, cy, px, py, 2.0f);

    if (assigned) {
        /* Type badge over the knob. */
        juce::String const badge = juce::String(Modulation::prefix(mod_type)) + juce::String(mod_index);
        g.setColour(active);
        g.setFont(juce::Font(juce::FontOptions().withHeight(10.0f).withStyle("Bold")));
        g.drawText(badge, kb.toNearestInt(), juce::Justification::centred, false);

        /* Secondary display: the modulation range as a bar. */
        float lo = 0.0f;
        float hi = (float)depth;

        if (mod_type != Modulation::ENVELOPE) {
            lo = 0.5f - (float)depth * 0.5f;
            hi = 0.5f + (float)depth * 0.5f;
        }

        juce::Rectangle<float> const bar = value_area.toFloat().reduced(2.0f, 3.0f);
        g.setColour(Theme::INSET);
        g.fillRect(bar);
        g.setColour(active.withAlpha(0.85f));
        g.fillRect(bar.getX() + lo * bar.getWidth(), bar.getY(),
                   (hi - lo) * bar.getWidth(), bar.getHeight());
    } else {
        g.setColour(Theme::TEXT);
        g.setFont(11.0f);
        g.drawText(format_value(), value_area, juce::Justification::centred, false);
    }
}


void Knob::commit(double const new_ratio)
{
    ratio = juce::jlimit(0.0, 1.0, new_ratio);
    bridge.set_ratio(param_id, ratio);
    repaint();
}


void Knob::open_assign_menu()
{
    juce::PopupMenu menu;

    if (assigned) {
        menu.addSectionHeader(
            juce::String(mod_type == Modulation::ENVELOPE ? "Envelope "
                       : mod_type == Modulation::LFO ? "LFO " : "Macro ")
            + juce::String(mod_index)
        );
        menu.addItem(4, "Remove modulation");
        menu.addSeparator();
    }

    menu.addItem(1, "Assign envelope  (" + juce::String(allocator->free_count(Modulation::ENVELOPE)) + " free)",
                 allocator->free_count(Modulation::ENVELOPE) > 0);
    menu.addItem(2, "Assign LFO  (" + juce::String(allocator->free_count(Modulation::LFO)) + " free)",
                 allocator->free_count(Modulation::LFO) > 0);
    menu.addItem(3, "Assign macro  (" + juce::String(allocator->free_count(Modulation::MACRO)) + " free)",
                 allocator->free_count(Modulation::MACRO) > 0);

    Knob* const self = this;
    menu.showMenuAsync(
        juce::PopupMenu::Options().withTargetComponent(this),
        [self](int const result) {
            switch (result) {
                case 1: self->assign(Modulation::ENVELOPE); break;
                case 2: self->assign(Modulation::LFO); break;
                case 3: self->assign(Modulation::MACRO); break;
                case 4: self->remove_assignment(); break;
                default: break;
            }
        }
    );
}


void Knob::assign(Modulation::Type const type)
{
    int const index = allocator->allocate(type);

    if (index == 0) {
        return;   /* pool exhausted */
    }

    mod_type = type;
    mod_index = index;
    assigned = true;
    depth = DEFAULT_DEPTH;

    bridge.assign_controller(param_id, Modulation::controller_id(type, index));
    write_depth(depth);
    repaint();
}


void Knob::remove_assignment()
{
    if (assigned) {
        allocator->release(mod_type, mod_index);
    }

    assigned = false;
    bridge.assign_controller(param_id, Synth::ControllerId::NONE);
    repaint();
}


void Knob::mouseDown(juce::MouseEvent const& event)
{
    if (event.mods.isPopupMenu() && allocator != nullptr) {
        open_assign_menu();
        return;
    }

    dragging = true;

    if (assigned) {
        drag_start_depth = depth;
    } else {
        drag_start_visual = ratio_to_visual(ratio);
    }
}


void Knob::mouseDrag(juce::MouseEvent const& event)
{
    double const sensitivity = (
        event.mods.isCtrlDown() ? DRAG_PIXELS_FULL_RANGE * 5.0
                                : DRAG_PIXELS_FULL_RANGE
    );
    double const delta = -(double)event.getDistanceFromDragStartY() / sensitivity;

    if (assigned) {
        depth = juce::jlimit(0.0, 1.0, drag_start_depth + delta);
        write_depth(depth);
        repaint();
        return;
    }

    double new_ratio = visual_to_ratio(juce::jlimit(0.0, 1.0, drag_start_visual + delta));

    if (semitone_snap && !event.mods.isCtrlDown()) {
        /* Snap the *movement* to whole semitones, keeping any fine offset the
         * drag started from (e.g. 3.5 -> 4.5, not 4.0). */
        double const start_cents = bridge.display_value(param_id, visual_to_ratio(drag_start_visual));
        double const raw_cents = bridge.display_value(param_id, new_ratio);
        double const snapped = start_cents + std::round((raw_cents - start_cents) / 100.0) * 100.0;
        new_ratio = ratio_for_display(snapped);
    }

    commit(new_ratio);
}


void Knob::mouseUp(juce::MouseEvent const& /* event */)
{
    dragging = false;
}


void Knob::mouseDoubleClick(juce::MouseEvent const& /* event */)
{
    if (assigned) {
        depth = DEFAULT_DEPTH;
        write_depth(depth);
        repaint();
    } else {
        commit(bridge.get_default_ratio(param_id));
    }
}


void Knob::mouseWheelMove(
        juce::MouseEvent const& event,
        juce::MouseWheelDetails const& wheel
) {
    if (assigned) {
        depth = juce::jlimit(0.0, 1.0, depth + (double)wheel.deltaY * WHEEL_STEP);
        write_depth(depth);
        repaint();
        return;
    }

    if (semitone_snap && !event.mods.isCtrlDown()) {
        /* Step by exactly one semitone, keeping any fine offset. */
        double const cents = bridge.display_value(param_id, ratio);
        double const step = wheel.deltaY > 0.0f ? 100.0 : -100.0;
        commit(ratio_for_display(cents + step));
        return;
    }

    double const v = ratio_to_visual(ratio) + (double)wheel.deltaY * WHEEL_STEP;
    commit(visual_to_ratio(juce::jlimit(0.0, 1.0, v)));
}

}
