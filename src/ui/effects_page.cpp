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

#include "ui/effects_page.hpp"

#include "ui/modulation.hpp"
#include "ui/param_labels.hpp"
#include "ui/theme.hpp"


namespace JS80P
{

/* Two control sizes. Large matches the synth page's 72px cell. Medium is the
 * same knob at ~4/5 the circle size, vertically centred so its circle lines up
 * with the large row. */
static constexpr int LARGE_W = 56;
static constexpr int KNOB_H  = 72;
static constexpr int MED_W   = 48;
static constexpr int MED_H   = 65;
static constexpr int TITLE_H = 22;
static constexpr int PANEL_PAD = 8;
static constexpr int PANEL_GAP = 6;


EffectsPage::EffectsPage(ParamBridge& bridge, ModulationManager& manager)
    : bridge(bridge),
    manager(manager),
    content(*this)
{
    viewport.setViewedComponent(&content, false);
    viewport.setScrollBarsShown(true, false);
    addAndMakeVisible(viewport);

    /* The effect chain, grouped the way the original GUI groups it. Each panel
     * is one left-to-right row of controls in two sizes: large primary knobs
     * and ~4/5-size medium knobs for the related secondary controls, mixed in
     * whatever order reads best. "TYPE" / mode params stay as knobs (they show
     * the option name); the old logarithmic-scale toggles are omitted. */
    using P = Synth::ParamId;

    /* Row 0 - top compact strip: input, the gain stages, the two distortions
     * and the two filters. (The global output volume now lives in the header.) */
    add_large(begin_panel("INPUT", 0), { { P::INVOL, "VOL" } });
    add_large(begin_panel("VOL 1", 0), { { P::EV1V, "VOL" } });

    int const dist1 = begin_panel("DIST 1", 0);
    add_large(dist1, { { P::ED1L, "LEVEL" } });
    add_medium(dist1, { { P::ED1TYP, "TYPE" } });

    int const dist2 = begin_panel("DIST 2", 0);
    add_large(dist2, { { P::ED2L, "LEVEL" } });
    add_medium(dist2, { { P::ED2TYP, "TYPE" } });

    int const filter1 = begin_panel("FILTER 1", 0);
    add_large(filter1, { { P::EF1FRQ, "FREQ" } });
    add_medium(filter1, { { P::EF1TYP, "TYPE" }, { P::EF1Q, "Q" }, { P::EF1G, "GAIN" } });

    int const filter2 = begin_panel("FILTER 2", 0);
    add_large(filter2, { { P::EF2FRQ, "FREQ" } });
    add_medium(filter2, { { P::EF2TYP, "TYPE" }, { P::EF2Q, "Q" }, { P::EF2G, "GAIN" } });

    add_large(begin_panel("VOL 2", 0), { { P::EV2V, "VOL" } });

    /* Row 1 - CHORUS (left) and TAPE (right) share a single row. WIDTH is a
     * large knob kept in its natural place, right after the TYPE selector. */
    int const chorus = begin_panel("CHORUS", 1);
    add_mix(chorus, P::ECWET, P::ECDRY);
    add_large(chorus, { { P::ECFRQ, "FREQ" }, { P::ECDPT, "DEPTH" } });
    add_medium(chorus, { { P::ECDEL, "DELAY" }, { P::ECFB, "FB" }, { P::ECTYP, "TYPE" } });
    add_large(chorus, { { P::ECWID, "WIDTH" } });
    add_medium(chorus, {
        { P::ECDF, "DAMP F" }, { P::ECDG, "DAMP G" },
        { P::ECHPF, "HPF" }, { P::ECHPQ, "HP Q" }
    });
    /* Tempo-sync toggle for the LFO rate, centred over the FREQ knob. */
    add_button(chorus, P::ECSYN, "BPM", P::ECFRQ);

    /* TAPE: STOP is a large primary knob and sits last in the row. */
    int const tape = begin_panel("TAPE", 1);
    add_large(tape, { { P::ETWFA, "WOW" }, { P::ETSAT, "SAT" } });
    add_medium(tape, {
        { P::ETWFS, "SPEED" }, { P::ETCLR, "COLOR" },
        { P::ETSTR, "STEREO" }, { P::ETSTYP, "TYPE" }, { P::ETHSS, "HISS" }
    });
    add_large(tape, { { P::ETSTP, "STOP" } });
    add_button(tape, P::ETEND, "PRE FX")
        ->set_option_labels({ "PRE FX", "POST FX" });

    /* Row 2 - ECHO. REV 1 / REV 2 / SC MODE move to title buttons. WIDTH is a
     * large knob kept right after DIST. */
    int const echo = begin_panel("ECHO", 2);
    add_mix(echo, P::EEWET, P::EEDRY);
    add_large(echo, { { P::EEDEL, "DELAY" }, { P::EEFB, "FB" } });
    add_medium(echo, { { P::EEINV, "IN" }, { P::EEDST, "DIST" } });
    add_large(echo, { { P::EEWID, "WIDTH" } });
    add_medium(echo, {
        { P::EEDF, "DAMP F" }, { P::EEDG, "DAMP G" },
        { P::EEHPF, "HPF" }, { P::EEHPQ, "HP Q" },
        { P::EECTH, "SC TH" }, { P::EECAT, "SC AT" },
        { P::EECRL, "SC RL" }, { P::EECR, "SC R" }
    });
    /* Tempo-sync toggle for the DELAY time, centred over the DELAY knob. */
    add_button(echo, P::EESYN, "BPM", P::EEDEL);
    /* REV 1 sits over the FB knob, with REV 2 laid out just to its right. */
    add_button(echo, P::EER1, "REV 1", P::EEFB);
    add_button_trailing(echo, P::EER2, "REV 2");
    /* Side-chain mode toggle, centred over the side-chain group's lead knob. */
    add_button(echo, P::EECM, "SC", P::EECTH)->set_option_labels({ "COMP", "EXPD" });

    /* Row 3 - REVERB. WIDTH is a large knob kept right after DIST. */
    int const reverb = begin_panel("REVERB", 3);
    add_mix(reverb, P::ERWET, P::ERDRY);
    add_large(reverb, { { P::ERRS, "SIZE" }, { P::ERRR, "REFL" } });
    add_medium(reverb, { { P::ERTYP, "TYPE" }, { P::ERDST, "DIST" } });
    add_large(reverb, { { P::ERWID, "WIDTH" } });
    add_medium(reverb, {
        { P::ERCM, "SC MODE" },
        { P::ERDF, "DAMP F" }, { P::ERDG, "DAMP G" },
        { P::ERHPF, "HPF" }, { P::ERHPQ, "HP Q" },
        { P::ERCTH, "SC TH" }, { P::ERCAT, "SC AT" },
        { P::ERCRL, "SC RL" }, { P::ERCR, "SC R" }
    });
}


int EffectsPage::begin_panel(juce::String title, int const row)
{
    Panel panel;
    panel.title = std::move(title);
    panel.row = row;
    panels.push_back(std::move(panel));
    return (int)panels.size() - 1;
}


Knob* EffectsPage::make_knob(KnobSpec const& spec, bool const medium)
{
    using P = Synth::ParamId;

    Knob* const knob = new Knob(bridge, spec.id, spec.label);

    /* Continuous effect knobs are macro-modulation destinations; the discrete
     * "type" / "mode" selectors are not. */
    if (!bridge.is_discrete(spec.id)) {
        knob->set_manager(&manager);
        knob->set_mod_caps(Modulation::CAP_MACRO);
    }

    /* Frequency cutoffs (filters, damping, HPF): an exponential 20 Hz .. 20 kHz
     * sweep with 1.5 kHz at mid-travel, instead of the parameter's full native
     * range. */
    switch (spec.id) {
        case P::EF1FRQ: case P::EF2FRQ:
        case P::ECDF: case P::ECHPF:
        case P::EEDF: case P::EEHPF:
        case P::ERDF: case P::ERHPF:
            knob->set_freq_range(20.0, 20000.0, 1500.0);
            break;

        /* Chorus LFO rate: exponential, 3 Hz at mid-travel. */
        case P::ECFRQ:
            knob->set_center_value(3.0);
            break;

        default:
            break;
    }

    /* Show names instead of indices for the type / mode selectors. */
    if (spec.id == P::EF1TYP || spec.id == P::EF2TYP) {
        knob->set_discrete_labels(filter_type_labels());
    } else if (spec.id == P::ED1TYP || spec.id == P::ED2TYP
            || spec.id == P::ETSTYP) {
        knob->set_discrete_labels(distortion_type_labels());
    } else if (spec.id == P::ERCM) {
        knob->set_discrete_labels({ "COMP", "EXPD" });
    }

    if (medium) {
        knob->set_compact(true);
    }

    knobs.add(knob);
    content.addAndMakeVisible(knob);
    return knob;
}


void EffectsPage::add_large(
        int const panel, std::initializer_list<KnobSpec> const specs
) {
    for (KnobSpec const& spec : specs) {
        Cell cell;
        cell.knob = make_knob(spec, false);
        cell.id = spec.id;
        panels[(size_t)panel].cells.push_back(cell);
    }
}


void EffectsPage::add_medium(
        int const panel, std::initializer_list<KnobSpec> const specs
) {
    for (KnobSpec const& spec : specs) {
        Cell cell;
        cell.knob = make_knob(spec, true);
        cell.medium = true;
        cell.id = spec.id;
        panels[(size_t)panel].cells.push_back(cell);
    }
}


void EffectsPage::add_mix(
        int const panel, Synth::ParamId const wet, Synth::ParamId const dry
) {
    /* One rotary folding an effect's WET and DRY volumes into a single MIX:
     * mid-travel = both full; clockwise holds WET at 100% and fades DRY; counter-
     * clockwise holds DRY at 100% and fades WET. Driven through value hooks so it
     * inherits the standard drag / reset / typed-entry behaviour. */
    ParamBridge* const bp = &bridge;

    auto const read_mix = [bp, wet, dry]() -> double {
        double const w = bp->get_ratio(wet);
        double const d = bp->get_ratio(dry);
        return w >= d ? juce::jlimit(0.5, 1.0, 1.0 - d * 0.5)
                      : juce::jlimit(0.0, 0.5, w * 0.5);
    };
    auto const write_mix = [bp, wet, dry](double const m) {
        bp->set_ratio(wet, juce::jmin(1.0, 2.0 * m));
        bp->set_ratio(dry, juce::jmin(1.0, 2.0 * (1.0 - m)));
    };
    auto const format_mix = [](double const m) {
        int const wet_pct = (int)std::lround(juce::jmin(1.0, 2.0 * m) * 100.0);
        int const dry_pct = (int)std::lround(juce::jmin(1.0, 2.0 * (1.0 - m)) * 100.0);
        return juce::String(wet_pct) + "/" + juce::String(dry_pct);
    };

    Control* const mix = new Control(bridge, wet, "MIX");
    mix->set_value_hooks(read_mix, write_mix, format_mix);
    mix->set_hook_default(0.5);
    mix_knobs.add(mix);
    content.addAndMakeVisible(mix);

    Cell cell;
    cell.mix = mix;
    panels[(size_t)panel].cells.push_back(cell);
}


MiniButton* EffectsPage::add_button(
        int const panel, Synth::ParamId const id, juce::String label,
        Synth::ParamId const anchor
) {
    MiniButton* const button = new MiniButton(bridge, id, std::move(label));
    buttons.add(button);
    content.addAndMakeVisible(button);

    if (anchor != Synth::ParamId::PARAM_ID_COUNT) {
        panels[(size_t)panel].anchored.push_back({ button, anchor, {} });
    } else {
        panels[(size_t)panel].buttons.push_back(button);
    }

    return button;
}


MiniButton* EffectsPage::add_button_trailing(
        int const panel, Synth::ParamId const id, juce::String label
) {
    MiniButton* const button = new MiniButton(bridge, id, std::move(label));
    buttons.add(button);
    content.addAndMakeVisible(button);
    panels[(size_t)panel].anchored.back().trailing.push_back(button);
    return button;
}


void EffectsPage::resized()
{
    viewport.setBounds(getLocalBounds());
    layout();
}


juce::Point<int> EffectsPage::panel_size(Panel const& p) const
{
    int inner = 0;
    for (Cell const& c : p.cells) {
        inner += c.medium ? MED_W : LARGE_W;
    }

    return juce::Point<int>(
        inner + 2 * PANEL_PAD, TITLE_H + KNOB_H + PANEL_PAD
    );
}


void EffectsPage::place_panel(Panel& panel)
{
    int const inner_y = panel.bounds.getY() + TITLE_H;
    int const med_y = inner_y + (KNOB_H - MED_H) / 2;
    int x = panel.bounds.getX() + PANEL_PAD;

    /* One left-to-right row. Large cells (primary knobs, the MIX cell) are full
     * height; medium cells are shorter and vertically centred so their circles
     * line up with the large row. */
    for (Cell const& cell : panel.cells) {
        if (cell.medium) {
            cell.knob->setBounds(x, med_y, MED_W, MED_H);
            x += MED_W;
        } else if (cell.mix != nullptr) {
            cell.mix->setBounds(x, inner_y, LARGE_W, KNOB_H);
            x += LARGE_W;
        } else {
            cell.knob->setBounds(x, inner_y, LARGE_W, KNOB_H);
            x += LARGE_W;
        }
    }

    /* Title-bar buttons, vertically centred on the title text. The title is
     * drawn in a 16px band starting PANEL pad-6 below the panel top (see
     * paint_content), so its centre sits at getY()+14. */
    int const bh = 14;
    int const by = panel.bounds.getY() + 14 - bh / 2;

    /* Anchored title buttons: horizontally centred over the knob they affect
     * (e.g. the CHORUS/ECHO BPM toggle over FREQ / DELAY). */
    for (AnchoredButton const& ab : panel.anchored) {
        int const bw = ab.button->preferred_width();
        int cx = panel.bounds.getX() + PANEL_PAD + LARGE_W / 2;   /* fallback */

        for (Cell const& cell : panel.cells) {
            if (cell.knob != nullptr && cell.id == ab.anchor) {
                cx = cell.knob->getBounds().getCentreX();
                break;
            }
        }

        ab.button->setBounds(cx - bw / 2, by, bw, bh);

        /* Any trailing buttons flow to the right of the anchored one. */
        int tx = cx - bw / 2 + bw + 4;
        for (MiniButton* const t : ab.trailing) {
            int const tw = t->preferred_width();
            t->setBounds(tx, by, tw, bh);
            tx += tw + 4;
        }
    }

    /* Title-bar buttons, right-aligned and laid out left-to-right in order. */
    int bx = panel.bounds.getRight() - PANEL_PAD;

    for (int i = (int)panel.buttons.size() - 1; i >= 0; --i) {
        int const bw = panel.buttons[(size_t)i]->preferred_width();
        bx -= bw;
        panel.buttons[(size_t)i]->setBounds(bx, by, bw, bh);
        bx -= 4;
    }
}


void EffectsPage::layout()
{
    int const avail = juce::jmax(
        1, viewport.getWidth() - viewport.getScrollBarThickness()
    );

    int max_row = 0;
    for (Panel const& panel : panels) {
        max_row = juce::jmax(max_row, panel.row);
    }

    int y = 0;
    int content_w = 0;

    for (int row = 0; row <= max_row; ++row) {
        int x = 0;
        int row_h = 0;

        for (Panel& panel : panels) {
            if (panel.row != row) {
                continue;
            }

            juce::Point<int> const sz = panel_size(panel);
            panel.bounds = juce::Rectangle<int>(x, y, sz.x, sz.y);
            place_panel(panel);

            x += sz.x + PANEL_GAP;
            row_h = juce::jmax(row_h, sz.y);
        }

        content_w = juce::jmax(content_w, x - PANEL_GAP);
        y += row_h + PANEL_GAP;
    }

    content.setSize(juce::jmax(avail, content_w), y + 2);
    content.repaint();
}


void EffectsPage::paint_content(juce::Graphics& g)
{
    g.setFont(juce::Font(juce::FontOptions().withHeight(13.0f).withStyle("Bold")));

    for (Panel const& panel : panels) {
        juce::Rectangle<float> const r = panel.bounds.toFloat();
        g.setColour(Theme::PANEL);
        g.fillRoundedRectangle(r, Theme::RADIUS);
        g.setColour(Theme::EDGE);
        g.drawRoundedRectangle(r.reduced(0.5f), Theme::RADIUS, 1.0f);

        g.setColour(Theme::TEXT_DIM);
        g.setFont(juce::Font(juce::FontOptions().withHeight(13.0f).withStyle("Bold")));
        g.drawText(
            panel.title,
            panel.bounds.reduced(10, 6).removeFromTop(16),
            juce::Justification::centredLeft,
            false
        );
    }
}


void EffectsPage::Content::paint(juce::Graphics& g)
{
    owner.paint_content(g);
}


void EffectsPage::refresh()
{
    for (Knob* const knob : knobs) {
        knob->refresh();
    }

    for (Control* const mix : mix_knobs) {
        mix->refresh();
    }

    for (MiniButton* const button : buttons) {
        button->refresh();
    }
}

}
