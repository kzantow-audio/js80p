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

#ifndef JS80P__UI__MODULATION_MANAGER_HPP
#define JS80P__UI__MODULATION_MANAGER_HPP

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "js80p.hpp"
#include "synth.hpp"

#include "ui/modulation.hpp"
#include "ui/param_bridge.hpp"


namespace JS80P
{

/**
 * \brief Shape-grouping allocator over the flat modulator pools. Tracks which
 *        pool slot backs which destination, buckets slots that share a shape
 *        into groups (one on-screen editor each), allocates/clones fresh slots
 *        on assign, and keeps a group's shape in sync across its duplicates.
 *        See doc/z-gui.md 5-6. The engine is never modified.
 */
class ModulationManager
{
    public:
        struct Group {
            Modulation::Type type;
            int rep;                                  /* representative slot */
            std::vector<int> members;                 /* slots sharing the shape */
            std::vector<Synth::ParamId> destinations; /* params driven */
            /* one (slot, destination) per connection, ordered by slot */
            std::vector<std::pair<int, Synth::ParamId>> connections;
        };

        explicit ModulationManager(ParamBridge& bridge) noexcept
            : bridge(bridge)
        {
        }

        /** Re-read the engine's assignments and rebuild the group list. */
        void rescan();

        std::vector<Group> const& groups() const { return groups_; }

        int free_count(Modulation::Type const type) const;

        /**
         * \brief Assign a modulator to \c dest. If \c clone_from > 0 the shape is
         *        copied from that existing slot; otherwise a fresh default is
         *        used. The new slot's base range is set to \c base_ratio.
         *        Returns the slot index, or 0 if the pool is exhausted.
         */
        int assign(
            Synth::ParamId const dest,
            Modulation::Type const type,
            double const base_ratio,
            int const clone_from = 0
        );

        void unassign(Synth::ParamId const dest);

    private:
        std::string shape_key(Modulation::Type const type, int const index) const;
        int allocate(Modulation::Type const type, bool const need_pw);
        void copy_shape(Modulation::Type const type, int const from, int const to);
        void set_base(Modulation::Type const type, int const slot, double const base_ratio);

        bool is_used(Modulation::Type const type, int const index) const;

        ParamBridge& bridge;
        std::vector<Group> groups_;
        std::set<int> reserved;   /* type*100 + slot; local, pre-audio-thread */
};

}

#endif
