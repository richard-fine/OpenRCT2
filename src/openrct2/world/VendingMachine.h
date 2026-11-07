/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "Location.hpp"

namespace OpenRCT2
{
    struct VendingMachineElement;

    /**
     * Gets the vending machine standing on the path at exactly loc.z, against edge loc.direction.
     */
    VendingMachineElement* VendingMachineGetElementAt(const CoordsXYZD& loc, bool isGhost);

    /**
     * Whether a (non-ghost) vending machine overlapping the given height range stands against the given edge.
     */
    bool VendingMachineInTheWay(const CoordsXYRangedZ& loc, Direction edge);

    /**
     * Removes every vending machine standing on the path at pathLoc (machines share the path's base height).
     */
    void VendingMachinesRemoveFromPath(const CoordsXYZ& pathLoc);
} // namespace OpenRCT2
