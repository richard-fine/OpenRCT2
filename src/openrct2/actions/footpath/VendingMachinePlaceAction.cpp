/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "VendingMachinePlaceAction.h"

#include "../../Cheats.h"
#include "../../Diagnostic.h"
#include "../../GameState.h"
#include "../../OpenRCT2.h"
#include "../../localisation/StringIds.h"
#include "../../management/Finance.h"
#include "../../object/ObjectEntryManager.h"
#include "../../object/PathAdditionEntry.h"
#include "../../world/Footpath.h"
#include "../../world/Location.hpp"
#include "../../world/Map.h"
#include "../../world/VendingMachine.h"
#include "../../world/tile_element/PathElement.h"
#include "../../world/tile_element/VendingMachineElement.h"

namespace OpenRCT2::GameActions
{
    VendingMachinePlaceAction::VendingMachinePlaceAction(const CoordsXYZD& loc, ObjectEntryIndex entryIndex)
        : _loc(loc)
        , _entryIndex(entryIndex)
    {
    }

    void VendingMachinePlaceAction::AcceptParameters(GameActionParameterVisitor& visitor)
    {
        visitor.Visit(_loc);
        visitor.Visit("object", _entryIndex);
    }

    uint16_t VendingMachinePlaceAction::GetActionFlags() const
    {
        return GameAction::GetActionFlags();
    }

    void VendingMachinePlaceAction::Serialise(DataSerialiser& stream)
    {
        GameAction::Serialise(stream);

        stream << DS_TAG(_loc) << DS_TAG(_entryIndex);
    }

    Result VendingMachinePlaceAction::Query(GameState_t& gameState, Park::ParkData& park) const
    {
        auto res = Result();
        res.expenditure = ExpenditureType::landscaping;
        res.position = _loc;
        if (!LocationValid(_loc))
        {
            return Result(Status::invalidParameters, STR_CANT_POSITION_THIS_HERE, STR_OFF_EDGE_OF_MAP);
        }

        if (_loc.direction >= kNumOrthogonalDirections)
        {
            return Result(Status::invalidParameters, STR_CANT_POSITION_THIS_HERE, kStringIdNone);
        }

        if (!(gLegacyScene == LegacyScene::scenarioEditor || gameState.cheats.sandboxMode) && !MapIsLocationOwned(_loc))
        {
            return Result(Status::disallowed, STR_CANT_POSITION_THIS_HERE, STR_LAND_NOT_OWNED_BY_PARK);
        }

        if (_loc.z < kFootpathMinHeight)
        {
            return Result(Status::invalidParameters, STR_CANT_POSITION_THIS_HERE, STR_TOO_LOW);
        }

        if (_loc.z > kFootpathMaxHeight)
        {
            return Result(Status::invalidParameters, STR_CANT_POSITION_THIS_HERE, STR_TOO_HIGH);
        }

        auto* entry = ObjectEntryManager::GetObjectEntry<PathAdditionEntry>(_entryIndex);
        if (entry == nullptr || !entry->flags.has(PathAdditionFlag::isVendingMachine))
        {
            LOG_ERROR("Invalid vending machine entry for entryIndex %d", _entryIndex);
            return Result(Status::invalidParameters, STR_CANT_POSITION_THIS_HERE, STR_UNKNOWN_OBJECT_TYPE);
        }

        auto* pathElement = MapGetFootpathElement(_loc);
        if (pathElement == nullptr)
        {
            return Result(Status::invalidParameters, STR_CANT_POSITION_THIS_HERE, STR_ERR_PATH_ELEMENT_NOT_FOUND);
        }

        if (pathElement->isLevelCrossing(_loc))
        {
            return Result(
                Status::invalidParameters, STR_CANT_POSITION_THIS_HERE, STR_CANNOT_BUILD_PATH_ADDITIONS_ON_LEVEL_CROSSINGS);
        }

        if (pathElement->isSloped())
        {
            return Result(Status::invalidParameters, STR_CANT_POSITION_THIS_HERE, STR_CANT_BUILD_THIS_ON_SLOPED_FOOTPATH);
        }

        if (pathElement->isQueue())
        {
            return Result(Status::invalidParameters, STR_CANT_POSITION_THIS_HERE, STR_CANNOT_PLACE_THESE_ON_QUEUE_LINE_AREA);
        }

        if (pathElement->getEdges() & (1 << _loc.direction))
        {
            return Result(Status::invalidParameters, STR_CANT_POSITION_THIS_HERE, STR_CAN_ONLY_BE_PLACED_ON_PATH_EDGES);
        }

        const auto clearanceZ = _loc.z + kVendingMachineClearanceZ;
        auto* existing = VendingMachineGetElementAt(_loc, false);
        const bool isRepair = existing != nullptr && existing->getEntryIndex() == _entryIndex && existing->isBroken();
        if (!isRepair && !GetFlags().has(CommandFlag::ghost) && existing != nullptr
            && existing->getEntryIndex() == _entryIndex)
        {
            // Same machine already here and working: nothing to do.
            return res;
        }

        if (existing == nullptr && !gameState.cheats.disableClearanceChecks)
        {
            // Machines may stand in front of fences, but not in front of each other.
            if (VendingMachineInTheWay({ _loc, _loc.z, clearanceZ }, _loc.direction))
            {
                return Result(Status::itemAlreadyPlaced, STR_CANT_POSITION_THIS_HERE, STR_OBJECT_IN_THE_WAY);
            }
        }

        if (GetFlags().has(CommandFlag::ghost) && existing != nullptr)
        {
            return Result(Status::itemAlreadyPlaced, STR_CANT_POSITION_THIS_HERE, kStringIdNone);
        }

        if (existing == nullptr && !MapCheckCapacityAndReorganise(_loc))
        {
            return Result(Status::noFreeElements, STR_CANT_POSITION_THIS_HERE, STR_TILE_ELEMENT_LIMIT_REACHED);
        }

        res.cost = entry->price;
        return res;
    }

    Result VendingMachinePlaceAction::Execute(GameState_t& gameState, Park::ParkData& park) const
    {
        auto res = Result();
        res.position = _loc;
        res.expenditure = ExpenditureType::landscaping;

        auto* entry = ObjectEntryManager::GetObjectEntry<PathAdditionEntry>(_entryIndex);
        if (entry == nullptr)
        {
            return Result(Status::invalidParameters, STR_CANT_POSITION_THIS_HERE, STR_UNKNOWN_OBJECT_TYPE);
        }

        const bool isGhost = GetFlags().has(CommandFlag::ghost);
        auto* machine = isGhost ? nullptr : VendingMachineGetElementAt(_loc, false);
        if (machine != nullptr)
        {
            if (machine->getEntryIndex() == _entryIndex && !machine->isBroken())
            {
                return res;
            }

            // Re-placing the same machine repairs it; a different machine replaces it with a fresh one.
            if (machine->getEntryIndex() != _entryIndex)
            {
                machine->setEntryIndex(_entryIndex);
                machine->setStock(entry->stockCapacity);
            }
            machine->setIsBroken(false);
        }
        else
        {
            machine = TileElementInsert<VendingMachineElement>(_loc, 0b0000);
            if (machine == nullptr)
            {
                return Result(Status::noFreeElements, STR_CANT_POSITION_THIS_HERE, STR_TILE_ELEMENT_LIMIT_REACHED);
            }

            machine->setClearanceZ(_loc.z + kVendingMachineClearanceZ);
            machine->setDirection(_loc.direction);
            machine->setEntryIndex(_entryIndex);
            machine->setIsBroken(false);
            machine->setStock(entry->stockCapacity);
            machine->setMachineIndex(0xFFFF);
            machine->setGhost(isGhost);
        }

        MapInvalidateTileZoom1({ _loc, machine->getBaseZ(), machine->getClearanceZ() });

        res.cost = entry->price;
        return res;
    }
} // namespace OpenRCT2::GameActions
