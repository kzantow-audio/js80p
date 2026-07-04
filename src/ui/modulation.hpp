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
 * \brief Pure id/colour maps for the pooled modulators (envelope / LFO / macro).
 *        See doc/z-gui.md 5-6. The engine is untouched; these just translate
 *        between the GUI's (type, 1-based slot) and the engine's ParamId /
 *        ControllerId. Strides are irregular, hence the explicit tables.
 */
namespace Modulation
{
    enum Type { ENVELOPE, LFO, MACRO };

    static constexpr int ENVELOPE_COUNT = 12;
    static constexpr int LFO_COUNT = 8;
    /* Macros 1-8 are the top performance strip; 9-30 back auto-modulation. */
    static constexpr int MACRO_POOL_FIRST = 9;
    static constexpr int MACRO_POOL_LAST = 30;

    inline int pool_first(Type const t) { return t == MACRO ? MACRO_POOL_FIRST : 1; }
    inline int pool_last(Type const t)
    {
        return t == ENVELOPE ? ENVELOPE_COUNT : (t == LFO ? LFO_COUNT : MACRO_POOL_LAST);
    }

    inline Synth::ParamId pid(int const v) { return (Synth::ParamId)v; }

    /* ---- ControllerId <-> (type, 1-based slot) ---- */

    inline Synth::ControllerId controller_id(Type const type, int const i)
    {
        if (type == LFO) {
            return (Synth::ControllerId)((int)Synth::ControllerId::LFO_1 + i - 1);
        }

        if (type == ENVELOPE) {
            return i <= 6
                ? (Synth::ControllerId)((int)Synth::ControllerId::ENVELOPE_1 + i - 1)
                : (Synth::ControllerId)((int)Synth::ControllerId::ENVELOPE_7 + i - 7);
        }

        if (i <= 10) return (Synth::ControllerId)((int)Synth::ControllerId::MACRO_1 + i - 1);
        if (i <= 20) return (Synth::ControllerId)((int)Synth::ControllerId::MACRO_11 + i - 11);
        return (Synth::ControllerId)((int)Synth::ControllerId::MACRO_21 + i - 21);
    }

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
        if (v >= (int)Synth::ControllerId::MACRO_11 && v <= (int)Synth::ControllerId::MACRO_20) {
            type = MACRO; index = v - (int)Synth::ControllerId::MACRO_11 + 11; return true;
        }
        if (v >= (int)Synth::ControllerId::MACRO_21 && v <= (int)Synth::ControllerId::MACRO_30) {
            type = MACRO; index = v - (int)Synth::ControllerId::MACRO_21 + 21; return true;
        }
        return false;
    }

    /* ---- Envelope params (stride 12 from N1SCL = 339) ---- */

    inline Synth::ParamId env_scl(int const i) { return pid(339 + 12 * (i - 1)); }
    inline Synth::ParamId env_ini(int const i) { return pid(340 + 12 * (i - 1)); }
    inline Synth::ParamId env_del(int const i) { return pid(341 + 12 * (i - 1)); }
    inline Synth::ParamId env_atk(int const i) { return pid(342 + 12 * (i - 1)); }
    inline Synth::ParamId env_pk (int const i) { return pid(343 + 12 * (i - 1)); }
    inline Synth::ParamId env_hld(int const i) { return pid(344 + 12 * (i - 1)); }
    inline Synth::ParamId env_dec(int const i) { return pid(345 + 12 * (i - 1)); }
    inline Synth::ParamId env_sus(int const i) { return pid(346 + 12 * (i - 1)); }
    inline Synth::ParamId env_rel(int const i) { return pid(347 + 12 * (i - 1)); }
    inline Synth::ParamId env_fin(int const i) { return pid(348 + 12 * (i - 1)); }

    /* Envelope shape curves (stride-1 blocks): attack / decay / release. */
    inline Synth::ParamId env_ash(int const i) { return pid(649 + (i - 1)); }
    inline Synth::ParamId env_dsh(int const i) { return pid(661 + (i - 1)); }
    inline Synth::ParamId env_rsh(int const i) { return pid(673 + (i - 1)); }

    /* ---- LFO params (irregular main block: only odd LFOs have PW) ---- */

    inline int lfo_frq_base(int const i)
    {
        static int const base[9] = { 0, 484, 491, 499, 506, 514, 521, 529, 536 };
        return base[i];
    }

    inline bool lfo_has_pw(int const i) { return (i % 2) == 1; }
    inline Synth::ParamId lfo_pw (int const i) { return pid(lfo_frq_base(i) - 1); }
    inline Synth::ParamId lfo_frq(int const i) { return pid(lfo_frq_base(i)); }
    inline Synth::ParamId lfo_phs(int const i) { return pid(lfo_frq_base(i) + 1); }
    inline Synth::ParamId lfo_min(int const i) { return pid(lfo_frq_base(i) + 2); }
    inline Synth::ParamId lfo_max(int const i) { return pid(lfo_frq_base(i) + 3); }
    inline Synth::ParamId lfo_amp(int const i) { return pid(lfo_frq_base(i) + 4); }
    inline Synth::ParamId lfo_dst(int const i) { return pid(lfo_frq_base(i) + 5); }
    inline Synth::ParamId lfo_rnd(int const i) { return pid(lfo_frq_base(i) + 6); }
    inline Synth::ParamId lfo_wav(int const i) { return pid(555 + (i - 1)); }
    inline Synth::ParamId lfo_log(int const i) { return pid(563 + (i - 1)); }
    inline Synth::ParamId lfo_cen(int const i) { return pid(571 + (i - 1)); }
    inline Synth::ParamId lfo_syn(int const i) { return pid(579 + (i - 1)); }

    /* ---- Macro params (stride 7 from M1MID = 129) ---- */

    inline Synth::ParamId macro_in (int const i) { return pid(130 + 7 * (i - 1)); }
    inline Synth::ParamId macro_min(int const i) { return pid(131 + 7 * (i - 1)); }
    inline Synth::ParamId macro_max(int const i) { return pid(132 + 7 * (i - 1)); }

    /* ---- Per-destination range: base parameter(s) and depth (top) parameter.
     *      base = env INI/SUS/FIN or LFO/macro MIN; depth target = env PK or
     *      LFO/macro MAX. ---- */

    inline Synth::ParamId depth_param(Type const type, int const i)
    {
        return type == ENVELOPE ? env_pk(i) : (type == LFO ? lfo_max(i) : macro_max(i));
    }

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
