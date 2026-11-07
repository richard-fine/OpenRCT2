/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "VendingMachineElement.h"

#include "../../object/ObjectEntryManager.h"
#include "../../object/PathAdditionEntry.h"

namespace OpenRCT2
{
    ObjectEntryIndex VendingMachineElement::getEntryIndex() const
    {
        return entryIndex;
    }

    void VendingMachineElement::setEntryIndex(ObjectEntryIndex newIndex)
    {
        entryIndex = newIndex;
    }

    const PathAdditionEntry* VendingMachineElement::getEntry() const
    {
        return ObjectEntryManager::GetObjectEntry<PathAdditionEntry>(entryIndex);
    }

    bool VendingMachineElement::isBroken() const
    {
        return (machineFlags & kVendingMachineFlagBroken) != 0;
    }

    void VendingMachineElement::setIsBroken(bool broken)
    {
        machineFlags &= ~kVendingMachineFlagBroken;
        if (broken)
            machineFlags |= kVendingMachineFlagBroken;
    }

    uint16_t VendingMachineElement::getStock() const
    {
        return stock;
    }

    void VendingMachineElement::setStock(uint16_t newStock)
    {
        stock = newStock;
    }

    uint16_t VendingMachineElement::getMachineIndex() const
    {
        return machineIndex;
    }

    void VendingMachineElement::setMachineIndex(uint16_t newIndex)
    {
        machineIndex = newIndex;
    }
} // namespace OpenRCT2
