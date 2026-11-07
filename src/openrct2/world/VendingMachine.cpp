/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "VendingMachine.h"

#include "../Diagnostic.h"
#include "../GameState.h"
#include "../object/ObjectEntryManager.h"
#include "../object/PathAdditionEntry.h"
#include "../ride/ShopItem.h"
#include "Map.h"
#include "TileElementsView.h"
#include "tile_element/TileElement.h"
#include "tile_element/VendingMachineElement.h"

#include <vector>

namespace OpenRCT2
{
    void VendingMachineInit(GameState_t& gameState)
    {
        gameState.vendingMachines.clear();
    }

    VendingMachine* GetVendingMachine(VendingMachineIndex id)
    {
        auto& gameState = getGameState();
        const auto index = id.ToUnderlying();
        if (id.IsNull() || index >= gameState.vendingMachines.size())
            return nullptr;

        auto& machine = gameState.vendingMachines[index];
        return machine.isNull() ? nullptr : &machine;
    }

    VendingMachine* GetOrCreateVendingMachine(VendingMachineIndex id)
    {
        auto& gameState = getGameState();
        const auto index = id.ToUnderlying();
        if (id.IsNull() || index >= kMaxVendingMachines)
            return nullptr;

        if (index >= gameState.vendingMachines.size())
            gameState.vendingMachines.resize(index + 1);

        auto& machine = gameState.vendingMachines[index];
        machine.id = id;
        return &machine;
    }

    static VendingMachineIndex VendingMachineGetNewIndex()
    {
        auto& gameState = getGameState();
        for (size_t index = 0; index < kMaxVendingMachines; index++)
        {
            if (index >= gameState.vendingMachines.size() || gameState.vendingMachines[index].isNull())
                return VendingMachineIndex::FromUnderlying(static_cast<VendingMachineIndex::UnderlyingType>(index));
        }
        return VendingMachineIndex::GetNull();
    }

    VendingMachine* CreateVendingMachine(ObjectEntryIndex type, const TileCoordsXYZD& position)
    {
        auto* machine = GetOrCreateVendingMachine(VendingMachineGetNewIndex());
        if (machine == nullptr)
            return nullptr;

        const auto id = machine->id;
        *machine = {};
        machine->id = id;
        machine->type = type;
        machine->position = position;

        auto* entry = ObjectEntryManager::GetObjectEntry<PathAdditionEntry>(type);
        machine->price = entry != nullptr ? VendingMachineGetDefaultPrice(entry->shopItem) : 0;
        return machine;
    }

    void DeleteVendingMachine(VendingMachineIndex id)
    {
        auto* machine = GetVendingMachine(id);
        if (machine != nullptr)
        {
            *machine = {};
        }
    }

    size_t GetNumVendingMachines()
    {
        size_t count = 0;
        for (const auto& machine : getGameState().vendingMachines)
        {
            if (!machine.isNull())
                count++;
        }
        return count;
    }

    bool HasReachedVendingMachineLimit()
    {
        return GetNumVendingMachines() >= kMaxVendingMachines;
    }

    money64 VendingMachineGetDefaultPrice(ShopItem shopItem)
    {
        auto& gameState = getGameState();
        if (gameState.park.flags.has(ParkFlag::noMoney) || shopItem == ShopItem::none)
            return 0;

        if (ShopItemHasCommonPrice(shopItem))
        {
            auto ridePrice = ShopItemGetCommonPrice(nullptr, shopItem);
            if (ridePrice != kMoney64Undefined)
                return ridePrice;

            for (const auto& machine : gameState.vendingMachines)
            {
                if (machine.isNull())
                    continue;
                auto* entry = ObjectEntryManager::GetObjectEntry<PathAdditionEntry>(machine.type);
                if (entry != nullptr && entry->shopItem == shopItem)
                    return machine.price;
            }
        }
        return GetShopItemDescriptor(shopItem).DefaultPrice;
    }

    void VendingMachineDeleteRecordFor(const TileElement& element)
    {
        if (auto* machineElement = element.asVendingMachine(); machineElement != nullptr)
        {
            DeleteVendingMachine(machineElement->getMachineIndex());
        }
    }

    void VendingMachineApplyFixes()
    {
        auto& gameState = getGameState();
        std::vector<bool> claimed(gameState.vendingMachines.size(), false);

        TileElementIterator it;
        TileElementIteratorBegin(&it);
        while (TileElementIteratorNext(&it))
        {
            auto* machineElement = it.element->asVendingMachine();
            if (machineElement == nullptr)
                continue;

            if (machineElement->isGhost())
            {
                machineElement->setMachineIndex(VendingMachineIndex::GetNull());
                continue;
            }

            const TileCoordsXYZD position{ it.x, it.y, machineElement->baseHeight, machineElement->getDirection() };

            auto index = machineElement->getMachineIndex();
            auto* machine = GetVendingMachine(index);
            const bool alreadyClaimed = machine != nullptr && claimed[index.ToUnderlying()];
            if (machine == nullptr || alreadyClaimed)
            {
                auto* newMachine = CreateVendingMachine(machineElement->getEntryIndex(), position);
                if (newMachine == nullptr)
                {
                    LOG_ERROR("Too many vending machines; unable to create a record for one at %d, %d.", it.x, it.y);
                    machineElement->setMachineIndex(VendingMachineIndex::GetNull());
                    continue;
                }
                if (alreadyClaimed)
                {
                    // Two machines shared a record: give this one a copy of it.
                    const auto newId = newMachine->id;
                    *newMachine = *machine;
                    newMachine->id = newId;
                }
                machine = newMachine;
                index = newMachine->id;
                machineElement->setMachineIndex(index);
            }

            if (index.ToUnderlying() >= claimed.size())
                claimed.resize(index.ToUnderlying() + 1, false);
            claimed[index.ToUnderlying()] = true;
            machine->type = machineElement->getEntryIndex();
            machine->position = position;
        }

        for (size_t i = 0; i < gameState.vendingMachines.size(); i++)
        {
            if (i >= claimed.size() || !claimed[i])
                gameState.vendingMachines[i] = {};
        }
    }

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
                DeleteVendingMachine(machine->getMachineIndex());
                TileElementRemove(reinterpret_cast<TileElement*>(machine));
                // Removing shifts the remaining elements, so restart the scan.
                removed = true;
                break;
            }
        } while (removed);
    }
} // namespace OpenRCT2
