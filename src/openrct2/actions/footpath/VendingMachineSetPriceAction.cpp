/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "VendingMachineSetPriceAction.h"

#include "../../Diagnostic.h"
#include "../../localisation/StringIds.h"
#include "../../management/Finance.h"
#include "../../ride/Ride.h"
#include "../../world/VendingMachine.h"

namespace OpenRCT2::GameActions
{
    VendingMachineSetPriceAction::VendingMachineSetPriceAction(VendingMachineIndex machineIndex, money64 price)
        : _machineIndex(machineIndex)
        , _price(price)
    {
    }

    void VendingMachineSetPriceAction::AcceptParameters(GameActionParameterVisitor& visitor)
    {
        visitor.Visit("vendingMachine", _machineIndex);
        visitor.Visit("price", _price);
    }

    uint16_t VendingMachineSetPriceAction::GetActionFlags() const
    {
        return GameAction::GetActionFlags() | Flags::AllowWhilePaused;
    }

    void VendingMachineSetPriceAction::Serialise(DataSerialiser& stream)
    {
        GameAction::Serialise(stream);

        stream << DS_TAG(_machineIndex) << DS_TAG(_price);
    }

    Result VendingMachineSetPriceAction::Query(GameState_t& gameState, Park::ParkData& park) const
    {
        if (GetVendingMachine(_machineIndex) == nullptr)
        {
            LOG_ERROR("Vending machine not found for index %u", _machineIndex.ToUnderlying());
            return Result(Status::invalidParameters, STR_ERR_INVALID_PARAMETER, kStringIdNone);
        }

        if (_price < kRideMinPrice || _price > kRideMaxPrice)
        {
            LOG_ERROR("Attempting to set an invalid price for vending machine %u", _machineIndex.ToUnderlying());
            return Result(Status::invalidParameters, STR_ERR_INVALID_PARAMETER, kStringIdEmpty);
        }

        return Result();
    }

    Result VendingMachineSetPriceAction::Execute(GameState_t& gameState, Park::ParkData& park) const
    {
        auto res = Result();
        res.expenditure = ExpenditureType::shopSales;

        auto* machine = GetVendingMachine(_machineIndex);
        if (machine == nullptr)
        {
            LOG_ERROR("Vending machine not found for index %u", _machineIndex.ToUnderlying());
            return Result(Status::invalidParameters, STR_ERR_INVALID_PARAMETER, kStringIdNone);
        }

        // TODO: Apply to other machines (and shops) when "same price throughout park" is on for the item.
        machine->price = _price;
        res.position = machine->position.toCoordsXYZ().toTileCentre();
        return res;
    }
} // namespace OpenRCT2::GameActions
