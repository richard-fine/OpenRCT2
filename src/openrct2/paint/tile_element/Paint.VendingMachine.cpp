/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "Paint.VendingMachine.h"

#include "../../drawing/PaletteIndex.h"
#include "../../interface/Viewport.h"
#include "../../object/PathAdditionEntry.h"
#include "../../profiling/Profiling.h"
#include "../../ride/TrackDesign.h"
#include "../../world/tile_element/VendingMachineElement.h"
#include "../Paint.h"

using namespace OpenRCT2;
using namespace OpenRCT2::Drawing;

namespace
{
    struct MachineGeometry
    {
        CoordsXYZ offset;
        CoordsXYZ boundBoxOffset;
        CoordsXYZ boundBoxLength;
    };

    // Indexed by screen edge (NE, SE, SW, NW). Machines stand against the edge, 6 units deep and 16 wide,
    // facing the centre of the path.
    constexpr MachineGeometry kMachineGeometry[] = {
        { { 4, 16, 0 }, { 1, 8, 2 }, { 6, 16, 24 } },
        { { 16, 28, 0 }, { 8, 25, 2 }, { 16, 6, 24 } },
        { { 28, 16, 0 }, { 25, 8, 2 }, { 6, 16, 24 } },
        { { 16, 4, 0 }, { 8, 1, 2 }, { 16, 6, 24 } },
    };
} // namespace

void PaintVendingMachine(
    PaintSession& session, uint8_t direction, int32_t height, const VendingMachineElement& machineElement)
{
    PROFILED_FUNCTION();

    if (session.rt.zoom_level > ZoomLevel{ 1 } || gTrackDesignSaveMode)
        return;

    auto* entry = machineElement.getEntry();
    if (entry == nullptr)
        return;

    const auto isBroken = machineElement.isBroken();
    const auto isEmpty = machineElement.getStock() == 0;
    if (session.ViewFlags.has(ViewportFlag::highlightPathIssues) && !isBroken && !isEmpty)
        return;

    session.InteractionType = ViewportInteractionItem::vendingMachine;

    ImageId imageTemplate;
    if (machineElement.isGhost())
    {
        session.InteractionType = ViewportInteractionItem::none;
        imageTemplate = ImageId().WithRemap(FilterPaletteID::paletteGhost);
    }
    else if (session.SelectedElement == reinterpret_cast<const TileElement*>(&machineElement))
    {
        imageTemplate = ImageId().WithRemap(FilterPaletteID::paletteGhost);
    }

    // direction is the machine's edge, already rotated to the view.
    const auto screenEdge = direction & 3;
    const auto stateOffset = isBroken ? 4 : (isEmpty ? 8 : 0);
    const auto imageIndex = entry->image + 1 + screenEdge + stateOffset;

    const auto& geometry = kMachineGeometry[screenEdge];
    PaintAddImageAsParent(
        session, imageTemplate.WithIndex(imageIndex), { geometry.offset.x, geometry.offset.y, height },
        { { geometry.boundBoxOffset.x, geometry.boundBoxOffset.y, height + geometry.boundBoxOffset.z },
          geometry.boundBoxLength });
}
