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

#ifndef JS80P__UI__MODULATOR_ALLOCATOR_HPP
#define JS80P__UI__MODULATOR_ALLOCATOR_HPP

#include <map>

#include "js80p.hpp"
#include "synth.hpp"

#include "ui/modulation.hpp"
#include "ui/param_bridge.hpp"


namespace JS80P
{

/**
 * \brief Hands out free envelope / LFO / macro slots from the finite pools.
 *        A slot counts as used if any parameter's controller points at it (the
 *        engine's own state, scanned via the atomic getters) or if the GUI has
 *        reserved it locally (so assignments made this session are honoured
 *        immediately, before the audio thread applies them).
 *
 *        Copy-on-write sharing/cloning is a later refinement; for now each
 *        assignment takes a fresh free slot.
 */
class ModulatorAllocator
{
    public:
        explicit ModulatorAllocator(ParamBridge& bridge) noexcept
            : bridge(bridge)
        {
        }

        /** A free 1-based slot index of \c type, or 0 if the pool is full. */
        int allocate(Modulation::Type const type)
        {
            bool used[64] = { false };
            scan_used(type, used);

            int const count = pool_count(type);

            for (int i = 1; i <= count; ++i) {
                if (!used[i]) {
                    reserved[key(type, i)] = true;
                    return i;
                }
            }

            return 0;
        }

        void release(Modulation::Type const type, int const index)
        {
            reserved.erase(key(type, index));
        }

        int free_count(Modulation::Type const type)
        {
            bool used[64] = { false };
            scan_used(type, used);

            int const count = pool_count(type);
            int free = 0;

            for (int i = 1; i <= count; ++i) {
                if (!used[i]) {
                    ++free;
                }
            }

            return free;
        }

        static int pool_count(Modulation::Type const type)
        {
            switch (type) {
                case Modulation::ENVELOPE: return Modulation::ENVELOPE_COUNT;
                case Modulation::LFO:      return Modulation::LFO_COUNT;
                default:                   return Modulation::MACRO_COUNT;
            }
        }

    private:
        static int key(Modulation::Type const type, int const index)
        {
            return (int)type * 100 + index;
        }

        void scan_used(Modulation::Type const type, bool* const used)
        {
            for (int pid = 0; pid != (int)Synth::ParamId::PARAM_ID_COUNT; ++pid) {
                Modulation::Type t;
                int i;

                if (
                    Modulation::decode(
                        bridge.controller((Synth::ParamId)pid), t, i
                    )
                    && t == type && i >= 1 && i < 64
                ) {
                    used[i] = true;
                }
            }

            for (std::map<int, bool>::const_iterator it = reserved.begin();
                    it != reserved.end(); ++it) {
                if (it->second && it->first / 100 == (int)type) {
                    int const i = it->first % 100;

                    if (i >= 1 && i < 64) {
                        used[i] = true;
                    }
                }
            }
        }

        ParamBridge& bridge;
        std::map<int, bool> reserved;
};

}

#endif
