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

#ifndef JS80P__UI__ENV_SCALE_SLIDER_HPP
#define JS80P__UI__ENV_SCALE_SLIDER_HPP

#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "js80p.hpp"
#include "synth.hpp"

#include "ui/modulation.hpp"
#include "ui/modulation_manager.hpp"
#include "ui/param_bridge.hpp"


namespace JS80P
{

/**
 * \brief A compact horizontal slider for an ENVELOPE group's SCALE (SCL)
 *        parameter, meant to sit in the middle of a ModulatorCard's header row.
 *        It mirrors the Knob modulation model (base + signed depth via an
 *        intermediate macro), but envelope SCL only accepts macro sources
 *        (Modulation::CAP_MACRO). A small rectangular badge on the right shows
 *        the assigned source (label + colour) and drags the modulation amount;
 *        right-click opens the macro-only assign menu. Unassigned, the slider
 *        writes SCL directly to every group member; assigned, it drives the
 *        intermediate macro's min/max, mirrored onto every member's SCL.
 */
class EnvScaleSlider : public juce::Component
{
    public:
        EnvScaleSlider(
            ParamBridge& bridge,
            ModulationManager& manager,
            int const rep,
            std::vector<int> const& members
        );

        void refresh();

        void paint(juce::Graphics& g) override;
        void mouseDown(juce::MouseEvent const& event) override;
        void mouseDrag(juce::MouseEvent const& event) override;
        void mouseUp(juce::MouseEvent const& event) override;
        void mouseDoubleClick(juce::MouseEvent const& event) override;

    private:
        static constexpr double DRAG_PIXELS_FULL_RANGE = 220.0;

        juce::Rectangle<float> track_rect() const;
        juce::Rectangle<float> badge_rect() const;

        bool has_source() const;

        void update_assignment();
        void read_base_depth();
        void write_base(double const b);   /* unassigned: SCL; assigned: macro min */
        void apply_depth(double const d);
        void assign_mirrors(int const macro_slot);
        void clear_mirrors();
        void open_assign_menu();
        juce::String format_value(double const ratio) const;

        ParamBridge& bridge;
        ModulationManager& manager;
        Synth::ParamId const scl_param;   /* env_scl(rep) */
        std::vector<Synth::ParamId> mirrors;   /* env_scl of non-rep members */

        double ratio;   /* unassigned live SCL value */

        bool assigned;
        Modulation::Type mod_type;
        int mod_slot;
        juce::String mod_label;
        juce::Colour mod_colour;
        double base;
        double depth;   /* signed: target = clamp(base + depth) */

        bool dragging;
        bool dragging_depth;
        double drag_start_base;
        double drag_start_depth;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnvScaleSlider)
};

}

#endif
