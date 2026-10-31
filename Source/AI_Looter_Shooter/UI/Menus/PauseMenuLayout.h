#pragma once

#include "CoreMinimal.h"

struct FSlateFontInfo;

/**
 * The pause menu hub's layout and plain rules (E2 "Big Dial", Docs/Archive/Work/2026-10-30-PauseMenu/CONTRACT.md; the mockup is
 * mockups/E2b_hub.png from e.js). Units are the hub's design space: 1080 tall and at least 1920 wide (a wider screen
 * widens it and the right column keeps to the right edge; a narrower one scales the whole down to fit).
 *
 * The mockup's px are kept for the shapes, and its type sizes are Slate sizes as they are (CLAUDE.md UI rules: in game the
 * type comes out a third bigger than in the mockup). So the pieces round the type are spaced out from the mockup where the
 * bigger type needs it: the dial sits 24 further left (its centre 16 in from the edge, not 40), the tags are 480 wide (not
 * 430), the lit tag grows 10 and slides 20 (not 30 and 26), the cards are 458 wide 48 from the right edge, the session tag
 * 660 wide; the callout's and the mission card's heights follow their text (MeasureLines).
 */
namespace PauseMenuLayout
{
	inline constexpr float DesignWidth = 1920.f;
	inline constexpr float DesignHeight = 1080.f;
	/** The right column (the cards) and the key hints end here: 48 in from the right edge. */
	inline constexpr float RightEdge = 1872.f;
	inline constexpr int32 NumRows = 4;
	/** The pictures' texture resolution (twice their drawn size, as the kit asks). */
	inline constexpr float PictureResolution = 2.f;

	// --- The title, top-left: GAME PAUSED over a ruler, the area's name under it ---

	inline constexpr float TitleX = 96.f;
	/** The title's box (its line centred in TitleBand), the ruler's line, the area's box. */
	inline constexpr float TitleY = 62.f;
	inline constexpr float TitleBand = 96.f;
	inline constexpr int32 TitleFont = 52;
	inline constexpr int32 TitleSpacing = 240;
	inline constexpr float RulerY = 164.f;
	inline constexpr float RulerLength = 600.f;
	inline constexpr float RulerStep = 10.7f;
	inline constexpr int32 RulerLongEvery = 5;
	inline constexpr float AreaY = 176.f;
	inline constexpr float AreaBand = 36.f;
	inline constexpr int32 AreaFont = 17;
	inline constexpr int32 AreaSpacing = 500;

	// --- The dial, cut by the left edge, and its range arc ---

	inline constexpr float DialX = 16.f;
	inline constexpr float DialY = 560.f;
	inline constexpr float DialRadius = 232.f;
	inline constexpr float ArcRadius = 336.f;
	inline constexpr float ArcFrom = -58.f;
	inline constexpr float ArcTo = 58.f;
	/** A tick every 10.7 px along the arc, every fifth long (e.js arcScale). */
	inline constexpr float ArcTickPx = 10.7f;
	inline constexpr int32 ArcLongEvery = 5;
	/** The small reticle on the lit option's tick. */
	inline constexpr float ReticleRadius = 19.f;

	// --- The rows: each option's tick on the arc, its number past it, a peg out to its tag ---

	/** The rows' middles, top to bottom (QUIT GAME set a little apart, as in the mockup). */
	inline constexpr float RowY[NumRows] = { 328.f, 448.f, 568.f, 712.f };
	/** Along a row from its tick: the salt dash across the arc (13 each way), the short peg to 30, the number from 34 (40
	 *  lit, clear of the reticle), the long peg from 62 to the tag's gem. */
	inline constexpr float DashHalf = 13.f;
	inline constexpr float ShortPegTo = 30.f;
	inline constexpr float NumberGap = 34.f;
	inline constexpr float NumberLitGap = 40.f;
	inline constexpr float LongPegFrom = 62.f;
	inline constexpr int32 NumberFont = 19;
	inline constexpr int32 NumberSpacing = 70;

	// --- The tags ---

	inline constexpr float TagX = 452.f;
	inline constexpr float TagWidth = 480.f;
	inline constexpr float TagHeight = 76.f;
	inline constexpr float TagLitGrow = 10.f;
	inline constexpr float TagLitSlide = 20.f;
	inline constexpr float TagTextX = 62.f;
	inline constexpr int32 TagFont = 34;
	inline constexpr int32 TagSpacing = 110;
	/** The gem on the tag's notch: its size, its left from the tag's left, the icon in it. */
	inline constexpr float TagGem = 52.f;
	inline constexpr float TagGemX = -17.f;
	inline constexpr float TagIcon = 22.f;

	// --- The callout beside the lit tag, pointing back at it ---

	inline constexpr float CalloutGap = 20.f;
	inline constexpr float CalloutX = TagX + TagLitSlide + TagWidth + TagLitGrow + CalloutGap;
	inline constexpr float CalloutWidth = 422.f;
	/** Its point's tip from its top: level with the tag's middle. */
	inline constexpr float CalloutPointY = 52.f;
	inline constexpr float CalloutPadLeft = 50.f;
	inline constexpr float CalloutPadRight = 22.f;
	inline constexpr float CalloutPadTop = 22.f;
	inline constexpr float CalloutPadBottom = 24.f;
	inline constexpr float CalloutTextWidth = CalloutWidth - CalloutPadLeft - CalloutPadRight;
	inline constexpr int32 CalloutTitleFont = 23;
	inline constexpr int32 CalloutTitleSpacing = 160;
	inline constexpr int32 HelpFont = 23;
	inline constexpr int32 FlavorFont = 18;
	/** The ornament's room (10 above, 14 tall, 10 below) and the gap before the flavor line. */
	inline constexpr float OrnamentBlock = 34.f;
	inline constexpr float FlavorGap = 6.f;
	/** The flavor line's lean: Chakra Petch has no italic (Pipeline Game.md "The gun card"). */
	inline constexpr float ItalicShear = -12.f;

	// --- The right column: the hero card, the tracked mission under it ---

	inline constexpr float CardWidth = 458.f;
	inline constexpr float CardX = RightEdge - CardWidth;
	inline constexpr float HeroY = 64.f;
	inline constexpr float HeroHeight = 400.f;
	inline constexpr float CardGap = 22.f;
	inline constexpr float MissionY = HeroY + HeroHeight + CardGap;
	/** The mission card's room round its text: the top (under its gem's step), the bottom, the sides. */
	inline constexpr float MissionPadTop = 38.f;
	inline constexpr float MissionPadBottom = 24.f;
	inline constexpr float MissionPadSide = 28.f;
	inline constexpr float MissionTextWidth = CardWidth - MissionPadSide * 2.f;
	inline constexpr int32 MissionLabelFont = 16;
	inline constexpr int32 MissionCountFont = 20;
	inline constexpr int32 MissionNameFont = 30;
	inline constexpr int32 MissionLineFont = 22;
	inline constexpr int32 MissionOptionalFont = 18;
	inline constexpr int32 MissionLabelSpacing = 260;
	inline constexpr int32 MissionNameSpacing = 80;
	/** The gaps in the mission card: under the label row, under the name, before each optional line. */
	inline constexpr float MissionLabelGap = 6.f;
	inline constexpr float MissionNameGap = 8.f;
	inline constexpr float MissionOptionalGap = 4.f;

	// --- The session tag, bottom-left, and the key hints, bottom-right ---

	inline constexpr float SessionX = 110.f;
	inline constexpr float SessionWidth = 660.f;
	inline constexpr float SessionHeight = 108.f;
	inline constexpr float SessionY = DesignHeight - 84.f - SessionHeight;
	inline constexpr float KeysY = 1006.f;
	/** The key hints' room: about the widest bar the hub shows (ENTER SELECT, ESC RESUME at their in-game size). */
	inline constexpr float KeysWidth = 560.f;
	inline constexpr float KeysHeight = 44.f;

	// --- Motion ---

	/** The needle's swing to a newly lit option (the contract: about 0.18 s, a slight overshoot), and its first swing up as
	 *  the hub opens, from pointing down past the last option. */
	inline constexpr float SwingSeconds = 0.18f;
	inline constexpr float OpenSwingSeconds = 0.34f;
	inline constexpr float OpenSwingFrom = 80.f;

	// --- Rules ---

	/** Where row Row crosses the range arc (its tick). */
	FVector2D RowArcPoint(int32 Row);

	/** The needle's angle aimed at row Row's tick (LooterReticle's degrees: 0 at three o'clock, clockwise). */
	float RowDegrees(int32 Row);

	/** The needle's swing at Alpha (0 to 1) of its time: eased out with a slight overshoot (about 4%) and settling at 1. */
	float NeedleSwing(float Alpha);

	/** A box from its top-left and size. */
	FBox2D MakeBox(float X, float Y, float Width, float Height);

	/** Row Row's tag (bLit: grown and slid out), its gem included. */
	FBox2D TagBox(int32 Row, bool bLit);
	/** The callout of row Row, Height tall, its point included. */
	FBox2D CalloutBox(int32 Row, float Height);
	FBox2D HeroBox();
	FBox2D MissionBox(float Height);
	FBox2D SessionBox();
	FBox2D KeysBox();
	/** GAME PAUSED, its ruler and the area's name, TitleWidth being the title's measured width. */
	FBox2D TitleBox(float TitleWidth);
	/** The dial's ring and ticks, and its range arc. */
	FBox2D DialBox();

	// --- The type's room ---

	/** How many lines Text wraps to at WrapWidth in Font, as a text block wraps it (whole words). */
	int32 MeasureLines(const FString& Text, const FSlateFontInfo& Font, float WrapWidth);
	/** One line's height in Font. */
	float LineHeight(const FSlateFontInfo& Font);
	/** One line's width in Font. */
	float MeasureWidth(const FString& Text, const FSlateFontInfo& Font);

	/** A callout's height for its words: the title, the ornament, the help and the flavor line. */
	float CalloutHeight(const FString& Help, const FString& Flavor);

	/** The mission card's height for its step line and its optional lines (none: no optional). */
	float MissionHeight(const FString& Line, const TArray<FString>& Optionals);
}
