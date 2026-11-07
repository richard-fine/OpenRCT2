/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "../Identifiers.h"
#include "../core/Money.hpp"
#include "../object/ObjectTypes.h"
#include "Location.hpp"

#include <cstddef>
#include <cstdint>

enum class ShopItem : uint8_t;

namespace OpenRCT2
{
    struct GameState_t;
    struct TileElement;
    struct VendingMachineElement;

    constexpr size_t kMaxVendingMachines = 8192;

    /**
     * Per-machine state that doesn't fit in the tile element: price and statistics.
     * Stock and the broken flag live in the VendingMachineElement itself.
     */
    struct VendingMachine
    {
        VendingMachineIndex id = VendingMachineIndex::GetNull();
        // The path addition object; kObjectEntryIndexNull marks a free slot.
        ObjectEntryIndex type = kObjectEntryIndexNull;
        TileCoordsXYZD position;
        money64 price{};
        money64 totalProfit{};
        uint32_t totalCustomers{};
        uint32_t numItemsSold{};
        uint32_t timesRestocked{};

        bool isNull() const
        {
            return type == kObjectEntryIndexNull;
        }
    };

    void VendingMachineInit(GameState_t& gameState);
    VendingMachine* GetVendingMachine(VendingMachineIndex id);
    VendingMachine* GetOrCreateVendingMachine(VendingMachineIndex id);

    /**
     * Allocates a record for a new machine of the given type, priced at the default for what it sells.
     * Returns nullptr if the limit has been reached.
     */
    VendingMachine* CreateVendingMachine(ObjectEntryIndex type, const TileCoordsXYZD& position);
    void DeleteVendingMachine(VendingMachineIndex id);
    size_t GetNumVendingMachines();
    bool HasReachedVendingMachineLimit();

    /**
     * The price a new machine selling shopItem starts at: the item's default price, or the park-wide price
     * when "same price throughout park" is on for that item.
     */
    money64 VendingMachineGetDefaultPrice(ShopItem shopItem);

    /**
     * Sets the price of every vending machine selling shopItem (for "same price throughout park").
     */
    void VendingMachinesSetCommonPrice(ShopItem shopItem, money64 price);

    /**
     * Deletes the record of a vending machine element that is about to be removed (no-op for other elements).
     */
    void VendingMachineDeleteRecordFor(const TileElement& element);

    /**
     * Repairs the link between machine elements and their records after loading: gives every machine its own
     * record, corrects positions and frees records no machine refers to.
     */
    void VendingMachineApplyFixes();

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
