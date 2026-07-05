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

#include "ui/effects_page.hpp"

#include "ui/modulation.hpp"
#include "ui/param_labels.hpp"
#include "ui/theme.hpp"


namespace JS80P
{

/* Knob cell size (matches the synth page's 72px cell height). */
static constexpr int KNOB_W = 56;
static constexpr int KNOB_H = 72;
static constexpr int TITLE_H = 22;
static constexpr int PANEL_PAD = 8;
static constexpr int PANEL_GAP = 8;


EffectsPage::EffectsPage(ParamBridge& bridge, ModulationManager& manager)
    : bridge(bridge),
    manager(manager),
    content(*this)
{
    viewport.setViewedComponent(&content, false);
    viewport.setScrollBarsShown(true, false);
    addAndMakeVisible(viewport);

    /* The effect chain, grouped the way the original GUI groups it. "TYPE" /
     * mode / reversed params (originally discrete selectors or toggles) are
     * shown here as knobs that step through their options. The logarithmic-
     * scale toggles from the old GUI are intentionally omitted. */
    using P = Synth::ParamId;

    /* Top line: input, the three gain stages, the two distortions, the two
     * filters and the output volume (VOL 3, renamed OUT). */
    add_panel("INPUT", 1, { { P::INVOL, "VOL" } });
    add_panel("VOL 1", 1, { { P::EV1V, "VOL" } });

    add_panel("DIST 1", 2, {
        { P::ED1L, "LEVEL" }, { P::ED1TYP, "TYPE" }
    });

    add_panel("DIST 2", 2, {
        { P::ED2L, "LEVEL" }, { P::ED2TYP, "TYPE" }
    });

    add_panel("FILTER 1", 4, {
        { P::EF1TYP, "TYPE" }, { P::EF1FRQ, "FREQ" },
        { P::EF1Q, "Q" }, { P::EF1G, "GAIN" }
    });

    add_panel("FILTER 2", 4, {
        { P::EF2TYP, "TYPE" }, { P::EF2FRQ, "FREQ" },
        { P::EF2Q, "Q" }, { P::EF2G, "GAIN" }
    });

    add_panel("VOL 2", 1, { { P::EV2V, "VOL" } });
    add_panel("OUT", 1, { { P::EV3V, "VOL" } });

    /* Each of the big multi-knob effects gets its own full-width row so it fits
     * on a single line. cols == knob count keeps them one row tall. */
    add_panel("TAPE", 9, {
        { P::ETSTP, "STOP" }, { P::ETWFA, "WOW" }, { P::ETWFS, "SPEED" },
        { P::ETSAT, "SAT" }, { P::ETCLR, "COLOR" }, { P::ETHSS, "HISS" },
        { P::ETSTR, "STEREO" }, { P::ETSTYP, "TYPE" }, { P::ETEND, "END" }
    }, true);

    add_panel("CHORUS", 12, {
        { P::ECTYP, "TYPE" }, { P::ECDEL, "DELAY" }, { P::ECFRQ, "FREQ" },
        { P::ECDPT, "DEPTH" }, { P::ECFB, "FB" }, { P::ECDF, "DAMP F" },
        { P::ECDG, "DAMP G" }, { P::ECWID, "WIDTH" }, { P::ECHPF, "HPF" },
        { P::ECHPQ, "HP Q" }, { P::ECWET, "WET" }, { P::ECDRY, "DRY" }
    }, true);

    add_panel("ECHO", 18, {
        { P::EEINV, "IN" }, { P::EEDEL, "DELAY" }, { P::EEFB, "FB" },
        { P::EEDST, "DIST" }, { P::EEDF, "DAMP F" }, { P::EEDG, "DAMP G" },
        { P::EEWID, "WIDTH" }, { P::EEHPF, "HPF" }, { P::EEHPQ, "HP Q" },
        { P::EECTH, "SC TH" }, { P::EECAT, "SC AT" }, { P::EECRL, "SC RL" },
        { P::EECR, "SC R" }, { P::EECM, "SC MODE" }, { P::EER1, "REV 1" },
        { P::EER2, "REV 2" }, { P::EEWET, "WET" }, { P::EEDRY, "DRY" }
    }, true);

    add_panel("REVERB", 16, {
        { P::ERTYP, "TYPE" }, { P::ERRS, "SIZE" }, { P::ERRR, "REFL" },
        { P::ERDST, "DIST" }, { P::ERDF, "DAMP F" }, { P::ERDG, "DAMP G" },
        { P::ERWID, "WIDTH" }, { P::ERHPF, "HPF" }, { P::ERHPQ, "HP Q" },
        { P::ERCTH, "SC TH" }, { P::ERCAT, "SC AT" }, { P::ERCRL, "SC RL" },
        { P::ERCR, "SC R" }, { P::ERCM, "SC MODE" }, { P::ERWET, "WET" },
        { P::ERDRY, "DRY" }
    }, true);
}


void EffectsPage::add_panel(
        juce::String title,
        int const cols,
        std::initializer_list<KnobSpec> const specs,
        bool const full_row
) {
    using P = Synth::ParamId;

    Panel panel;
    panel.title = std::move(title);
    panel.cols = cols;
    panel.full_row = full_row;

    for (KnobSpec const& spec : specs) {
        Knob* const knob = new Knob(bridge, spec.id, spec.label);

        /* Continuous effect knobs are macro-modulation destinations; the
         * discrete "type" / "mode" selectors are not. */
        if (!bridge.is_discrete(spec.id)) {
            knob->set_manager(&manager);
            knob->set_mod_caps(Modulation::CAP_MACRO);
        }

        /* Filter cutoffs use the same exponential scaling as the oscillator
         * filters: 1 kHz at mid-travel. */
        if (spec.id == P::EF1FRQ || spec.id == P::EF2FRQ) {
            knob->set_center_value(1000.0);
        }

        /* Show names instead of indices for the type selectors. */
        if (spec.id == P::EF1TYP || spec.id == P::EF2TYP) {
            knob->set_discrete_labels(filter_type_labels());
        } else if (spec.id == P::ED1TYP || spec.id == P::ED2TYP) {
            knob->set_discrete_labels(distortion_type_labels());
        }

        knobs.add(knob);
        content.addAndMakeVisible(knob);
        panel.knobs.push_back(knob);
    }

    panels.push_back(std::move(panel));
}


void EffectsPage::resized()
{
    viewport.setBounds(getLocalBounds());
    layout();
}


void EffectsPage::place_knobs(Panel& panel)
{
    int const n = (int)panel.knobs.size();
    int const inner_x = panel.bounds.getX() + PANEL_PAD;
    int const inner_y = panel.bounds.getY() + TITLE_H;

    for (int i = 0; i != n; ++i) {
        panel.knobs[(size_t)i]->setBounds(
            inner_x + (i % panel.cols) * KNOB_W,
            inner_y + (i / panel.cols) * KNOB_H,
            KNOB_W,
            KNOB_H
        );
    }
}


void EffectsPage::layout()
{
    int const avail = juce::jmax(
        1, viewport.getWidth() - viewport.getScrollBarThickness()
    );

    auto panel_size = [](Panel const& p) {
        int const n = (int)p.knobs.size();
        int const rows = (n + p.cols - 1) / p.cols;
        return juce::Point<int>(
            p.cols * KNOB_W + 2 * PANEL_PAD, TITLE_H + rows * KNOB_H + PANEL_PAD
        );
    };

    int x = 0;
    int y = 0;
    int top_h = 0;
    int content_w = 0;

    /* Top line: all the compact (non-full-row) panels side by side. */
    for (Panel& panel : panels) {
        if (panel.full_row) {
            continue;
        }

        juce::Point<int> const sz = panel_size(panel);
        panel.bounds = juce::Rectangle<int>(x, y, sz.x, sz.y);
        place_knobs(panel);

        x += sz.x + PANEL_GAP;
        top_h = juce::jmax(top_h, sz.y);
    }

    content_w = juce::jmax(content_w, x - PANEL_GAP);
    y += top_h + PANEL_GAP;

    /* Each big effect on its own full-width row. */
    for (Panel& panel : panels) {
        if (!panel.full_row) {
            continue;
        }

        juce::Point<int> const sz = panel_size(panel);
        panel.bounds = juce::Rectangle<int>(0, y, sz.x, sz.y);
        place_knobs(panel);

        content_w = juce::jmax(content_w, sz.x);
        y += sz.y + PANEL_GAP;
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
}

}
