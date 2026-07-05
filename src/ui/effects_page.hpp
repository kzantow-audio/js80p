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

#ifndef JS80P__UI__EFFECTS_PAGE_HPP
#define JS80P__UI__EFFECTS_PAGE_HPP

#include <initializer_list>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "js80p.hpp"
#include "synth.hpp"

#include "ui/knob.hpp"
#include "ui/param_bridge.hpp"


namespace JS80P
{

/**
 * \brief The EFFECTS page of the new GUI: recreates the original GUI's effect
 *        chain (input/output volumes, two distortions, two filters, tape,
 *        chorus, echo, reverb) as draw_panel-style boxes of Knobs. Discrete
 *        "type"/mode params are shown as knobs that step through the options;
 *        the old logarithmic-scale toggles are intentionally omitted. Scrolls
 *        vertically via an internal Viewport when the body is short.
 */
class EffectsPage : public juce::Component
{
    public:
        explicit EffectsPage(ParamBridge& bridge);

        void resized() override;

        /** Re-read every knob from the live Synth (called from NewGui's timer). */
        void refresh();

    private:
        struct KnobSpec
        {
            Synth::ParamId id;
            char const* label;
        };

        struct Panel
        {
            juce::String title;
            int cols;
            std::vector<Knob*> knobs;
            juce::Rectangle<int> bounds;
        };

        /* Inner scroll content: draws the panel boxes behind the knobs. */
        class Content : public juce::Component
        {
            public:
                explicit Content(EffectsPage& owner) : owner(owner) {}
                void paint(juce::Graphics& g) override;

            private:
                EffectsPage& owner;
        };

        void add_panel(
            juce::String title,
            int const cols,
            std::initializer_list<KnobSpec> const specs
        );
        void layout();
        void paint_content(juce::Graphics& g);

        ParamBridge& bridge;
        Content content;
        juce::Viewport viewport;
        juce::OwnedArray<Knob> knobs;
        std::vector<Panel> panels;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EffectsPage)
};

}

#endif
