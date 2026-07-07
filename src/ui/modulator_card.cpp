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

#include "ui/modulator_card.hpp"

#include "ui/theme.hpp"


namespace JS80P
{

ModulatorCard::ModulatorCard(
        ParamBridge& bridge, ModulationManager& manager,
        ModulationManager::Group const& group
) : bridge(bridge),
    manager(manager),
    type(group.type),
    rep(group.rep),
    members(group.members),
    destinations(group.destinations),
    connections(group.connections),
    sus_fraction(0.0),
    lfo_expanded(false)
{
    setWantsKeyboardFocus(false);

    auto build_curve = [this](Synth::ParamId (*fn)(int)) {
        std::vector<Synth::ParamId> v;
        for (int m : members) {
            v.push_back(fn(m));
        }
        return v;
    };

    /* The other grouped copies' matching param, so modulating a knob applies to
     * every copy. */
    auto mirror = [this](Synth::ParamId (*fn)(int)) {
        std::vector<Synth::ParamId> v;
        for (int m : members) {
            if (m != rep) {
                v.push_back(fn(m));
            }
        }
        return v;
    };

    if (type == Modulation::ENVELOPE) {
        knobs.add(new Knob(bridge, Modulation::env_del(rep), "DLY"));
        knobs.add(new Knob(bridge, Modulation::env_atk(rep), "ATK"));
        knobs.getLast()->set_min_ratio(bridge.ratio_for_display(Modulation::env_atk(rep), 0.001));
        knobs.add(new Knob(bridge, Modulation::env_hld(rep), "HLD"));
        knobs.add(new Knob(bridge, Modulation::env_dec(rep), "DEC"));
        knobs.add(new Knob(bridge, Modulation::env_rel(rep), "REL"));

        for (Knob* const k : knobs) {
            k->set_manager(&manager);
            k->set_mod_caps(Modulation::CAP_MACRO);   /* envelope params: macros only */
            k->set_center_value(1.0);   /* time knobs: exponential, 1 second midpoint */
        }

        knobs[0]->set_mirrors(mirror(Modulation::env_del));
        knobs[1]->set_mirrors(mirror(Modulation::env_atk));
        knobs[2]->set_mirrors(mirror(Modulation::env_hld));
        knobs[3]->set_mirrors(mirror(Modulation::env_dec));
        knobs[4]->set_mirrors(mirror(Modulation::env_rel));

        curves.add(new CurveSelector(bridge, build_curve(Modulation::env_ash), false));
        curves.add(new CurveSelector(bridge, build_curve(Modulation::env_dsh), true));
        curves.add(new CurveSelector(bridge, build_curve(Modulation::env_rsh), true));

        for (CurveSelector* const c : curves) {
            addAndMakeVisible(c);
        }

        /* Sustain as a fraction between each destination's min (INI) and max
         * (PK); read the current fraction from the representative. */
        double const ini = bridge.get_ratio(Modulation::env_ini(rep));
        double const pk = bridge.get_ratio(Modulation::env_pk(rep));
        sus_fraction = (pk - ini) > 1.0e-6
            ? juce::jlimit(0.0, 1.0, (bridge.get_ratio(Modulation::env_sus(rep)) - ini) / (pk - ini))
            : 0.0;

        double const def_ini = bridge.get_default_ratio(Modulation::env_ini(rep));
        double const def_pk = bridge.get_default_ratio(Modulation::env_pk(rep));
        double const def_sus = bridge.get_default_ratio(Modulation::env_sus(rep));
        double const def_fraction = (def_pk - def_ini) > 1.0e-6
            ? juce::jlimit(0.0, 1.0, (def_sus - def_ini) / (def_pk - def_ini)) : 1.0;

        sus_fader = std::make_unique<Control>(
            bridge, Modulation::env_sus(rep), "SUS",
            Control::Style::V_SLIDER, Control::Size::NORMAL
        );
        sus_fader->set_value_hooks(
            [this]() { return sus_fraction; },
            [this](double const v) { sus_fraction = v; write_sustain(); },
            [](double const v) { return juce::String(v, 2); }
        );
        sus_fader->set_hook_default(def_fraction);
        /* SUS is a macro-modulation destination like the other envelope params;
         * once assigned it edits env_sus directly (the fraction hook and the
         * periodic write_sustain step aside — see refresh / write_sustain). */
        sus_fader->set_manager(&manager);
        sus_fader->set_mod_caps(Modulation::CAP_MACRO);
        sus_fader->set_mirrors(mirror(Modulation::env_sus));
        addAndMakeVisible(*sus_fader);

        /* SCL lives in the middle of the header row (not among the knobs): a
         * horizontal slider that is a macro-only modulation destination, its
         * value/modulation mirrored onto every grouped member's SCL. */
        std::vector<Synth::ParamId> scl_mirrors;
        for (int const m : members) {
            if (m != rep) {
                scl_mirrors.push_back(Modulation::env_scl(m));
            }
        }

        env_scale = std::make_unique<Control>(
            bridge, Modulation::env_scl(rep), "SCL",
            Control::Style::H_SLIDER, Control::Size::SMALL
        );
        env_scale->set_manager(&manager);
        env_scale->set_mod_caps(Modulation::CAP_MACRO);
        env_scale->set_label_placement(Control::LabelPos::LEFT);
        env_scale->set_value_display(Control::ValueDisplay::POPOVER);
        env_scale->set_source_placeholder(true);
        env_scale->set_mirrors(scl_mirrors);
        env_scale->set_value_mirrors(scl_mirrors);
        addAndMakeVisible(*env_scale);

        /* Two tiny pie dots in the header's far-right band: time inaccuracy and
         * level inaccuracy, mirrored onto every grouped member. */
        tin_dot = std::make_unique<Control>(
            bridge, Modulation::env_tin(rep), "Time inacc.",
            Control::Style::DOT, Control::Size::TINY
        );
        tin_dot->set_value_display(Control::ValueDisplay::POPOVER);
        tin_dot->set_value_mirrors(mirror(Modulation::env_tin));
        tin_dot->setTooltip("Envelope time inaccuracy");
        addAndMakeVisible(*tin_dot);

        vin_dot = std::make_unique<Control>(
            bridge, Modulation::env_vin(rep), "Level inacc.",
            Control::Style::DOT, Control::Size::TINY
        );
        vin_dot->set_value_display(Control::ValueDisplay::POPOVER);
        vin_dot->set_value_mirrors(mirror(Modulation::env_vin));
        vin_dot->setTooltip("Envelope level inaccuracy");
        addAndMakeVisible(*vin_dot);
    } else if (type == Modulation::LFO) {
        wave = std::make_unique<WaveformSelector>(bridge, Modulation::lfo_wav(rep));
        wave->set_single(true);
        wave->set_on_click([this] { set_expanded(true); });
        addAndMakeVisible(*wave);

        sync_button = std::make_unique<SyncButton>();
        sync_button->setWantsKeyboardFocus(false);
        sync_button->is_active = [this] {
            return this->bridge.get_discrete(Modulation::lfo_syn(rep)) != 0;
        };
        sync_button->on_toggle = [this] { toggle_sync(); };
        addAndMakeVisible(*sync_button);

        shape_grid = std::make_unique<WaveformSelector>(bridge, Modulation::lfo_wav(rep));
        shape_grid->set_on_select([this](int) { set_expanded(false); });
        addChildComponent(*shape_grid);

        knobs.add(new Knob(bridge, Modulation::lfo_frq(rep), "RATE"));
        knobs.add(new Knob(bridge, Modulation::lfo_phs(rep), "PHS"));
        knobs.add(new Knob(bridge, Modulation::lfo_dst(rep), "DIST"));
        knobs.add(new Knob(bridge, Modulation::lfo_rnd(rep), "RAND"));

        for (Knob* const k : knobs) {
            k->set_manager(&manager);
            k->set_mod_caps(Modulation::CAP_LFO | Modulation::CAP_MACRO);   /* LFOs + macros */
        }

        knobs[0]->set_mirrors(mirror(Modulation::lfo_frq));
        knobs[1]->set_mirrors(mirror(Modulation::lfo_phs));
        knobs[2]->set_mirrors(mirror(Modulation::lfo_dst));
        knobs[3]->set_mirrors(mirror(Modulation::lfo_rnd));
    }

    for (Knob* const k : knobs) {
        addAndMakeVisible(k);
    }
}


int ModulatorCard::preferred_height() const
{
    /* Match the oscillator knob size (cell height 72) + header + padding. */
    return 100;
}


void ModulatorCard::set_expanded(bool const expanded)
{
    lfo_expanded = expanded;
    resized();
    repaint();
}


void ModulatorCard::write_sustain()
{
    for (int m : members) {
        double const ini = bridge.get_ratio(Modulation::env_ini(m));
        double const pk = bridge.get_ratio(Modulation::env_pk(m));
        double const target = juce::jlimit(0.0, 1.0, ini + sus_fraction * (pk - ini));

        if (std::fabs(bridge.get_ratio(Modulation::env_sus(m)) - target) > 1.0e-5) {
            bridge.set_ratio(Modulation::env_sus(m), target);
        }
    }
}


void ModulatorCard::toggle_sync()
{
    int const on = bridge.get_discrete(Modulation::lfo_syn(rep)) != 0 ? 0 : 1;

    for (int m : members) {
        bridge.set_discrete(Modulation::lfo_syn(m), on);
    }

    repaint();
}


void ModulatorCard::propagate()
{
    for (int m : members) {
        if (m == rep) {
            continue;
        }

        if (type == Modulation::ENVELOPE) {
            int const off[] = { 0, 2, 3, 5, 6, 8 };  /* SCL, DEL, ATK, HLD, DEC, REL */
            for (int o : off) {
                bridge.set_ratio(Modulation::pid((int)Modulation::env_scl(m) + o),
                                 bridge.get_ratio(Modulation::pid((int)Modulation::env_scl(rep) + o)));
            }
        } else if (type == Modulation::LFO) {
            bridge.set_ratio(Modulation::lfo_wav(m), bridge.get_ratio(Modulation::lfo_wav(rep)));
            bridge.set_ratio(Modulation::lfo_frq(m), bridge.get_ratio(Modulation::lfo_frq(rep)));
            bridge.set_ratio(Modulation::lfo_phs(m), bridge.get_ratio(Modulation::lfo_phs(rep)));
            bridge.set_ratio(Modulation::lfo_dst(m), bridge.get_ratio(Modulation::lfo_dst(rep)));
            bridge.set_ratio(Modulation::lfo_rnd(m), bridge.get_ratio(Modulation::lfo_rnd(rep)));

            if (Modulation::lfo_has_pw(rep) && Modulation::lfo_has_pw(m)) {
                bridge.set_ratio(Modulation::lfo_pw(m), bridge.get_ratio(Modulation::lfo_pw(rep)));
            }
        }
    }
}


void ModulatorCard::refresh()
{
    if (sus_fader != nullptr) {
        /* Track the fraction as min/max change, unless a modulator now owns
         * env_sus (then it must not be overwritten every frame). */
        if (!sus_fader->is_assigned()) {
            write_sustain();
        }
        sus_fader->refresh();
    }

    if (env_scale != nullptr) {
        env_scale->refresh();
    }

    if (tin_dot != nullptr) {
        tin_dot->refresh();
    }

    if (vin_dot != nullptr) {
        vin_dot->refresh();
    }

    for (Knob* const k : knobs) {
        k->refresh();
    }

    for (CurveSelector* const c : curves) {
        c->refresh();
    }

    if (wave != nullptr) {
        wave->refresh();
    }

    if (shape_grid != nullptr) {
        shape_grid->refresh();
    }
}


void ModulatorCard::resized()
{
    juce::Rectangle<int> b = getLocalBounds().reduced(6);

    /* SCL slider in the middle of the header row: destination badges keep the
     * left, and ~40px on the far right stays clear for later per-member circles. */
    if (env_scale != nullptr) {
        int const far_right = 40;
        int const scl_w = 134;
        int const scl_h = 16;
        int const scl_y = 3;
        int scl_x = getWidth() / 2 - scl_w / 2;
        scl_x = juce::jmin(scl_x, getWidth() - far_right - scl_w);
        scl_x = juce::jmax(scl_x, 8);
        env_scale->setBounds(scl_x, scl_y, scl_w, scl_h);
    }

    /* Time / level inaccuracy dots in the reserved far-right header band. */
    if (tin_dot != nullptr && vin_dot != nullptr) {
        int const sz = 12;
        int const gap = 5;
        int const right = getWidth() - 6;
        int const y = 11 - sz / 2;   /* centre on the header line (y 3..19) */
        vin_dot->setBounds(right - sz, y, sz, sz);
        tin_dot->setBounds(right - 2 * sz - gap, y, sz, sz);
    }

    b.removeFromTop(16);   /* one-line header */

    if (type == Modulation::LFO) {
        int const bh = 20;   /* WAVE/BPM height = one oscillator selector button */

        if (lfo_expanded) {
            /* Selector at the oscillator-button height (2 rows x 20px), not filling
             * the card. */
            shape_grid->setBounds(b.removeFromTop(2 * bh));
            shape_grid->setVisible(true);
            wave->setVisible(false);
            sync_button->setVisible(false);
            for (Knob* const k : knobs) k->setVisible(false);
            return;
        }

        shape_grid->setVisible(false);
        wave->setVisible(true);
        sync_button->setVisible(true);

        /* Row: WAVE | BPM | RATE | PHS | DIST | RAND. */
        int const by = b.getY() + (b.getHeight() - bh) / 2;
        int const wave_w = 36;
        int const bpm_w = 30;
        int x = b.getX();

        wave->setBounds(x, by, wave_w, bh);
        x += wave_w + 4;
        sync_button->setBounds(x, by, bpm_w, bh);
        x += bpm_w + 6;

        int const n = knobs.size();
        int const cell = n > 0 ? (b.getRight() - x) / n : 0;
        for (int i = 0; i != n; ++i) {
            knobs[i]->setVisible(true);
            knobs[i]->setBounds(x + i * cell, b.getY(), cell, b.getHeight());
        }
        return;
    }

    /* Envelope row:
     * DLY | A-curve | ATK | HLD | DEC | D-curve | SUS | REL | R-curve.
     * Knobs and the sustain fader are full height; the curve selectors sit in
     * their own space along the bottom edge. */
    int const cw = 16;   /* curve square */
    int const sw = 26;   /* sustain fader */
    int const kw = juce::jmax(28, (b.getWidth() - 3 * cw - sw) / 5);
    int const total = 5 * kw + 3 * cw + sw;
    int const h = b.getHeight();
    int x = b.getX() + juce::jmax(0, (b.getWidth() - total) / 2);

    /* Bottom of a knob's circle (mirrors Knob::knob_circle), so the curve
     * squares line up with the base of the knobs, not the value text. */
    int const circle = juce::jmin(kw - 4, h - 32);
    int const circle_bottom = h / 2 + circle / 2 - 3;

    auto place_knob = [&](int const i) {
        knobs[i]->setBounds(x, b.getY(), kw, h);
        x += kw;
    };
    auto place_curve = [&](int const i) {
        curves[i]->setBounds(x, b.getY() + circle_bottom - cw, cw, cw);
        x += cw;
    };

    place_knob(0);    /* DLY */
    place_curve(0);   /* attack curve */
    place_knob(1);    /* ATK */
    place_knob(2);    /* HLD */
    place_knob(3);    /* DEC */
    place_curve(1);   /* decay curve */
    sus_fader->setBounds(x, b.getY(), sw, h);   /* SUS */
    x += sw;
    place_knob(4);    /* REL */
    place_curve(2);   /* release curve */
}


void ModulatorCard::SyncButton::paint(juce::Graphics& g)
{
    bool const on = is_active && is_active();
    juce::Colour const c = Theme::ACCENT;
    juce::Rectangle<float> const r = getLocalBounds().toFloat().reduced(0.5f);
    float const radius = 2.0f;

    if (on) {
        g.setColour(hover ? c.brighter(0.15f) : c);
        g.fillRoundedRectangle(r, radius);
    } else if (hover) {
        g.setColour(c.withAlpha(0.18f));
        g.fillRoundedRectangle(r, radius);
    }

    g.setColour(on || !hover ? c : c.brighter(0.2f));
    g.drawRoundedRectangle(r, radius, 1.0f);

    /* Solid fill needs dark text for contrast; the outline state keeps it orange. */
    g.setColour(on ? Theme::BG : c);
    g.setFont(9.0f);
    g.drawText("BPM", getLocalBounds(), juce::Justification::centred, false);
}


void ModulatorCard::SyncButton::mouseUp(juce::MouseEvent const& event)
{
    if (on_toggle && getLocalBounds().contains(event.getPosition())) {
        on_toggle();
    }
}


void ModulatorCard::SyncButton::mouseEnter(juce::MouseEvent const& /* event */)
{
    hover = true;
    repaint();
}


void ModulatorCard::SyncButton::mouseExit(juce::MouseEvent const& /* event */)
{
    hover = false;
    repaint();
}


void ModulatorCard::paint(juce::Graphics& g)
{
    /* Transparent background; a single 2px accent line on the left. */
    g.setColour(Modulation::colour(type));
    g.fillRect(0.0f, 0.0f, 2.0f, (float)getHeight());

    /* One-line header: each slot next to its destination -
     * "E1 <dest> E5 <dest> ...". */
    juce::Font const sf(juce::FontOptions().withHeight(12.0f).withStyle("Bold"));
    juce::Font const df(juce::FontOptions().withHeight(11.0f));
    int const right = getWidth() - 8;
    int x = 8;

    for (std::pair<int, Synth::ParamId> const& c : connections) {
        if (x >= right) {
            break;
        }

        juce::String const slot = juce::String(Modulation::prefix(type)) + juce::String(c.first);
        g.setFont(sf);
        g.setColour(Modulation::colour(type));
        g.drawText(slot, x, 3, right - x, 14, juce::Justification::centredLeft, false);
        x += (int)juce::GlyphArrangement::getStringWidth(sf, slot) + 4;

        juce::String const dest = Modulation::display_dest_name(bridge.param_name(c.second));
        g.setFont(df);
        g.setColour(Theme::TEXT_DIM);
        g.drawText(dest, x, 3, right - x, 14, juce::Justification::centredLeft, false);
        x += (int)juce::GlyphArrangement::getStringWidth(df, dest) + 10;
    }
}

}
