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

#ifndef JS80P__UI__MODULATION_HPP
#define JS80P__UI__MODULATION_HPP

#include <juce_graphics/juce_graphics.h>

#include "js80p.hpp"
#include "synth.hpp"

#include "ui/theme.hpp"


namespace JS80P
{

/**
 * \brief Helpers mapping the GUI's notion of a "modulator" (a pooled envelope /
 *        LFO / macro slot) to the engine's ControllerId and range parameters.
 *        The engine is untouched; this is pure bookkeeping over its ids.
 */
namespace Modulation
{
    enum Type { ENVELOPE, LFO, MACRO };

    static constexpr int ENVELOPE_COUNT = 12;
    static constexpr int LFO_COUNT = 8;
    static constexpr int MACRO_COUNT = 10;   /* macros 1-10 (7-param stride) */

    /** ControllerId for a 1-based modulator slot, or NONE if out of range. */
    inline Synth::ControllerId controller_id(Type const type, int const index)
    {
        if (type == LFO && index >= 1 && index <= LFO_COUNT) {
            return (Synth::ControllerId)((int)Synth::ControllerId::LFO_1 + index - 1);
        }

        if (type == ENVELOPE && index >= 1 && index <= ENVELOPE_COUNT) {
            /* ENVELOPE_1..6 = 149..154, ENVELOPE_7..12 = 172..177. */
            return index <= 6
                ? (Synth::ControllerId)((int)Synth::ControllerId::ENVELOPE_1 + index - 1)
                : (Synth::ControllerId)((int)Synth::ControllerId::ENVELOPE_7 + index - 7);
        }

        if (type == MACRO && index >= 1 && index <= MACRO_COUNT) {
            return (Synth::ControllerId)((int)Synth::ControllerId::MACRO_1 + index - 1);
        }

        return Synth::ControllerId::NONE;
    }

    /** The (type, 1-based index) a ControllerId denotes, or false if it's not a
     *  pooled modulator (e.g. a raw MIDI CC). */
    inline bool decode(Synth::ControllerId const id, Type& type, int& index)
    {
        int const v = (int)id;

        if (v >= (int)Synth::ControllerId::LFO_1 && v <= (int)Synth::ControllerId::LFO_8) {
            type = LFO; index = v - (int)Synth::ControllerId::LFO_1 + 1; return true;
        }

        if (v >= (int)Synth::ControllerId::ENVELOPE_1 && v <= (int)Synth::ControllerId::ENVELOPE_6) {
            type = ENVELOPE; index = v - (int)Synth::ControllerId::ENVELOPE_1 + 1; return true;
        }

        if (v >= (int)Synth::ControllerId::ENVELOPE_7 && v <= (int)Synth::ControllerId::ENVELOPE_12) {
            type = ENVELOPE; index = v - (int)Synth::ControllerId::ENVELOPE_7 + 7; return true;
        }

        if (v >= (int)Synth::ControllerId::MACRO_1 && v <= (int)Synth::ControllerId::MACRO_10) {
            type = MACRO; index = v - (int)Synth::ControllerId::MACRO_1 + 1; return true;
        }

        return false;
    }

    /* Range parameters for a 1-based modulator slot (verified strides). */
    inline Synth::ParamId lfo_min(int const i)  { return (Synth::ParamId)((int)Synth::ParamId::L1MIN + (i - 1) * 7); }
    inline Synth::ParamId lfo_max(int const i)  { return (Synth::ParamId)((int)Synth::ParamId::L1MAX + (i - 1) * 7); }
    inline Synth::ParamId macro_min(int const i){ return (Synth::ParamId)((int)Synth::ParamId::M1MIN + (i - 1) * 7); }
    inline Synth::ParamId macro_max(int const i){ return (Synth::ParamId)((int)Synth::ParamId::M1MAX + (i - 1) * 7); }
    inline Synth::ParamId env_init(int const i) { return (Synth::ParamId)((int)Synth::ParamId::N1INI + (i - 1) * 12); }
    inline Synth::ParamId env_peak(int const i) { return (Synth::ParamId)((int)Synth::ParamId::N1PK  + (i - 1) * 12); }
    inline Synth::ParamId env_sus(int const i)  { return (Synth::ParamId)((int)Synth::ParamId::N1SUS + (i - 1) * 12); }

    inline juce::Colour colour(Type const type)
    {
        switch (type) {
            case ENVELOPE: return Theme::ENV;
            case LFO:      return Theme::LFO;
            default:       return Theme::MACRO;
        }
    }

    inline char const* prefix(Type const type)
    {
        switch (type) {
            case ENVELOPE: return "E";
            case LFO:      return "L";
            default:       return "M";
        }
    }
}

}

#endif
