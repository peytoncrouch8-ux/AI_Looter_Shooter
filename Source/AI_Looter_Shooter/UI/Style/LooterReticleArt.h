#pragma once

#include "CoreMinimal.h"
#include "UI/Style/LooterUIStyle.h"

/**
 * Concept 3 "Halo Reticle"'s pictures (the user's pick for the main menu and half the pause menu, 2026-10-29): the gun's
 * sight as the motif, halo rings of ticks, rulers for rules, the choice held in a reticle. Each is a picture for
 * LooterUI::PaintedIconBrush in view-box px (y down), drawn like every kit line: the line (salt, or gold where the mockup
 * has it gold) over a dark edge, so it reads over a bright sky as well as over dark glass.
 *
 * Angles are degrees, 0 at three o'clock, growing clockwise on screen (the mockups' pt(): y down), so 90 is straight down.
 * Every picture leaves Pad round its lines so a dark edge is never clipped; the helpers say where the geometry lands in
 * its view box, so a caller can put a ring's centre or a ruler's line exactly where the mockup has it.
 *
 * Pictures are built each call (cheap); PaintedIconBrush draws a texture once per name and resolution, so give each
 * different picture its own brush name. Mockup: Docs/Work/2026-10-29-Morning/ui3/make_a.js (c3Pause, c3Menu).
 */
namespace LooterReticle
{
	/** Room round every picture's lines, view-box px. */
	constexpr float Pad = 4.f;

	// --- The halo: a ring with ticks outside it ---

	/**
	 * A ring of Radius with ticks outside it every TickStep degrees (from 0; TickStep <= 0: no ticks), every LongEvery-th
	 * tick long (0: none), optional inner cross hairs (four lines from the ring inward, with a gap in the middle, as round
	 * the pause bars), and a gold arc just inside the ring from GoldFrom to GoldTo degrees (equal: none). Square view box,
	 * 2 x HaloExtent(Radius) a side, the ring's centre in its middle.
	 */
	LooterUI::FPaintedIcon Halo(float Radius, float TickStep, int32 LongEvery, bool bCross, float GoldFrom, float GoldTo);

	/** Half the side of Halo's (and HaloTicks') view box: the ring's centre is at (HaloExtent, HaloExtent). */
	float HaloExtent(float Radius);

	/**
	 * Only Halo's ticks from FromDeg up to (not including) ToDeg, in the same view box, so pictures of a halo's sectors laid
	 * over Halo(Radius, 0, ...) can light one after another (the main menu's arrival: its ticks light clockwise).
	 */
	LooterUI::FPaintedIcon HaloTicks(float Radius, float TickStep, int32 LongEvery, float FromDeg, float ToDeg);

	// --- The ruler: a rule with range ticks ---

	/**
	 * A horizontal ruler Length long: a salt line with ticks down from it every Step, long ones every LongEvery-th (from
	 * the first). bDots (c3's mil-dot rule under GAME PAUSED): ticks crossing the line every LongEvery-th step instead,
	 * small dots on the line between. The line runs from x = Pad to Pad + Length at y = RulerLineY(bDots).
	 */
	LooterUI::FPaintedIcon Ruler(float Length, float Step, int32 LongEvery, bool bDots,
		const FLinearColor& Line = LooterUI::Color::SaltLine());

	/** Where Ruler's line runs in its view box, and the view box's height. */
	float RulerLineY(bool bDots);
	float RulerHeight(bool bDots);

	// --- The reticle bracket: half of "( )" round a chosen row ---

	/**
	 * c3's reticle bracket half "(" for a row Height tall: an arc bowing out, a short tick through its middle sticking out
	 * past it; bRight mirrors it ")". Gold (the choice) unless given. View box BracketWidth(Height) x (Height + 2 Pad).
	 */
	LooterUI::FPaintedIcon Bracket(float Height, bool bRight, const FLinearColor& Line = LooterUI::Color::SoulGold());

	float BracketWidth(float Height);
	/**
	 * Where the row's edge meets the bracket in its view box: the arc's outermost point, x from the left for "(" (from the
	 * right for ")"); the arc's middle is at y = Pad + Height / 2. Put the picture so this lands on the row's edge.
	 */
	float BracketEdgeX(float Height);

	// --- The small reticle: the mark on a chosen row ---

	/**
	 * A circle broken into four arcs, a tick through each gap, a dot in the middle (c3's Resume mark, the main menu's pick
	 * mark). Gold unless given. Square view box 2 x SmallReticleExtent(Radius) a side, centred; turn it as a widget.
	 */
	LooterUI::FPaintedIcon SmallReticle(float Radius, const FLinearColor& Line = LooterUI::Color::SoulGold());

	float SmallReticleExtent(float Radius);

	// --- The range arc: part of a big circle with ticks ---

	/**
	 * The part of a circle of Radius from FromDeg to ToDeg with ticks every TickStep degrees (from FromDeg), long ones every
	 * LongEvery-th (0: none); ticks point out of the circle, or in with bTicksIn (the main menu's range arc, its ticks
	 * toward the planet). The view box is the part's own bounds plus Pad: ArcScaleCentre (same arguments) says where the
	 * circle's centre lands in it (often far outside), to place the picture.
	 */
	LooterUI::FPaintedIcon ArcScale(float Radius, float FromDeg, float ToDeg, float TickStep, int32 LongEvery = 5,
		bool bTicksIn = false, const FLinearColor& Line = LooterUI::Color::SaltLine());

	FVector2D ArcScaleCentre(float Radius, float FromDeg, float ToDeg, float TickStep, int32 LongEvery = 5, bool bTicksIn = false);
}
