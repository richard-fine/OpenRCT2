/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "VendingMachine.h"

#include "Map.h"
#include "TileElementsView.h"
#include "tile_element/VendingMachineElement.h"

namespace OpenRCT2
{
    VendingMachineElement* VendingMachineGetElementAt(const CoordsXYZD& loc, bool isGhost)
    {
        for (auto* machine : TileElementsView<VendingMachineElement>(loc))
        {
            if (machine->getBaseZ() != loc.z)
                continue;
            if (machine->getDirection() != loc.direction)
                continue;
            if (machine->isGhost() != isGhost)
                continue;
            return machine;
        }
        return nullptr;
    }

    bool VendingMachineInTheWay(const CoordsXYRangedZ& loc, Direction edge)
    {
        for (auto* machine : TileElementsView<VendingMachineElement>(loc))
        {
            if (machine->isGhost())
                continue;
            if (machine->getDirection() != edge)
                continue;
            if (loc.baseZ >= machine->getClearanceZ() || loc.clearanceZ <= machine->getBaseZ())
                continue;
            return true;
        }
        return false;
    }

    void VendingMachinesRemoveFromPath(const CoordsXYZ& pathLoc)
    {
        bool removed;
        do
        {
            removed = false;
            for (auto* machine : TileElementsView<VendingMachineElement>(pathLoc))
            {
                if (machine->getBaseZ() != pathLoc.z)
                    continue;
                MapInvalidateTileZoom1({ pathLoc, machine->getBaseZ(), machine->getClearanceZ() });
                TileElementRemove(reinterpret_cast<TileElement*>(machine));
                // Removing shifts the remaining elements, so restart the scan.
                removed = true;
                break;
            }
        } while (removed);
    }
} // namespace OpenRCT2
