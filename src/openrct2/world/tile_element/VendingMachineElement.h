/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "../../Identifiers.h"
#include "../../object/ObjectTypes.h"
#include "TileElementBase.h"

#include <cstdint>

namespace OpenRCT2
{
    struct PathAdditionEntry;
    struct VendingMachine;

    // Height of a vending machine above the path it stands on.
    constexpr int32_t kVendingMachineClearanceZ = 32;

    constexpr uint8_t kVendingMachineFlagBroken = (1 << 0);

#pragma pack(push, 1)
    /**
     * A vending machine standing on a footpath tile, against one of the path's unconnected edges.
     * The edge is stored as the element's direction. Like walls, vending machines occupy no quadrants.
     */
    struct VendingMachineElement : TileElementBase
    {
        static constexpr TileElementType kElementType = TileElementType::vendingMachine;

    private:
        ObjectEntryIndex entryIndex; // 05 (path addition object)
        uint8_t machineFlags;        // 07
        uint16_t stock;              // 08
        VendingMachineIndex machineIndex; // 0A (price and statistics; null for ghosts)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-private-field"
        uint8_t pad0C[4];
#pragma clang diagnostic pop

    public:
        ObjectEntryIndex getEntryIndex() const;
        void setEntryIndex(ObjectEntryIndex newIndex);
        const PathAdditionEntry* getEntry() const;

        bool isBroken() const;
        void setIsBroken(bool isBroken);

        uint16_t getStock() const;
        void setStock(uint16_t newStock);

        VendingMachineIndex getMachineIndex() const;
        void setMachineIndex(VendingMachineIndex newIndex);
        VendingMachine* getMachine() const;
    };
    static_assert(sizeof(VendingMachineElement) == kTileElementSize);

#pragma pack(pop)
} // namespace OpenRCT2
