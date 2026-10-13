#pragma once

#include "CoreMinimal.h"

class UCanvasPanel;
class UWidgetTree;

/** The map page's compass: the letters N, E, S and W on the middles of the map frame's four edges. */
namespace MapCompass
{
	/**
	 * Puts a small disc with each letter on the middle of the frame's edge it faces (the map is north up): N top, E right,
	 * S bottom, W left. Page is the canvas the frame sits on, FramePosition and FrameSize the frame's box on it. N is the
	 * accent's, as on the minimap's bezel; E, S and W are dimmer. The letters are named MapCompass_N and so on.
	 */
	void Build(UWidgetTree* Tree, UCanvasPanel& Page, const FVector2D& FramePosition, const FVector2D& FrameSize);
}
