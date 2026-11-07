/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "VendingMachineRemoveAction.h"

#include "../../Cheats.h"
#include "../../GameState.h"
#include "../../OpenRCT2.h"
#include "../../localisation/StringIds.h"
#include "../../management/Finance.h"
#include "../../world/Location.hpp"
#include "../../world/Map.h"
#include "../../world/VendingMachine.h"
#include "../../world/tile_element/TileElement.h"
#include "../../world/tile_element/VendingMachineElement.h"

namespace OpenRCT2::GameActions
{
    VendingMachineRemoveAction::VendingMachineRemoveAction(const CoordsXYZD& loc)
        : _loc(loc)
    {
    }

    void VendingMachineRemoveAction::AcceptParameters(GameActionParameterVisitor& visitor)
    {
        visitor.Visit(_loc);
    }

    uint16_t VendingMachineRemoveAction::GetActionFlags() const
    {
        return GameAction::GetActionFlags();
    }

    void VendingMachineRemoveAction::Serialise(DataSerialiser& stream)
    {
        GameAction::Serialise(stream);

        stream << DS_TAG(_loc);
    }

    Result VendingMachineRemoveAction::Query(GameState_t& gameState, Park::ParkData& park) const
    {
        auto res = Result();
        res.expenditure = ExpenditureType::landscaping;
        res.position = _loc;

        if (!LocationValid(_loc))
        {
            return Result(Status::invalidParameters, STR_CANT_REMOVE_THIS, STR_OFF_EDGE_OF_MAP);
        }

        const bool isGhost = GetFlags().has(CommandFlag::ghost);
        if (!isGhost && gLegacyScene != LegacyScene::scenarioEditor && !gameState.cheats.sandboxMode
            && !MapIsLocationOwned(_loc))
        {
            return Result(Status::notOwned, STR_CANT_REMOVE_THIS, STR_LAND_NOT_OWNED_BY_PARK);
        }

        if (VendingMachineGetElementAt(_loc, isGhost) == nullptr)
        {
            return Result(Status::invalidParameters, STR_CANT_REMOVE_THIS, STR_INVALID_SELECTION_OF_OBJECTS);
        }

        return res;
    }

    Result VendingMachineRemoveAction::Execute(GameState_t& gameState, Park::ParkData& park) const
    {
        auto res = Result();
        res.expenditure = ExpenditureType::landscaping;
        res.position = { _loc.x + 16, _loc.y + 16, _loc.z };

        auto* machine = VendingMachineGetElementAt(_loc, GetFlags().has(CommandFlag::ghost));
        if (machine == nullptr)
        {
            return Result(Status::invalidParameters, STR_CANT_REMOVE_THIS, STR_INVALID_SELECTION_OF_OBJECTS);
        }

        MapInvalidateTileZoom1({ _loc, machine->getBaseZ(), machine->getClearanceZ() });
        DeleteVendingMachine(machine->getMachineIndex());
        TileElementRemove(reinterpret_cast<TileElement*>(machine));
        return res;
    }
} // namespace OpenRCT2::GameActions
