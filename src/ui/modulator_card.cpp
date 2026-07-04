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
        ParamBridge& bridge, ModulationManager::Group const& group
) : bridge(bridge),
    type(group.type),
    rep(group.rep),
    members(group.members),
    destinations(group.destinations)
{
    setWantsKeyboardFocus(false);

    if (type == Modulation::ENVELOPE) {
        sliders.add(new VSlider(bridge, Modulation::env_atk(rep), "A"));
        sliders.add(new VSlider(bridge, Modulation::env_hld(rep), "H"));
        sliders.add(new VSlider(bridge, Modulation::env_dec(rep), "D"));
        sliders.add(new VSlider(bridge, Modulation::env_rel(rep), "R"));
    } else if (type == Modulation::LFO) {
        wave = std::make_unique<WaveformSelector>(bridge, Modulation::lfo_wav(rep));
        addAndMakeVisible(*wave);
        sliders.add(new VSlider(bridge, Modulation::lfo_frq(rep), "RATE"));
        sliders.add(new VSlider(bridge, Modulation::lfo_phs(rep), "PHS"));
    }

    for (VSlider* const s : sliders) {
        addAndMakeVisible(s);
    }
}


int ModulatorCard::preferred_height() const
{
    return type == Modulation::LFO ? 118 : 78;
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
    for (VSlider* const s : sliders) {
        s->refresh();
    }

    if (wave != nullptr) {
        wave->refresh();
    }
}


void ModulatorCard::resized()
{
    juce::Rectangle<int> b = getLocalBounds().reduced(6);
    b.removeFromTop(16);   /* one-line header */

    if (wave != nullptr) {
        wave->setBounds(b.removeFromTop(40));
        b.removeFromTop(4);
    }

    int const n = sliders.size();

    if (n > 0) {
        int const cell = b.getWidth() / n;
        for (int i = 0; i != n; ++i) {
            sliders[i]->setBounds(b.getX() + i * cell, b.getY(), cell - 3, b.getHeight());
        }
    }
}


void ModulatorCard::paint(juce::Graphics& g)
{
    juce::Rectangle<float> const box = getLocalBounds().toFloat().reduced(1.0f);
    g.setColour(Theme::PANEL_2);
    g.fillRoundedRectangle(box, 4.0f);
    g.setColour(Modulation::colour(type).withAlpha(0.5f));
    g.drawRoundedRectangle(box, 4.0f, 1.0f);

    /* One-line header: "<rep> (+<others>) Target1 Target2 ..." */
    juce::String slots = juce::String(Modulation::prefix(type)) + juce::String(rep);

    if (members.size() > 1) {
        slots += " (+";
        bool first = true;
        for (int m : members) {
            if (m == rep) {
                continue;
            }
            slots += (first ? "" : " ") + juce::String(m);
            first = false;
        }
        slots += ")";
    }

    juce::Font const hf(juce::FontOptions().withHeight(12.0f).withStyle("Bold"));
    g.setFont(hf);
    g.setColour(Modulation::colour(type));
    g.drawText(slots, 8, 3, getWidth() - 16, 14, juce::Justification::centredLeft, false);

    int const slots_w = (int)juce::GlyphArrangement::getStringWidth(hf, slots) + 2;

    juce::String dests;
    for (Synth::ParamId const d : destinations) {
        dests += juce::String(bridge.param_name(d).c_str()) + " ";
    }

    g.setColour(Theme::TEXT_DIM);
    g.setFont(11.0f);
    g.drawText(dests.trim(), 8 + slots_w + 8, 3, getWidth() - 24 - slots_w, 14,
               juce::Justification::centredLeft, true);
}

}
