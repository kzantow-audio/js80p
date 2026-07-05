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

#include "ui/env_scale_slider.hpp"

#include "ui/theme.hpp"


namespace JS80P
{

/* Global sources assignable through an intermediate macro (label + ControllerId),
 * matching the "Modulate by" submenu offered by envelope knobs (knob.cpp). */
static struct { char const* name; int id; } const SCL_SOURCES[] = {
    { "Macro 1", 131 }, { "Macro 2", 132 }, { "Macro 3", 133 }, { "Macro 4", 134 },
    { "Macro 5", 135 }, { "Macro 6", 136 }, { "Macro 7", 137 }, { "Macro 8", 138 },
    { "Mod wheel", 1 }, { "Breath", 2 }, { "Foot", 4 }, { "Volume", 7 },
    { "Pan", 10 }, { "Expression", 11 }, { "Sustain", 64 }, { "Cutoff CC74", 74 },
    { "Pitch wheel", 128 }, { "Note", 129 }, { "Velocity", 130 }, { "Aftertouch", 155 },
    { "MIDI Learn", 156 },
};
static constexpr int SCL_SOURCE_COUNT = (int)(sizeof(SCL_SOURCES) / sizeof(SCL_SOURCES[0]));


EnvScaleSlider::EnvScaleSlider(
        ParamBridge& bridge,
        ModulationManager& manager,
        int const rep,
        std::vector<int> const& members
) : bridge(bridge),
    manager(manager),
    scl_param(Modulation::env_scl(rep)),
    ratio(bridge.get_ratio(Modulation::env_scl(rep))),
    assigned(false),
    mod_type(Modulation::MACRO),
    mod_slot(0),
    base(0.0),
    depth(0.0),
    dragging(false),
    dragging_depth(false),
    drag_start_base(0.0),
    drag_start_depth(0.0)
{
    setWantsKeyboardFocus(false);

    for (int const m : members) {
        if (m != rep) {
            mirrors.push_back(Modulation::env_scl(m));
        }
    }
}


bool EnvScaleSlider::has_source() const
{
    return assigned;
}


juce::Rectangle<float> EnvScaleSlider::track_rect() const
{
    /* [ SCL ][ ==== track ==== ][ badge ][ value ] laid out left-to-right. */
    juce::Rectangle<int> b = getLocalBounds();
    b.removeFromLeft(24);         /* "SCL" caption */
    b.removeFromRight(30);        /* value read-out */
    b.removeFromRight(badge_rect().getWidth() + 4.0f);   /* badge column */
    return b.reduced(2, 0).toFloat();
}


juce::Rectangle<float> EnvScaleSlider::badge_rect() const
{
    juce::Rectangle<int> b = getLocalBounds();
    juce::Rectangle<int> const value_area = b.removeFromRight(30);

    float w = 18.0f;

    if (assigned) {
        juce::Font const bf(juce::FontOptions().withHeight(10.0f).withStyle("Bold"));
        w = juce::jmax(18.0f, juce::GlyphArrangement::getStringWidth(bf, mod_label) + 8.0f);
    }

    float const h = 12.0f;
    float const x = (float)value_area.getX() - 2.0f - w;
    float const y = (float)b.getCentreY() - h * 0.5f;
    return juce::Rectangle<float>(x, y, w, h);
}


void EnvScaleSlider::update_assignment()
{
    Modulation::Type type;
    int slot;
    bool const now = Modulation::decode(bridge.controller(scl_param), type, slot);

    if (now == assigned && (!now || (type == mod_type && slot == mod_slot))) {
        return;
    }

    assigned = now;

    if (now) {
        mod_type = type;
        mod_slot = slot;

        mod_label = juce::String(Modulation::prefix(type)) + juce::String(slot);
        Synth::ControllerId src = Synth::ControllerId::NONE;
        bool is_random = false;

        if (type == Modulation::MACRO) {
            src = bridge.controller(Modulation::macro_in(slot));
            if (src != Synth::ControllerId::NONE) {
                mod_label = Modulation::source_short_name(src);
            } else if (bridge.get_ratio(Modulation::macro_rnd(slot)) >= 0.99) {
                mod_label = "Rnd";
                is_random = true;
            }
        }

        mod_colour = is_random ? Theme::MACRO : Modulation::assigned_colour(type, src);
        read_base_depth();
    }

    repaint();
}


void EnvScaleSlider::read_base_depth()
{
    Synth::ParamId const base_param =
        mod_type == Modulation::ENVELOPE ? Modulation::env_ini(mod_slot)
      : mod_type == Modulation::LFO ? Modulation::lfo_min(mod_slot)
      : Modulation::macro_min(mod_slot);

    base = bridge.get_ratio(base_param);
    depth = bridge.get_ratio(Modulation::depth_param(mod_type, mod_slot)) - base;
}


void EnvScaleSlider::write_base(double const b)
{
    base = juce::jlimit(0.0, 1.0, b);

    if (!assigned) {
        ratio = base;
        bridge.set_ratio(scl_param, base);
        for (Synth::ParamId const m : mirrors) {
            bridge.set_ratio(m, base);
        }
        repaint();
        return;
    }

    /* Assigned: the base is the intermediate's min; the depth target rides on top. */
    if (mod_type == Modulation::ENVELOPE) {
        bridge.set_ratio(Modulation::env_ini(mod_slot), base);
        bridge.set_ratio(Modulation::env_fin(mod_slot), base);
    } else if (mod_type == Modulation::LFO) {
        bridge.set_ratio(Modulation::lfo_min(mod_slot), base);
    } else {
        bridge.set_ratio(Modulation::macro_min(mod_slot), base);
    }

    bridge.set_ratio(
        Modulation::depth_param(mod_type, mod_slot), juce::jlimit(0.0, 1.0, base + depth)
    );
    repaint();
}


void EnvScaleSlider::apply_depth(double const d)
{
    depth = juce::jlimit(-1.0, 1.0, d);
    bridge.set_ratio(
        Modulation::depth_param(mod_type, mod_slot), juce::jlimit(0.0, 1.0, base + depth)
    );
    repaint();
}


void EnvScaleSlider::assign_mirrors(int const macro_slot)
{
    if (macro_slot == 0) {
        return;
    }

    Synth::ControllerId const cid = Modulation::controller_id(Modulation::MACRO, macro_slot);
    for (Synth::ParamId const m : mirrors) {
        bridge.assign_controller(m, cid);
    }
}


void EnvScaleSlider::clear_mirrors()
{
    for (Synth::ParamId const m : mirrors) {
        bridge.assign_controller(m, Synth::ControllerId::NONE);
    }
}


void EnvScaleSlider::refresh()
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
        double const live = bridge.get_ratio(scl_param);

        if (std::fabs(live - ratio) > 1.0e-6) {
            ratio = live;
            repaint();
        }
    }
}


juce::String EnvScaleSlider::format_value(double const r) const
{
    double const value = bridge.display_value(scl_param, r);
    int const decimals =
        std::fabs(value) >= 100.0 ? 0 : (std::fabs(value) >= 10.0 ? 1 : 2);
    return juce::String(value, decimals);
}


void EnvScaleSlider::paint(juce::Graphics& g)
{
    juce::Rectangle<int> const label_area = getLocalBounds().removeFromLeft(24);
    juce::Rectangle<int> const value_area = getLocalBounds().removeFromRight(30);

    g.setColour(Theme::TEXT_DIM);
    g.setFont(juce::Font(juce::FontOptions().withHeight(9.5f).withStyle("Bold")));
    g.drawText("SCL", label_area, juce::Justification::centredLeft, false);

    juce::Rectangle<float> const track = track_rect();
    float const ty = track.getCentreY();
    float const x0 = track.getX();
    float const x1 = track.getRight();
    float const span = juce::jmax(1.0f, x1 - x0);

    juce::Colour const active = assigned ? mod_colour : Theme::ACCENT;
    double const position = assigned ? base : ratio;
    float const px = x0 + (float)position * span;

    /* Groove. */
    g.setColour(Theme::TRACK);
    g.fillRoundedRectangle(x0, ty - 2.0f, span, 4.0f, 2.0f);

    /* Filled portion up to the base position. */
    g.setColour(assigned ? active.withAlpha(0.55f) : active);
    g.fillRoundedRectangle(x0, ty - 2.0f, juce::jmax(0.0f, px - x0), 4.0f, 2.0f);

    /* Reach segment base -> base+depth (only when assigned). */
    if (assigned) {
        float const target = (float)juce::jlimit(0.0, 1.0, base + depth);
        float const tx = x0 + target * span;
        float const a = juce::jmin(px, tx);
        float const b = juce::jmax(px, tx);
        g.setColour(active);
        g.fillRoundedRectangle(a, ty - 5.0f, juce::jmax(1.0f, b - a), 2.0f, 1.0f);
        g.fillEllipse(tx - 2.0f, ty - 4.0f, 4.0f, 4.0f);
    }

    /* Thumb. */
    g.setColour(Theme::PANEL_2);
    g.fillEllipse(px - 4.0f, ty - 4.0f, 8.0f, 8.0f);
    g.setColour(active);
    g.drawEllipse(px - 4.0f, ty - 4.0f, 8.0f, 8.0f, assigned ? 1.5f : 1.0f);

    /* Source badge: solid labelled chip when assigned, else a faint placeholder. */
    juce::Rectangle<float> const badge = badge_rect();

    if (assigned) {
        g.setColour(active.withAlpha(dragging_depth ? 0.35f : 0.18f));
        g.fillRoundedRectangle(badge, 3.0f);
        g.setColour(active);
        g.setFont(juce::Font(juce::FontOptions().withHeight(10.0f).withStyle("Bold")));
        g.drawText(mod_label, badge, juce::Justification::centred, false);
    } else {
        g.setColour(Theme::EDGE);
        g.drawRoundedRectangle(badge, 3.0f, 1.0f);
    }

    /* Value read-out: base value, or the modulation target while dragging depth. */
    double const shown = dragging_depth ? juce::jlimit(0.0, 1.0, base + depth) : position;
    g.setColour(dragging_depth ? active : Theme::TEXT);
    g.setFont(10.0f);
    g.drawText(format_value(shown), value_area, juce::Justification::centredRight, false);
}


void EnvScaleSlider::mouseDown(juce::MouseEvent const& event)
{
    if (event.mods.isPopupMenu()) {
        open_assign_menu();
        return;
    }

    if (badge_rect().expanded(3.0f).contains(event.position)) {
        dragging = true;
        dragging_depth = assigned;   /* no range to drag without a source */
        drag_start_depth = depth;
        repaint();
        return;
    }

    dragging = true;
    dragging_depth = false;
    drag_start_base = base;
}


void EnvScaleSlider::mouseDrag(juce::MouseEvent const& event)
{
    if (!dragging) {
        return;
    }

    bool const fine = event.mods.isCtrlDown();

    if (dragging_depth) {
        double const sens = fine ? DRAG_PIXELS_FULL_RANGE * 5.0 : DRAG_PIXELS_FULL_RANGE;
        double const delta = -(double)event.getDistanceFromDragStartY() / sens;
        apply_depth(drag_start_depth + delta);
        return;
    }

    /* Horizontal drag over the track width sweeps the full range. */
    double const track_w = juce::jmax(40.0f, track_rect().getWidth());
    double const sens = fine ? track_w * 5.0 : track_w;
    double const delta = (double)event.getDistanceFromDragStartX() / sens;
    write_base(drag_start_base + delta);
}


void EnvScaleSlider::mouseUp(juce::MouseEvent const& /* event */)
{
    bool const was_depth = dragging_depth;
    dragging = false;
    dragging_depth = false;

    if (was_depth) {
        repaint();
    }
}


void EnvScaleSlider::mouseDoubleClick(juce::MouseEvent const& event)
{
    if (assigned && badge_rect().expanded(3.0f).contains(event.position)) {
        apply_depth(0.0);
        return;
    }

    write_base(bridge.get_default_ratio(scl_param));
}


void EnvScaleSlider::open_assign_menu()
{
    juce::PopupMenu menu;

    if (assigned) {
        menu.addSectionHeader(
            juce::String(Modulation::prefix(mod_type)) + juce::String(mod_slot)
        );
        menu.addItem(4, "Remove modulation");
        menu.addSeparator();
    }

    /* Envelope SCL accepts macro sources only: global sources route through a
     * fresh intermediate macro carrying this destination's range. */
    bool const have_macro = manager.free_count(Modulation::MACRO) > 0;
    juce::PopupMenu sources;
    for (int i = 0; i != SCL_SOURCE_COUNT; ++i) {
        sources.addItem(200 + i, SCL_SOURCES[i].name, have_macro);
    }
    sources.addSeparator();
    sources.addItem(300, "Random", have_macro);
    menu.addSubMenu(
        "Modulate by  (" + juce::String(manager.free_count(Modulation::MACRO)) + ")", sources
    );

    EnvScaleSlider* const self = this;
    menu.showMenuAsync(
        juce::PopupMenu::Options().withTargetComponent(this),
        [self](int const result) {
            double const b = self->bridge.get_ratio(self->scl_param);
            int slot = 0;

            if (result == 4) {
                self->manager.unassign(self->scl_param);
                self->clear_mirrors();
                self->update_assignment();
                return;
            }

            if (result >= 200 && result - 200 < SCL_SOURCE_COUNT) {
                slot = self->manager.assign_source(
                    self->scl_param, (Synth::ControllerId)SCL_SOURCES[result - 200].id, b
                );
            } else if (result == 300) {
                slot = self->manager.assign_random(self->scl_param, b);
            } else {
                return;
            }

            self->assign_mirrors(slot);
            self->update_assignment();
        }
    );
}

}
