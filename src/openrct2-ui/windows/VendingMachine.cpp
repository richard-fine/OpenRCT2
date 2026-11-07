/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "../UiStringIds.h"

#include <openrct2-ui/interface/Widget.h>
#include <openrct2-ui/interface/Window.h>
#include <openrct2-ui/windows/Windows.h>
#include <openrct2/GameState.h>
#include <openrct2/SpriteIds.h>
#include <openrct2/actions/GameActionRunner.h>
#include <openrct2/actions/footpath/VendingMachineRemoveAction.h>
#include <openrct2/actions/footpath/VendingMachineSetPriceAction.h>
#include <openrct2/core/String.hpp>
#include <openrct2/drawing/Text.h>
#include <openrct2/localisation/Currency.h>
#include <openrct2/interface/Viewport.h>
#include <openrct2/localisation/Formatter.h>
#include <openrct2/localisation/Formatting.h>
#include <openrct2/object/ObjectEntryManager.h>
#include <openrct2/object/PathAdditionEntry.h>
#include <openrct2/ride/Ride.h>
#include <openrct2/ride/ShopItem.h>
#include <openrct2/ui/WindowManager.h>
#include <openrct2/world/VendingMachine.h>
#include <openrct2/world/tile_element/VendingMachineElement.h>

namespace OpenRCT2::Ui::Windows
{
    static constexpr StringId kWindowTitle = STR_THEMES_WINDOW_VENDING_MACHINE;
    static constexpr ScreenSize kWindowSize = { 230, 172 };
    static constexpr int32_t kInfoTop = 106;
    static constexpr int32_t kLineHeight = 12;

    enum WindowVendingMachineWidgetIdx : WidgetIndex
    {
        WIDX_BACKGROUND,
        WIDX_TITLE,
        WIDX_CLOSE,
        WIDX_VIEWPORT,
        WIDX_DEMOLISH,
        WIDX_PRICE_LABEL,
        WIDX_PRICE,
        WIDX_PRICE_INCREASE,
        WIDX_PRICE_DECREASE,
    };

    // clang-format off
    static constexpr auto kVendingMachineWidgets = makeWidgets(
        makeWindowShim(kWindowTitle, kWindowSize),
        makeWidget                ({                     3, 17}, {kWindowSize.width - 31, 66}, WidgetType::viewport, WindowColour::secondary, 0x0FFFFFFFE                                       ),
        makeWidget                ({kWindowSize.width - 25, 19}, {                     24, 24}, WidgetType::flatBtn,  WindowColour::secondary, ImageId(SPR_DEMOLISH), STR_DEMOLISH_VENDING_MACHINE_TIP),
        makeWidget                ({                     5, 88}, {                    115, 14}, WidgetType::label,    WindowColour::secondary                                                    ),
        makeHoldableSpinnerWidgets({                   120, 87}, {                    105, 14}, WidgetType::spinner,  WindowColour::secondary, kStringIdEmpty                                    )  // NB: 3 widgets
    );
    // clang-format on

    class VendingMachineWindow final : public Window
    {
    private:
        CoordsXYZ _viewPos;
        std::string _priceCaption;
        char _moneyInputText[kMoneyStringMaxlength] = {};

        VendingMachineIndex GetMachineIndex() const
        {
            return VendingMachineIndex::FromUnderlying(static_cast<VendingMachineIndex::UnderlyingType>(number));
        }

        VendingMachine* GetMachine() const
        {
            return GetVendingMachine(GetMachineIndex());
        }

        VendingMachineElement* GetMachineElement() const
        {
            const auto* machine = GetMachine();
            if (machine == nullptr)
                return nullptr;
            return VendingMachineGetElementAt(machine->position.toCoordsXYZD(), false);
        }

        const PathAdditionEntry* GetEntry() const
        {
            const auto* machine = GetMachine();
            return machine != nullptr ? ObjectEntryManager::GetObjectEntry<PathAdditionEntry>(machine->type) : nullptr;
        }

        void createViewport()
        {
            const auto& viewportWidget = widgets[WIDX_VIEWPORT];
            ViewportCreate(
                *this, windowPos + ScreenCoordsXY{ viewportWidget.left + 1, viewportWidget.top + 1 },
                viewportWidget.width() - 2, viewportWidget.height() - 2, Focus(_viewPos));
            invalidate();
        }

        void SetPrice(money64 price)
        {
            auto action = GameActions::VendingMachineSetPriceAction(
                GetMachineIndex(), std::clamp(price, kRideMinPrice, kRideMaxPrice));
            GameActions::Execute(&action, getGameState());
        }

        bool CanChangePrice() const
        {
            return !getGameState().park.flags.has(ParkFlag::noMoney);
        }

    public:
        void onOpen() override
        {
            setWidgets(kVendingMachineWidgets);
            WindowInitScrollWidgets(*this);
        }

        void initialise(WindowNumber machineNumber)
        {
            number = machineNumber;
            const auto* machine = GetMachine();
            if (machine == nullptr)
            {
                close();
                return;
            }

            // Look at the middle of the path tile, a little above the path.
            const auto pos = machine->position.toCoordsXYZ();
            _viewPos = { pos.toTileCentre(), pos.z + 16 };
            createViewport();
        }

        void onMouseUp(WidgetIndex widgetIndex) override
        {
            switch (widgetIndex)
            {
                case WIDX_CLOSE:
                    close();
                    break;
                case WIDX_DEMOLISH:
                {
                    const auto* machine = GetMachine();
                    if (machine == nullptr)
                        break;
                    auto removeAction = GameActions::VendingMachineRemoveAction(machine->position.toCoordsXYZD());
                    GameActions::Execute(&removeAction, getGameState());
                    break;
                }
                case WIDX_PRICE:
                {
                    const auto* machine = GetMachine();
                    if (machine == nullptr || !CanChangePrice())
                        break;
                    MoneyToString(machine->price, _moneyInputText, kMoneyStringMaxlength, true);
                    WindowTextInputRawOpen(
                        this, WIDX_PRICE, STR_ENTER_NEW_VALUE, STR_ENTER_NEW_VALUE, {}, _moneyInputText,
                        kMoneyStringMaxlength);
                    break;
                }
            }
        }

        void onMouseDown(WidgetIndex widgetIndex) override
        {
            const auto* machine = GetMachine();
            if (machine == nullptr || !CanChangePrice())
                return;

            switch (widgetIndex)
            {
                case WIDX_PRICE_INCREASE:
                    if (machine->price < kRideMaxPrice)
                        SetPrice(machine->price + 1);
                    break;
                case WIDX_PRICE_DECREASE:
                    if (machine->price > kRideMinPrice)
                        SetPrice(machine->price - 1);
                    break;
            }
        }

        void onTextInput(WidgetIndex widgetIndex, std::string_view text) override
        {
            if (widgetIndex != WIDX_PRICE || text.empty())
                return;

            std::string strText{ text };
            money64 price = StringToMoney(strText.c_str());
            if (price != kMoney64Undefined)
            {
                SetPrice(price);
            }
        }

        void onUpdate() override
        {
            // Stock and statistics change as guests buy and handymen restock.
            if (GetMachine() == nullptr)
            {
                close();
                return;
            }
            invalidateWidget(WIDX_BACKGROUND);
        }

        void onViewportRotate() override
        {
            removeViewport();
            createViewport();
        }

        void onPrepareDraw() override
        {
            const auto* machine = GetMachine();
            const auto* entry = GetEntry();
            if (machine == nullptr || entry == nullptr)
                return;

            widgets[WIDX_TITLE].text = entry->name;

            const auto& descriptor = GetShopItemDescriptor(entry->shopItem);
            widgets[WIDX_PRICE_LABEL].text = descriptor.Naming.PriceLabel;

            if (machine->price == 0)
            {
                widgets[WIDX_PRICE].setString(STR_FREE);
            }
            else
            {
                _priceCaption = FormatStringID(STR_CURRENCY2DP, machine->price);
                widgets[WIDX_PRICE].setString(_priceCaption.c_str());
            }

            const bool canChangePrice = CanChangePrice();
            setWidgetDisabled(WIDX_PRICE, !canChangePrice);
            setWidgetDisabled(WIDX_PRICE_INCREASE, !canChangePrice);
            setWidgetDisabled(WIDX_PRICE_DECREASE, !canChangePrice);
        }

        void onDraw(Drawing::RenderTarget& rt) override
        {
            drawWidgets(rt);

            if (viewport != nullptr)
            {
                WindowDrawViewport(rt, *this);
            }

            const auto* machine = GetMachine();
            const auto* entry = GetEntry();
            const auto* machineElement = GetMachineElement();
            if (machine == nullptr || entry == nullptr || machineElement == nullptr)
                return;

            auto screenCoords = windowPos + ScreenCoordsXY{ 5, kInfoTop };

            auto ft = Formatter();
            ft.Add<uint16_t>(machineElement->getStock());
            ft.Add<uint16_t>(entry->stockCapacity);
            drawText(rt, screenCoords, STR_VENDING_MACHINE_STOCK, ft);
            screenCoords.y += kLineHeight;

            ft = Formatter();
            ft.Add<uint32_t>(machine->timesRestocked);
            drawText(rt, screenCoords, STR_VENDING_MACHINE_TIMES_RESTOCKED, ft);
            screenCoords.y += kLineHeight;

            ft = Formatter();
            ft.Add<uint32_t>(machine->totalCustomers);
            drawText(rt, screenCoords, STR_VENDING_MACHINE_CUSTOMERS, ft);
            screenCoords.y += kLineHeight;

            if (!getGameState().park.flags.has(ParkFlag::noMoney))
            {
                ft = Formatter();
                ft.Add<money64>(machine->totalProfit);
                drawText(rt, screenCoords, STR_VENDING_MACHINE_PROFIT, ft);
            }
            screenCoords.y += kLineHeight;

            StringId status = STR_VENDING_MACHINE_STATUS_WORKING;
            if (machineElement->isBroken())
                status = STR_VENDING_MACHINE_STATUS_BROKEN;
            else if (machineElement->getStock() == 0)
                status = STR_VENDING_MACHINE_STATUS_SOLD_OUT;
            ft = Formatter();
            drawText(rt, screenCoords, status, ft);
        }
    };

    WindowBase* VendingMachineOpen(WindowNumber number)
    {
        auto* windowMgr = GetWindowManager();
        auto* w = static_cast<VendingMachineWindow*>(windowMgr->BringToFrontByNumber(WindowClass::vendingMachine, number));
        if (w != nullptr)
            return w;

        w = windowMgr->Create<VendingMachineWindow>(WindowClass::vendingMachine, kWindowSize, {});
        if (w != nullptr)
            w->initialise(number);

        return w;
    }
} // namespace OpenRCT2::Ui::Windows
