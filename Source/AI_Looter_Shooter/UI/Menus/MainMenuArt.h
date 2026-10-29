#pragma once

#include "CoreMinimal.h"
#include "UI/Style/LooterUIStyle.h"

class UWidget;
class UWidgetTree;

/**
 * The main menu's own pictures, drawn in code with the kit's painted brushes (no texture assets), and concept 3 "Halo
 * Reticle"'s layout (the user's pick, 2026-10-29; the sight's pictures are LooterReticleArt's): the logo mark in the
 * title's halo, the dark shade behind the menu's side of the screen, the SOON tag's outline, the gold run on the range
 * arc under the lit word, a page's header and a key hint.
 */
namespace MainMenuArt
{
	/**
	 * Concept 3's main menu (Docs/Work/2026-10-29-Morning/ui3/make_a.js, c3Menu) in the menu's own units. The mockup's text
	 * sizes are Slate sizes as they are, and Slate draws them a third bigger (Docs/Pipeline/Bugs.md), so every distance
	 * round the type is the mockup's grown by a third too, keeping its proportions.
	 */
	namespace Layout
	{
		constexpr float Grow = 4.f / 3.f;

		// --- The title lockup (its own box, top-left of the menu's side) ---

		/** The halo round the logo mark (mockup r 34 at (58, 60)), its centre this far in from the lockup's corner. */
		constexpr float HaloRadius = 34.f * Grow;
		constexpr float HaloCentre = 70.f;
		/** Ticks every 10 degrees, the four at the cross long (mockup: major 90). */
		constexpr float HaloTickStep = 10.f;
		constexpr int32 HaloLongEvery = 9;
		/** Its ticks light clockwise in this many steps as the menu arrives. */
		constexpr int32 HaloSectors = 12;
		/** The logo mark inside the ring. */
		constexpr float LogoSize = 44.f * Grow;

		/** The lockup's words from the halo's centre (mockup: REVENANT from x 116 at y 54, the ruler 118 to 430 at 86, TEROPA
		 *  from 120 at 110; the halo at 58, 60). */
		constexpr float TitleX = (116.f - 58.f) * Grow;
		constexpr float TitleY = (54.f - 60.f) * Grow;
		constexpr float RulerX = (118.f - 58.f) * Grow;
		constexpr float RulerY = (86.f - 60.f) * Grow;
		constexpr float RulerLength = 312.f * Grow;
		constexpr float RulerStep = 8.f * Grow;
		constexpr int32 RulerLongEvery = 5;
		constexpr float PlanetX = (120.f - 58.f) * Grow;
		constexpr float PlanetY = (110.f - 60.f) * Grow;
		/** The type (mockup px as Slate sizes; spacing in thousandths of an em: the mockup's px over its size). */
		constexpr int32 TitleFont = 48;
		constexpr int32 TitleSpacing = 271;
		constexpr int32 PlanetFont = 14;
		constexpr int32 PlanetSpacing = 571;
		/** The lockup's box runs down to where the list's box begins (mockup y 124, a little above the range arc's top, so
		 *  the arc's picture and its edge fit inside the list's box). */
		constexpr float ListTopMockupY = 124.f;
		constexpr float TitleBoxHeight = HaloCentre + (ListTopMockupY - 60.f) * Grow;

		// --- The list under it: words right-aligned to a range arc, the row numbers on the arc ---

		/** The range arc is part of a big circle round the planet's side (mockup: centre (800, 250), r 440), in the list's
		 *  own box (its top at mockup y 124, x matched to the title's). */
		constexpr float ListX(float MockupX) { return HaloCentre + (MockupX - 58.f) * Grow; }
		constexpr float ListY(float MockupY) { return (MockupY - ListTopMockupY) * Grow; }
		constexpr float ArcRadius = 440.f * Grow;
		constexpr float ArcCentreX = ListX(800.f);
		constexpr float ArcCentreY = ListY(250.f);
		constexpr float ArcTop = ListY(132.f);
		constexpr float ArcBottom = ListY(362.f);
		/** A tick every 8 mockup px down the arc, pointing in toward the planet. */
		constexpr float ArcTickPx = 8.f * Grow;

		/** The rows, 60 mockup px apart from y 158, all alike (Quit Game no longer set apart: the mockup's even list). */
		constexpr float RowHeight = 60.f * Grow;
		constexpr float RowTop = ListY(158.f) - RowHeight * 0.5f;
		/** The list's box: wide enough for the SOON tag past the arc, tall enough for four rows. */
		constexpr float ListWidth = 610.f;
		constexpr float ListHeight = RowTop + RowHeight * 4.f;

		/** Along a row, from where it crosses the arc: the words end 48 before it, the number starts 30 past it, SOON 22
		 *  past it; a salt dash across the arc 10 each way; the lit row's gold dash from 40 to 26 before it. */
		constexpr float WordsGap = 48.f * Grow;
		constexpr float NumberGap = 30.f * Grow;
		constexpr float TagGap = 22.f * Grow;
		constexpr float TagWidth = 66.f * Grow;
		constexpr float TagHeight = 24.f * Grow;
		constexpr float DashHalf = 10.f * Grow;
		constexpr float GoldDashFrom = 40.f * Grow;
		constexpr float GoldDashTo = 26.f * Grow;
		/** The reticle on the lit row's arc, and the gold run on the arc round it (24 each way). */
		constexpr float ReticleRadius = 19.f * Grow;
		constexpr float LitRunHalf = 24.f * Grow;
		constexpr int32 WordFont = 28;
		constexpr int32 WordSpacing = 214;
		constexpr int32 NumberFont = 14;
		constexpr int32 NumberSpacing = 71;
		constexpr int32 TagFont = 12;
		constexpr int32 TagSpacing = 250;

		// --- The arrival's timing (c3's note: the halo spins up 90 degrees in 0.25 s as its ticks light clockwise, then the
		// words fade in), seconds from its start before Looter.Menu.Reveal.Speed ---

		constexpr float HaloDelay = 0.15f;
		constexpr float HaloSpinTime = 0.25f;
		constexpr float HaloTurn = 90.f;
		constexpr float WordsDelay = HaloDelay + HaloSpinTime;
		/** The rows one after another, quick enough to read as one movement; the arc and the reticle's layer with the first. */
		constexpr float FirstRowDelay = 0.5f;
		constexpr float RowStagger = 0.07f;

		/** Row Index's middle, down the list's box. */
		constexpr float RowY(int32 Index) { return RowTop + RowHeight * (Index + 0.5f); }

		/** Where the arc is at Y down the list's box (it bows left at the circle's middle). */
		inline float ArcX(float Y)
		{
			return ArcCentreX - FMath::Sqrt(FMath::Max(FMath::Square(ArcRadius) - FMath::Square(Y - ArcCentreY), 0.f));
		}

		/** The arc's angle at Y (LooterReticle's degrees: 180 at the circle's middle, more going up). */
		inline float ArcDegrees(float Y)
		{
			return 180.f - FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp((Y - ArcCentreY) / ArcRadius, -1.f, 1.f)));
		}
	}

	/**
	 * The game's logo mark in the title's halo. One function, so the mark the user picks (2026-10-29: "change the logo in
	 * the top left") drops in here alone. For now the earlier title mark in Slate & Salt: an ember standing through a
	 * coin's broken ring, a horizon line running out of it both ways (a ferryman's coin, Docs/Story.md, split by the ember
	 * a Revenant carries). 64 x 64 view box.
	 */
	LooterUI::FPaintedIcon LogoMark();

	/**
	 * The shade the menu sits on: the kit's backdrop dark at the left edge, holding through the menu's side, then thinning
	 * to nothing, so the planet on the right is untouched. Stretched over the shade's whole box; 64 x 4 view box.
	 */
	const LooterUI::FPaintedIcon& Shade();

	/** The SOON tag's hollow salt outline, chamfered at two corners, for a tag Width x Height (view box that plus Pad). */
	LooterUI::FPaintedIcon TagOutline(float Width, float Height);

	/**
	 * The gold run on the range arc round the lit row (c3: the arc drawn gold, 24 px each way): drawn at the circle's middle
	 * (180 degrees); the widget turns it round the circle's centre, LitRunCentre in its view box, onto the lit row.
	 */
	LooterUI::FPaintedIcon LitRun(float Radius, float HalfDegrees);
	FVector2D LitRunCentre(float Radius, float HalfDegrees);

	/** A page's header in the menu's column ("SINGLE PLAYER"): a gold tick, the words, a thin salt rule out to the right. */
	UWidget* MakePageHeader(UWidgetTree* Tree, const FString& Title);

	/** A key hint at the bottom (c3: ENTER SELECT): the key as a hollow gold key, its words floating beside it. */
	UWidget* MakeKeyHint(UWidgetTree* Tree, const FString& Key, const FString& Text);
}
