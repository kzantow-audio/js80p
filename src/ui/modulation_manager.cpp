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

#include <algorithm>
#include <cmath>

#include "ui/modulation_manager.hpp"


namespace JS80P
{

static int slot_key(Modulation::Type const type, int const index)
{
    return (int)type * 100 + index;
}


static int quant(double const r)
{
    return (int)std::lround(r * 1000.0);
}


std::string ModulationManager::shape_key(
        Modulation::Type const type, int const index
) const {
    std::string key;

    if (type == Modulation::ENVELOPE) {
        int const p[4] = {
            quant(bridge.get_ratio(Modulation::env_hld(index))),
            quant(bridge.get_ratio(Modulation::env_atk(index))),
            quant(bridge.get_ratio(Modulation::env_dec(index))),
            quant(bridge.get_ratio(Modulation::env_rel(index))),
        };
        for (int v : p) { key += std::to_string(v); key += ':'; }
    } else if (type == Modulation::LFO) {
        int p[6] = {
            quant(bridge.get_ratio(Modulation::lfo_wav(index))),
            quant(bridge.get_ratio(Modulation::lfo_frq(index))),
            quant(bridge.get_ratio(Modulation::lfo_phs(index))),
            quant(bridge.get_ratio(Modulation::lfo_dst(index))),
            quant(bridge.get_ratio(Modulation::lfo_rnd(index))),
            Modulation::lfo_has_pw(index) ? quant(bridge.get_ratio(Modulation::lfo_pw(index))) : -1,
        };
        for (int v : p) { key += std::to_string(v); key += ':'; }
    }

    return key;
}


bool ModulationManager::is_used(Modulation::Type const type, int const index) const
{
    if (reserved.count(slot_key(type, index)) != 0) {
        return true;
    }

    for (int pid = 0; pid != (int)Synth::ParamId::PARAM_ID_COUNT; ++pid) {
        Modulation::Type t;
        int i;

        if (Modulation::decode(bridge.controller((Synth::ParamId)pid), t, i)
                && t == type && i == index) {
            return true;
        }
    }

    return false;
}


void ModulationManager::rescan()
{
    /* slot (type,index) -> destinations */
    std::map<int, std::vector<Synth::ParamId>> used;

    for (int pid = 0; pid != (int)Synth::ParamId::PARAM_ID_COUNT; ++pid) {
        Modulation::Type t;
        int i;

        if (Modulation::decode(bridge.controller((Synth::ParamId)pid), t, i)) {
            used[slot_key(t, i)].push_back((Synth::ParamId)pid);
        }
    }

    /* Bucket used slots by (type, shape key). */
    std::map<std::string, Group> buckets;

    for (std::map<int, std::vector<Synth::ParamId>>::const_iterator it = used.begin();
            it != used.end(); ++it) {
        Modulation::Type const type = (Modulation::Type)(it->first / 100);
        int const index = it->first % 100;
        std::string const bkey = std::to_string((int)type) + "|" + shape_key(type, index);

        Group& g = buckets[bkey];
        g.type = type;
        g.members.push_back(index);
        g.destinations.insert(g.destinations.end(), it->second.begin(), it->second.end());
    }

    groups_.clear();

    for (std::map<std::string, Group>::iterator it = buckets.begin();
            it != buckets.end(); ++it) {
        Group& g = it->second;
        std::sort(g.members.begin(), g.members.end());
        g.rep = g.members.empty() ? 0 : g.members.front();
        groups_.push_back(g);
    }
}


int ModulationManager::free_count(Modulation::Type const type) const
{
    int n = 0;

    for (int i = Modulation::pool_first(type); i <= Modulation::pool_last(type); ++i) {
        if (!is_used(type, i)) {
            ++n;
        }
    }

    return n;
}


int ModulationManager::allocate(Modulation::Type const type, bool const need_pw)
{
    /* For an LFO that must carry a pulse width, prefer an odd slot. */
    if (type == Modulation::LFO && need_pw) {
        for (int i = 1; i <= Modulation::LFO_COUNT; ++i) {
            if (Modulation::lfo_has_pw(i) && !is_used(type, i)) {
                return i;
            }
        }
    }

    for (int i = Modulation::pool_first(type); i <= Modulation::pool_last(type); ++i) {
        if (!is_used(type, i)) {
            return i;
        }
    }

    return 0;
}


void ModulationManager::copy_shape(
        Modulation::Type const type, int const from, int const to
) {
    if (type == Modulation::ENVELOPE) {
        int const off[] = { 0, 2, 3, 5, 6, 8 };   /* SCL, DEL, ATK, HLD, DEC, REL */
        for (int o : off) {
            bridge.set_ratio(Modulation::pid((int)Modulation::env_scl(to) + o),
                             bridge.get_ratio(Modulation::pid((int)Modulation::env_scl(from) + o)));
        }
    } else if (type == Modulation::LFO) {
        bridge.set_ratio(Modulation::lfo_wav(to), bridge.get_ratio(Modulation::lfo_wav(from)));
        bridge.set_ratio(Modulation::lfo_frq(to), bridge.get_ratio(Modulation::lfo_frq(from)));
        bridge.set_ratio(Modulation::lfo_phs(to), bridge.get_ratio(Modulation::lfo_phs(from)));
        bridge.set_ratio(Modulation::lfo_dst(to), bridge.get_ratio(Modulation::lfo_dst(from)));
        bridge.set_ratio(Modulation::lfo_rnd(to), bridge.get_ratio(Modulation::lfo_rnd(from)));
        bridge.set_ratio(Modulation::lfo_amp(to), bridge.get_ratio(Modulation::lfo_amp(from)));

        if (Modulation::lfo_has_pw(from) && Modulation::lfo_has_pw(to)) {
            bridge.set_ratio(Modulation::lfo_pw(to), bridge.get_ratio(Modulation::lfo_pw(from)));
        }
    }
}


void ModulationManager::set_base(
        Modulation::Type const type, int const slot, double const base_ratio
) {
    if (type == Modulation::ENVELOPE) {
        bridge.set_ratio(Modulation::env_ini(slot), base_ratio);
        bridge.set_ratio(Modulation::env_sus(slot), base_ratio);
        bridge.set_ratio(Modulation::env_fin(slot), base_ratio);
    } else if (type == Modulation::LFO) {
        bridge.set_ratio(Modulation::lfo_min(slot), base_ratio);
    } else {
        bridge.set_ratio(Modulation::macro_min(slot), base_ratio);
    }
}


int ModulationManager::assign(
        Synth::ParamId const dest,
        Modulation::Type const type,
        double const base_ratio,
        int const clone_from
) {
    bool const need_pw =
        type == Modulation::LFO && clone_from > 0 && Modulation::lfo_has_pw(clone_from);

    int const slot = allocate(type, need_pw);

    if (slot == 0) {
        return 0;   /* pool exhausted */
    }

    if (clone_from > 0) {
        copy_shape(type, clone_from, slot);
    }

    set_base(type, slot, base_ratio);

    /* default depth: reach half-scale above the base */
    double const target = juce::jlimit(0.0, 1.0, base_ratio + 0.5);
    bridge.set_ratio(Modulation::depth_param(type, slot), target);

    bridge.assign_controller(dest, Modulation::controller_id(type, slot));
    reserved.insert(slot_key(type, slot));
    rescan();

    return slot;
}


void ModulationManager::unassign(Synth::ParamId const dest)
{
    Modulation::Type type;
    int index;

    if (Modulation::decode(bridge.controller(dest), type, index)) {
        reserved.erase(slot_key(type, index));
    }

    bridge.assign_controller(dest, Synth::ControllerId::NONE);
    rescan();
}

}
