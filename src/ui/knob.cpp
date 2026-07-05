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
#include <memory>
#include <utility>
#include <vector>

#include "ui/knob.hpp"

#include "ui/theme.hpp"


namespace JS80P
{

static constexpr float START_ANGLE = juce::MathConstants<float>::pi * 1.2f;
static constexpr float END_ANGLE = juce::MathConstants<float>::pi * 2.8f;

/* Global sources assignable through an intermediate macro (label + ControllerId). */
static struct { char const* name; int id; } const KNOB_SOURCES[] = {
    { "Macro 1", 131 }, { "Macro 2", 132 }, { "Macro 3", 133 }, { "Macro 4", 134 },
    { "Macro 5", 135 }, { "Macro 6", 136 }, { "Macro 7", 137 }, { "Macro 8", 138 },
    { "Mod wheel", 1 }, { "Breath", 2 }, { "Foot", 4 }, { "Volume", 7 },
    { "Pan", 10 }, { "Expression", 11 }, { "Sustain", 64 }, { "Cutoff CC74", 74 },
    { "Pitch wheel", 128 }, { "Note", 129 }, { "Velocity", 130 }, { "Aftertouch", 155 },
    { "MIDI Learn", 156 },
};
static constexpr int KNOB_SOURCE_COUNT = (int)(sizeof(KNOB_SOURCES) / sizeof(KNOB_SOURCES[0]));

static float angle_of(double const r)
{
    return START_ANGLE + (float)r * (END_ANGLE - START_ANGLE);
}


Knob::Knob(
        ParamBridge& bridge,
        Synth::ParamId const param_id,
        juce::String const& label
) : bridge(bridge),
    param_id(param_id),
    label(label),
    manager(nullptr),
    ratio(bridge.get_ratio(param_id)),
    skew(1.0),
    min_ratio(0.0),
    drag_start_visual(0.0),
    dragging(false),
    dragging_depth(false),
    drag_start_depth(0.0),
    semitone_snap(false),
    assigned(false),
    mod_type(Modulation::LFO),
    mod_slot(0),
    base(0.0),
    depth(0.5)
{
    setWantsKeyboardFocus(false);
}


void Knob::set_manager(ModulationManager* const m) { manager = m; }
void Knob::set_semitone_snap(bool const on) { semitone_snap = on; }
void Knob::set_min_ratio(double const r) { min_ratio = juce::jlimit(0.0, 1.0, r); }


double Knob::ratio_to_visual(double const r) const
{
    return (skew == 1.0 || r <= 0.0) ? r : std::pow(r, 1.0 / skew);
}


double Knob::visual_to_ratio(double const v) const
{
    return (skew == 1.0 || v <= 0.0) ? v : std::pow(v, skew);
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
        if ((bridge.display_value(param_id, mid) < target) == increasing) lo = mid;
        else hi = mid;
    }

    return 0.5 * (lo + hi);
}


void Knob::set_center_value(double const display_value)
{
    double const at_min = bridge.display_value(param_id, 0.0);
    double const at_max = bridge.display_value(param_id, 1.0);

    if (display_value <= juce::jmin(at_min, at_max) || display_value >= juce::jmax(at_min, at_max)) {
        return;
    }

    double const center_ratio = ratio_for_display(display_value);

    if (center_ratio > 1.0e-4 && center_ratio < 1.0 - 1.0e-4) {
        skew = juce::jlimit(0.1, 10.0, std::log(center_ratio) / std::log(0.5));
    }
}


void Knob::update_assignment()
{
    Modulation::Type type;
    int slot;
    bool const now = manager != nullptr
        && Modulation::decode(bridge.controller(param_id), type, slot);

    if (now != assigned || (now && (type != mod_type || slot != mod_slot))) {
        assigned = now;

        if (now) {
            mod_type = type;
            mod_slot = slot;

            /* An intermediate macro (input driven by a global source) shows the
             * source's name and colour, not its pool slot. */
            mod_label = juce::String(Modulation::prefix(type)) + juce::String(slot);
            Synth::ControllerId src = Synth::ControllerId::NONE;

            if (type == Modulation::MACRO) {
                src = bridge.controller(Modulation::macro_in(slot));
                if (src != Synth::ControllerId::NONE) {
                    mod_label = Modulation::source_short_name(src);
                }
            }

            mod_colour = Modulation::assigned_colour(type, src);
            read_base_depth();
        }

        repaint();
    }
}


void Knob::read_base_depth()
{
    Synth::ParamId const base_param =
        mod_type == Modulation::ENVELOPE ? Modulation::env_ini(mod_slot)
      : mod_type == Modulation::LFO ? Modulation::lfo_min(mod_slot)
      : Modulation::macro_min(mod_slot);

    base = bridge.get_ratio(base_param);
    depth = bridge.get_ratio(Modulation::depth_param(mod_type, mod_slot)) - base;
}


void Knob::apply_base(double const b)
{
    base = juce::jlimit(0.0, 1.0, b);

    if (mod_type == Modulation::ENVELOPE) {
        bridge.set_ratio(Modulation::env_ini(mod_slot), base);
        bridge.set_ratio(Modulation::env_sus(mod_slot), base);
        bridge.set_ratio(Modulation::env_fin(mod_slot), base);
    } else if (mod_type == Modulation::LFO) {
        bridge.set_ratio(Modulation::lfo_min(mod_slot), base);
    } else {
        bridge.set_ratio(Modulation::macro_min(mod_slot), base);
    }

    bridge.set_ratio(Modulation::depth_param(mod_type, mod_slot),
                     juce::jlimit(0.0, 1.0, base + depth));
    repaint();
}


void Knob::apply_depth(double const d)
{
    depth = juce::jlimit(-1.0, 1.0, d);
    bridge.set_ratio(Modulation::depth_param(mod_type, mod_slot),
                     juce::jlimit(0.0, 1.0, base + depth));
    repaint();
}


void Knob::refresh()
{
    update_assignment();

    if (dragging) {
        return;
    }

    if (assigned) {
        double const b0 = base;
        double const d0 = depth;
        read_base_depth();

        if (std::fabs(b0 - base) > 1.0e-6 || std::fabs(d0 - depth) > 1.0e-6) {
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
    return format_ratio(ratio);
}


juce::String Knob::format_ratio(double const r) const
{
    double const value = bridge.display_value(param_id, r);

    if (semitone_snap) {
        double const semitones = value / 100.0;
        return std::fabs(semitones - std::round(semitones)) < 0.005
            ? juce::String((int)std::round(semitones))
            : juce::String(semitones, 2);
    }

    int const decimals = bridge.is_discrete(param_id)
        ? 0
        : (std::fabs(value) >= 100.0 ? 0 : (std::fabs(value) >= 10.0 ? 1 : 2));

    return juce::String(value, decimals);
}


juce::Rectangle<float> Knob::knob_circle() const
{
    juce::Rectangle<int> b = getLocalBounds().reduced(2);
    b.removeFromTop(14);
    b.removeFromBottom(14);

    float const size = (float)juce::jmin(b.getWidth(), b.getHeight());
    return b.toFloat().withSizeKeepingCentre(size, size).reduced(3.0f);
}


juce::Rectangle<float> Knob::badge_rect() const
{
    /* Lower-left corner sits on the reach ring at the top-right (45 deg), so the
     * badge meets the modulation-amount curve regardless of cell width. Width
     * fits the label with >= 4px padding each side and grows rightward. */
    juce::Rectangle<float> const kb = knob_circle();
    juce::Font const bf(juce::FontOptions().withHeight(10.0f).withStyle("Bold"));
    float const tw = juce::GlyphArrangement::getStringWidth(bf, mod_label);
    float const w = juce::jmax(16.0f, tw + 8.0f);
    float const h = 12.0f;
    float const rr = kb.getWidth() * 0.5f + 3.0f;
    float const diag = 0.7071f;
    float x = kb.getCentreX() + diag * rr + 3.0f;
    float y = kb.getCentreY() - diag * rr - h + 3.0f;
    x = juce::jlimit(0.0f, juce::jmax(0.0f, (float)getWidth() - w), x);
    y = juce::jmax(0.0f, y);
    return juce::Rectangle<float>(x, y, w, h);
}


void Knob::paint(juce::Graphics& g)
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
    track.addCentredArc(cx, cy, r, r, 0.0f, START_ANGLE, END_ANGLE, true);
    g.setColour(Theme::TRACK);
    g.strokePath(track, stroke);

    juce::Colour const active = assigned ? mod_colour : Theme::ACCENT;
    double const position = ratio_to_visual(assigned ? base : ratio);
    float const pos_angle = angle_of(position);

    juce::Path value_arc;
    value_arc.addCentredArc(cx, cy, r, r, 0.0f, START_ANGLE, pos_angle, true);
    g.setColour(assigned ? active.withAlpha(0.55f) : active);
    g.strokePath(value_arc, stroke);

    float const body = r - 4.0f;
    g.setColour(Theme::PANEL_2);
    g.fillEllipse(cx - body, cy - body, body * 2.0f, body * 2.0f);
    g.setColour(assigned ? active : Theme::EDGE);
    g.drawEllipse(cx - body, cy - body, body * 2.0f, body * 2.0f, assigned ? 1.5f : 1.0f);

    g.setColour(active);
    g.drawLine(cx, cy + 0.0f,
               cx + std::sin(pos_angle) * body, cy - std::cos(pos_angle) * body, 2.0f);

    if (!assigned) {
        g.setColour(Theme::TEXT);
        g.setFont(11.0f);
        g.drawText(format_value(), value_area, juce::Justification::centred, false);
        return;
    }

    /* Outer reach ring: base -> base+depth (shows sign, direction, target). */
    float const target = (float)ratio_to_visual(juce::jlimit(0.0, 1.0, base + depth));
    float const target_angle = angle_of(target);
    float const rr = r + 3.0f;
    float const a0 = juce::jmin(pos_angle, target_angle);
    float const a1 = juce::jmax(pos_angle, target_angle);

    juce::Path reach;
    reach.addCentredArc(cx, cy, rr, rr, 0.0f, a0, a1, true);
    g.setColour(active);
    g.strokePath(reach, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    float const tx = cx + std::sin(target_angle) * rr;
    float const ty = cy - std::cos(target_angle) * rr;
    g.fillEllipse(tx - 2.0f, ty - 2.0f, 4.0f, 4.0f);

    /* Top-right source handle: drag it to set the modulation amount. */
    juce::Rectangle<float> const badge = badge_rect();
    g.setColour(active.withAlpha(dragging_depth ? 0.35f : 0.18f));
    g.fillRoundedRectangle(badge, 3.0f);
    g.setColour(active);
    g.setFont(juce::Font(juce::FontOptions().withHeight(10.0f).withStyle("Bold")));
    g.drawText(mod_label, badge, juce::Justification::centred, false);

    /* Below the knob: the line (base) value, or the amount while dragging it. */
    double const shown = dragging_depth ? juce::jlimit(0.0, 1.0, base + depth) : base;
    g.setColour(dragging_depth ? active : Theme::TEXT);
    g.setFont(11.0f);
    g.drawText(format_ratio(shown), value_area, juce::Justification::centred, false);
}


void Knob::commit(double const new_ratio)
{
    ratio = juce::jlimit(min_ratio, 1.0, new_ratio);
    bridge.set_ratio(param_id, ratio);
    repaint();
}


void Knob::open_assign_menu()
{
    juce::PopupMenu menu;

    if (assigned) {
        menu.addSectionHeader(
            juce::String(Modulation::prefix(mod_type)) + juce::String(mod_slot)
        );
        menu.addItem(4, "Remove modulation");
        menu.addSeparator();
    }

    std::shared_ptr<std::vector<std::pair<Modulation::Type, int>>> const clones =
        std::make_shared<std::vector<std::pair<Modulation::Type, int>>>();

    for (ModulationManager::Group const& grp : manager->groups()) {
        int const id = 100 + (int)clones->size();
        clones->push_back({ grp.type, grp.rep });
        menu.addItem(id,
            "Copy " + juce::String(Modulation::prefix(grp.type)) + juce::String(grp.rep));
    }

    menu.addSeparator();
    menu.addItem(1, "New envelope  (" + juce::String(manager->free_count(Modulation::ENVELOPE)) + ")",
                 manager->free_count(Modulation::ENVELOPE) > 0);
    menu.addItem(2, "New LFO  (" + juce::String(manager->free_count(Modulation::LFO)) + ")",
                 manager->free_count(Modulation::LFO) > 0);

    /* Global sources route through an intermediate macro carrying this
     * destination's unique range. */
    bool const have_macro = manager->free_count(Modulation::MACRO) > 0;
    juce::PopupMenu sources;
    for (int i = 0; i != KNOB_SOURCE_COUNT; ++i) {
        sources.addItem(200 + i, KNOB_SOURCES[i].name, have_macro);
    }
    menu.addSubMenu("Modulate by  (" + juce::String(manager->free_count(Modulation::MACRO)) + ")",
                    sources);

    Knob* const self = this;
    menu.showMenuAsync(
        juce::PopupMenu::Options().withTargetComponent(this),
        [self, clones](int const result) {
            double const b = self->bridge.get_ratio(self->param_id);

            if (result == 1) self->manager->assign(self->param_id, Modulation::ENVELOPE, b, 0);
            else if (result == 2) self->manager->assign(self->param_id, Modulation::LFO, b, 0);
            else if (result == 4) self->manager->unassign(self->param_id);
            else if (result >= 100 && result < 200 && result - 100 < (int)clones->size()) {
                std::pair<Modulation::Type, int> const& gp = (*clones)[result - 100];
                self->manager->assign(self->param_id, gp.first, b, gp.second);
            } else if (result >= 200 && result - 200 < KNOB_SOURCE_COUNT) {
                self->manager->assign_source(
                    self->param_id, (Synth::ControllerId)KNOB_SOURCES[result - 200].id, b
                );
            }

            self->update_assignment();
        }
    );
}


void Knob::mouseDown(juce::MouseEvent const& event)
{
    if (event.mods.isPopupMenu() && manager != nullptr) {
        open_assign_menu();
        return;
    }

    dragging = true;

    if (assigned && badge_rect().expanded(3.0f).contains(event.position)) {
        dragging_depth = true;
        drag_start_depth = depth;
    } else {
        dragging_depth = false;
        drag_start_visual = ratio_to_visual(assigned ? base : ratio);
    }
}


void Knob::mouseDrag(juce::MouseEvent const& event)
{
    double const sensitivity =
        event.mods.isCtrlDown() ? DRAG_PIXELS_FULL_RANGE * 5.0 : DRAG_PIXELS_FULL_RANGE;
    double const delta = -(double)event.getDistanceFromDragStartY() / sensitivity;

    if (dragging_depth) {
        apply_depth(drag_start_depth + delta);
        return;
    }

    double new_ratio = visual_to_ratio(juce::jlimit(0.0, 1.0, drag_start_visual + delta));

    if (assigned) {
        apply_base(new_ratio);
        return;
    }

    if (semitone_snap && !event.mods.isCtrlDown()) {
        double const start_cents = bridge.display_value(param_id, visual_to_ratio(drag_start_visual));
        double const raw_cents = bridge.display_value(param_id, new_ratio);
        new_ratio = ratio_for_display(start_cents + std::round((raw_cents - start_cents) / 100.0) * 100.0);
    }

    commit(new_ratio);
}


void Knob::mouseUp(juce::MouseEvent const& /* event */)
{
    bool const was_depth = dragging_depth;
    dragging = false;
    dragging_depth = false;

    if (was_depth) {
        repaint();   /* revert the below-knob value from amount to line value */
    }
}


void Knob::mouseDoubleClick(juce::MouseEvent const& event)
{
    if (assigned) {
        if (badge_rect().expanded(3.0f).contains(event.position)) {
            apply_depth(0.0);   /* upper bound = lower bound: no modulation */
        } else {
            apply_base(bridge.get_default_ratio(param_id));   /* reset base */
        }
        return;
    }

    commit(bridge.get_default_ratio(param_id));
}


void Knob::mouseWheelMove(
        juce::MouseEvent const& event,
        juce::MouseWheelDetails const& wheel
) {
    if (assigned) {
        apply_depth(depth + (double)wheel.deltaY * WHEEL_STEP);
        return;
    }

    if (semitone_snap && !event.mods.isCtrlDown()) {
        double const cents = bridge.display_value(param_id, ratio);
        commit(ratio_for_display(cents + (wheel.deltaY > 0.0f ? 100.0 : -100.0)));
        return;
    }

    double const v = ratio_to_visual(ratio) + (double)wheel.deltaY * WHEEL_STEP;
    commit(visual_to_ratio(juce::jlimit(0.0, 1.0, v)));
}

}
