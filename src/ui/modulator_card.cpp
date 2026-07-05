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

    if (type == Modulation::ENVELOPE) {
        knobs.add(new Knob(bridge, Modulation::env_del(rep), "DLY"));
        knobs.add(new Knob(bridge, Modulation::env_atk(rep), "ATK"));
        knobs.getLast()->set_min_ratio(bridge.ratio_for_display(Modulation::env_atk(rep), 0.001));
        knobs.add(new Knob(bridge, Modulation::env_hld(rep), "HLD"));
        knobs.add(new Knob(bridge, Modulation::env_dec(rep), "DEC"));
        knobs.add(new Knob(bridge, Modulation::env_rel(rep), "REL"));

        for (Knob* const k : knobs) {
            k->set_manager(&manager);
        }

        curves.add(new CurveSelector(bridge, build_curve(Modulation::env_ash), false));
        curves.add(new CurveSelector(bridge, build_curve(Modulation::env_dsh), true));
        curves.add(new CurveSelector(bridge, build_curve(Modulation::env_rsh), true));

        for (CurveSelector* const c : curves) {
            addAndMakeVisible(c);
        }
    } else if (type == Modulation::LFO) {
        wave = std::make_unique<WaveformSelector>(bridge, Modulation::lfo_wav(rep));
        wave->set_single(true);
        wave->set_on_click([this] { set_expanded(true); });
        addAndMakeVisible(*wave);

        shape_grid = std::make_unique<WaveformSelector>(bridge, Modulation::lfo_wav(rep));
        shape_grid->set_on_select([this](int) { set_expanded(false); });
        addChildComponent(*shape_grid);

        knobs.add(new Knob(bridge, Modulation::lfo_frq(rep), "RATE"));
        knobs.add(new Knob(bridge, Modulation::lfo_phs(rep), "PHS"));
        knobs.add(new Knob(bridge, Modulation::lfo_dst(rep), "DIST"));
        knobs.add(new Knob(bridge, Modulation::lfo_rnd(rep), "RAND"));
    }

    for (Knob* const k : knobs) {
        addAndMakeVisible(k);
    }
}


int ModulatorCard::preferred_height() const
{
    return 90;
}


void ModulatorCard::set_expanded(bool const expanded)
{
    lfo_expanded = expanded;
    resized();
    repaint();
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
    b.removeFromTop(16);   /* one-line header */

    if (type == Modulation::LFO) {
        if (lfo_expanded) {
            shape_grid->setBounds(b);
            shape_grid->setVisible(true);
            wave->setVisible(false);
            for (Knob* const k : knobs) k->setVisible(false);
            sync_bounds = {};
            return;
        }

        shape_grid->setVisible(false);
        wave->setVisible(true);

        juce::Rectangle<int> left = b.removeFromLeft(40);
        wave->setBounds(left.removeFromTop(b.getHeight() - 16));
        left.removeFromTop(2);
        sync_bounds = left.removeFromTop(14);
        b.removeFromLeft(6);

        int const n = knobs.size();
        int const cell = n > 0 ? b.getWidth() / n : 0;
        for (int i = 0; i != n; ++i) {
            knobs[i]->setVisible(true);
            knobs[i]->setBounds(b.getX() + i * cell, b.getY(), cell, b.getHeight());
        }
        return;
    }

    /* Envelope: one interspersed row -
     * DLY | attack-curve | ATK | HLD | DEC | decay-curve | REL | release-curve. */
    int const cw = 16;
    int const kw = juce::jmax(30, (b.getWidth() - 3 * cw) / 5);
    int const total = 5 * kw + 3 * cw;
    int const h = b.getHeight();
    int x = b.getX() + juce::jmax(0, (b.getWidth() - total) / 2);

    auto place_knob = [&](int const i) {
        knobs[i]->setBounds(x, b.getY(), kw, h);
        x += kw;
    };
    auto place_curve = [&](int const i) {
        curves[i]->setBounds(x + (cw - cw) / 2, b.getY() + (h - cw) / 2, cw, cw);
        x += cw;
    };

    place_knob(0);    /* DLY */
    place_curve(0);   /* attack curve */
    place_knob(1);    /* ATK */
    place_knob(2);    /* HLD */
    place_knob(3);    /* DEC */
    place_curve(1);   /* decay curve */
    place_knob(4);    /* REL */
    place_curve(2);   /* release curve */
}


void ModulatorCard::mouseDown(juce::MouseEvent const& event)
{
    if (type == Modulation::LFO && !lfo_expanded && sync_bounds.contains(event.getPosition())) {
        toggle_sync();
    }
}


void ModulatorCard::paint(juce::Graphics& g)
{
    juce::Rectangle<float> const box = getLocalBounds().toFloat().reduced(1.0f);
    g.setColour(Theme::PANEL_2);
    g.fillRoundedRectangle(box, 4.0f);
    g.setColour(Modulation::colour(type).withAlpha(0.5f));
    g.drawRoundedRectangle(box, 4.0f, 1.0f);

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

    /* LFO tempo-sync toggle (collapsed view only). */
    if (type == Modulation::LFO && !lfo_expanded && !sync_bounds.isEmpty()) {
        bool const on = bridge.get_discrete(Modulation::lfo_syn(rep)) != 0;
        g.setColour(on ? Theme::LFO.withAlpha(0.35f) : Theme::INSET);
        g.fillRoundedRectangle(sync_bounds.toFloat(), 2.0f);
        g.setColour(on ? Theme::LFO : Theme::TEXT_DIM);
        g.setFont(9.0f);
        g.drawText("BPM", sync_bounds, juce::Justification::centred, false);
    }
}

}
