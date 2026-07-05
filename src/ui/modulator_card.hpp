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

#ifndef JS80P__UI__MODULATOR_CARD_HPP
#define JS80P__UI__MODULATOR_CARD_HPP

#include <memory>
#include <utility>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "js80p.hpp"
#include "synth.hpp"

#include "ui/curve_selector.hpp"
#include "ui/env_scale_slider.hpp"
#include "ui/knob.hpp"
#include "ui/modulation.hpp"
#include "ui/modulation_manager.hpp"
#include "ui/param_bridge.hpp"
#include "ui/vfader.hpp"
#include "ui/waveform_selector.hpp"


namespace JS80P
{

/**
 * \brief One editor for a group of pooled slots sharing a shape (see
 *        doc/z-gui.md 5.2): an envelope's A/H/D/R or an LFO's waveform + rate,
 *        edited once and propagated to every duplicate slot in the group, with
 *        the group's destinations shown as badges.
 */
class ModulatorCard : public juce::Component
{
    public:
        ModulatorCard(
            ParamBridge& bridge, ModulationManager& manager,
            ModulationManager::Group const& group
        );

        /** LFO cards are taller to fit the 2-row waveform button group. */
        int preferred_height() const;

        /** Copy the representative slot's shape to every group member. */
        void propagate();
        void refresh();

        void paint(juce::Graphics& g) override;
        void resized() override;
        void mouseDown(juce::MouseEvent const& event) override;

    private:
        void set_expanded(bool const expanded);
        void toggle_sync();
        void write_sustain();   /* SUS = INI + fraction*(PK-INI) per member */

        ParamBridge& bridge;
        ModulationManager& manager;
        Modulation::Type type;
        int rep;
        std::vector<int> members;
        std::vector<Synth::ParamId> destinations;
        std::vector<std::pair<int, Synth::ParamId>> connections;

        juce::OwnedArray<Knob> knobs;        /* env D/A/H/D/R or LFO rate/phase/... */
        juce::OwnedArray<CurveSelector> curves;   /* env attack/decay/release shapes */
        std::unique_ptr<VFader> sus_fader;   /* env sustain fraction (min..max) */
        std::unique_ptr<EnvScaleSlider> env_scale;   /* env SCL, in the header row */
        double sus_fraction;
        std::unique_ptr<WaveformSelector> wave;        /* LFO shape button */
        std::unique_ptr<WaveformSelector> shape_grid;  /* LFO shape picker */
        bool lfo_expanded;
        juce::Rectangle<int> sync_bounds;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulatorCard)
};

}

#endif
