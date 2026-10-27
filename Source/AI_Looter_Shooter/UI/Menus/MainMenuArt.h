#pragma once

#include "CoreMinimal.h"
#include "UI/Style/LooterUIStyle.h"

class UWidget;
class UWidgetTree;

/**
 * The main menu's own pictures, drawn in code with the kit's painted brushes (no texture assets): the title's mark, the
 * dark shade behind the menu's side of the screen, the wash under a lit button, and a page's header.
 */
namespace MainMenuArt
{
	/**
	 * The game's mark beside its title: an ember standing through a coin's broken ring, a cyan horizon line running out
	 * of it both ways. Original: a ferryman's coin (Docs/Story.md: the dead paid with a coin on each eye) split by the
	 * ember a Revenant carries. 64 x 64 view box.
	 */
	const LooterUI::FPaintedIcon& TitleMark();

	/**
	 * The shade the menu sits on: the kit's backdrop dark at the left edge, holding through the menu's side, then thinning
	 * to nothing, so the planet on the right is untouched. Stretched over the shade's whole box; 64 x 4 view box.
	 */
	const LooterUI::FPaintedIcon& Shade();

	/** The faint orange wash under a lit main button, strongest at its left, gone by its right. 32 x 4 view box. */
	const LooterUI::FPaintedIcon& RowWash();

	/** A page's header in the menu's column ("SINGLE PLAYER"): an orange tick, the words, a thin cyan rule out to the right. */
	UWidget* MakePageHeader(UWidgetTree* Tree, const FString& Title);
}
