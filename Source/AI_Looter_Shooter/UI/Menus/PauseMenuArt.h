#pragma once
#include "CoreMinimal.h"
#include "UI/Style/LooterUIStyle.h"

/** The pause menu's and the settings page's pictures (E2 "Big Dial"), in LooterReticle's conventions: view-box px, y down,
 *  angles in degrees with 0 at three o'clock growing clockwise, Pad round every line, drawn with a dark edge under. */
namespace PauseMenuArt
{
	/** The dial without the rowel: salt ring of Radius (ticks every 10 deg, long at the four cross points), a fine inner ring
	 *  with inward ticks, a slim gold needle aimed at NeedleDeg on a hollow salt pivot with a gold dot, a short salt tail,
	 *  and a short gold arc inside the ring centred on NeedleDeg. Square view box 2 x DialExtent(Radius), centre in the middle. */
	LooterUI::FPaintedIcon Dial(float Radius, float NeedleDeg);
	float DialExtent(float Radius);

	/** How a tag or pennant reads. */
	enum class ELook : uint8 { Plain, Lit, Danger };

	/**
	 * Which part of a shape a picture holds (an addition to the contract; every call defaults to Whole, as written there). A
	 * shape's glass is a background and must fade with the UI transparency while its lines stay solid (CLAUDE.md UI rules):
	 * paint Fill in an image marked with LooterUI::MarkBackground and Lines in a second image over it, same place and size.
	 */
	enum class EPart : uint8 { Whole, Fill, Lines };

	/** C's option tag laid on its side, Width x Height: the gem notch at the left, cut shoulders, the point at the right. Fill
	 *  glass, edge salt (Plain), gold with a gold wash (Lit) or red (Danger). View box Width + 2 Pad by Height + 2 Pad. */
	LooterUI::FPaintedIcon Tag(float Width, float Height, ELook Look, EPart Part = EPart::Whole);

	/** A card in the gun card's shape, Width x Height: cut corners, a notch on top holding a diamond gem (bTopGem), and, with
	 *  bPointLeft, a point on its left edge at PointY (the callout pointing back at its tag). Glass fill, salt edge.
	 *  View box Width + 2 Pad by Height + 2 Pad; the card's own box starts at (Pad, Pad), its top edge (under the notch's
	 *  raised step) CardTopStep below that when bTopGem. */
	LooterUI::FPaintedIcon Card(float Width, float Height, bool bTopGem, bool bPointLeft, float PointY, EPart Part = EPart::Whole);

	/** A settings tab hanging from the ruler: the gun card's pennant, Width x Height, its diamond hook on top.
	 *  View box Width + 2 Pad by Height + PennantHookRise + 2 Pad: the hook's diamond reaches PennantHookRise above the body's
	 *  box, which starts at (Pad, Pad + PennantHookRise). */
	LooterUI::FPaintedIcon Pennant(float Width, float Height, ELook Look, EPart Part = EPart::Whole);

	/** A diamond gem of Size holding an icon (the tags' and cards' gems); bLit draws it gold. View box Size x Size (its line
	 *  kept inside). */
	LooterUI::FPaintedIcon Gem(float Size, bool bLit, EPart Part = EPart::Whole);

	// --- Additions for the hub (HUB's; the settings page may use them too) ---

	/** How far the raised step holding a card's top gem rises over the card's top edge (bTopGem), and the pennant's hook. */
	inline constexpr float CardTopStep = 12.f;
	inline constexpr float PennantHookRise = 16.f;

	/** A gem in any look (Danger: a red line), the danger option's gem. */
	LooterUI::FPaintedIcon GemLook(float Size, ELook Look, EPart Part = EPart::Whole);

	/** A seal: a dark gem with a gold line LineWidth wide (the hero card's level gem, the mission card's), Size x Size. */
	LooterUI::FPaintedIcon GoldGem(float Size, float LineWidth);

	/**
	 * The hero card and mission card: the gun card's tag standing up, Width x Height. A raised step NotchHalf each side of the
	 * middle along the top (the gem's seat), cut shoulders, and a point PointDepth deep at the foot (0: a cut-cornered foot).
	 * The line LineWidth wide at LineOpacity. View box as Card's.
	 */
	LooterUI::FPaintedIcon TagCard(float Width, float Height, float NotchHalf, float PointDepth, float LineWidth, float LineOpacity,
		EPart Part = EPart::Whole);

	/** The callout's and help card's rule: salt hairlines either side of a small gold diamond. Width x 14, view box that. */
	LooterUI::FPaintedIcon Ornament(float Width);

	/**
	 * The dial's still parts (Dial without its needle and gold arc), in Dial's view box: the face that stays while the
	 * needle swings over it.
	 */
	LooterUI::FPaintedIcon DialFace(float Radius);

	/**
	 * What swings with the dial's aim, aimed at 0 deg (three o'clock), to be turned round the dial's centre: the needle with
	 * its tail and pivot, the gold arc inside the ring, and, when ArcRadius > 0, the gold lead from the ring's ticks out to
	 * just short of the range arc of ArcRadius and the gold run on that arc (32 px each way). A strip just big enough;
	 * DialNeedleCentre says where the dial's centre is in it (the turn's pivot).
	 */
	LooterUI::FPaintedIcon DialNeedle(float Radius, float ArcRadius);
	FVector2D DialNeedleCentre(float Radius, float ArcRadius);

	/**
	 * The hub's backdrop over the paused game: dark at the left edge, thinning across to the right, with a soft vignette
	 * round the screen's edges (the mockup's shade and vignette). A small field (96 x 54) painted at 1 px a unit, to be
	 * stretched over the whole screen: the texture's filtering makes the gradients smooth.
	 */
	LooterUI::FPaintedIcon Backdrop();

	/** A slanted pip of the hero card's soul flask and grenade rows (W 10, H 18 and a 5 px lean), lit in Tint or dark. */
	LooterUI::FPaintedIcon Pip(const FLinearColor& Tint, bool bLit);
	inline constexpr float PipWidth = 10.f;
	inline constexpr float PipHeight = 18.f;
	inline constexpr float PipLean = 5.f;

	/** A mission step's diamond (16 px): 0 done (salt), 1 the current one (gold), 2 still to come (hollow). */
	LooterUI::FPaintedIcon StepPip(int32 State);
	inline constexpr float StepPipSize = 16.f;

	/** The hero card's slim slanted bar (the HUD's): its dark track with a faint salt edge, or its fill alone (Tint), Width x
	 *  Height, view box that size. */
	LooterUI::FPaintedIcon SlantBar(float Width, float Height, bool bFill, const FLinearColor& Tint);

	/** The portrait's medallion: a diamond bezel (gunmetal under a salt line, a faint salt ring inside), Size across, hollow.
	 *  View box Size + 2 Pad, the diamond's box at (Pad, Pad). */
	LooterUI::FPaintedIcon Medallion(float Size);
}
