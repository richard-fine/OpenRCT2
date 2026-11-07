/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "Paint.VendingMachine.h"

#include "../../drawing/LightFX.h"
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

    // Indexed by screen edge (NE, SE, SW, NW). Machines are 6 units deep and 16 wide, facing the centre of the path.
    // They stand just inside the railings of raised paths (which are 4 units in from the tile edge), so their bounding
    // boxes don't overlap and the two sort correctly.
    constexpr MachineGeometry kMachineGeometry[] = {
        { { 8, 16, 0 }, { 5, 8, 2 }, { 6, 16, 24 } },
        { { 16, 24, 0 }, { 8, 21, 2 }, { 16, 6, 24 } },
        { { 24, 16, 0 }, { 21, 8, 2 }, { 6, 16, 24 } },
        { { 16, 8, 0 }, { 8, 5, 2 }, { 16, 6, 24 } },
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

    // The display glows at night, unless the machine is broken. Sold-out machines stay lit; their sprite shows they
    // are empty.
    if (LightFx::IsAvailable() && !isBroken && !machineElement.isGhost())
    {
        // Offsets from the tile centre towards the machine's edge, by (unrotated) direction.
        static constexpr CoordsXY kLightOffsets[] = { { -5, 0 }, { 0, 5 }, { 5, 0 }, { 0, -5 } };
        const auto& offset = kLightOffsets[machineElement.getDirection() & 3];
        LightFx::Add3DLightMagicFromDrawingTile(
            session.MapPosition, offset.x, offset.y, height + 18, LightFx::LightType::lantern1);
    }
}
