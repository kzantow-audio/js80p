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

#ifndef JS80P__UI__KNOB_HPP
#define JS80P__UI__KNOB_HPP

#include <juce_gui_basics/juce_gui_basics.h>

#include "js80p.hpp"
#include "synth.hpp"

#include "ui/modulation.hpp"
#include "ui/modulation_manager.hpp"
#include "ui/param_bridge.hpp"


namespace JS80P
{

/**
 * \brief A resolution-independent rotary control bound to one Synth parameter.
 *        When a modulator is assigned, the rotation sets the source's per-
 *        destination base (envelope INI/SUS/FIN, or LFO/macro MIN); a small
 *        filled circle at the top-right sets the signed range/depth (peak/max);
 *        an outer ring shows where the modulation reaches. Right-click assigns.
 */
class Knob : public juce::Component
{
    public:
        Knob(
            ParamBridge& bridge,
            Synth::ParamId const param_id,
            juce::String const& label
        );

        /** Make this knob a modulation destination (right-click to assign). */
        void set_manager(ModulationManager* const manager);

        void set_center_value(double const display_value);
        void set_semitone_snap(bool const on);

        void refresh();

        void paint(juce::Graphics& g) override;
        void mouseDown(juce::MouseEvent const& event) override;
        void mouseDrag(juce::MouseEvent const& event) override;
        void mouseUp(juce::MouseEvent const& event) override;
        void mouseDoubleClick(juce::MouseEvent const& event) override;
        void mouseWheelMove(
            juce::MouseEvent const& event,
            juce::MouseWheelDetails const& wheel
        ) override;

    private:
        static constexpr double DRAG_PIXELS_FULL_RANGE = 220.0;
        static constexpr double WHEEL_STEP = 0.04;

        juce::String format_value() const;
        juce::String format_ratio(double const r) const;
        void commit(double const new_ratio);

        double ratio_to_visual(double const r) const;
        double visual_to_ratio(double const v) const;
        double ratio_for_display(double const target) const;

        void update_assignment();
        void read_base_depth();
        void apply_base(double const b);
        void apply_depth(double const d);
        void open_assign_menu();

        juce::Rectangle<float> knob_circle() const;
        juce::Rectangle<float> badge_rect() const;

        ParamBridge& bridge;
        Synth::ParamId const param_id;
        juce::String const label;
        ModulationManager* manager;

        double ratio;
        double skew;
        double drag_start_visual;
        bool dragging;
        bool dragging_depth;
        double drag_start_depth;
        bool semitone_snap;

        bool assigned;
        Modulation::Type mod_type;
        int mod_slot;
        double base;
        double depth;   /* signed: target = clamp(base + depth) */

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Knob)
};

}

#endif
