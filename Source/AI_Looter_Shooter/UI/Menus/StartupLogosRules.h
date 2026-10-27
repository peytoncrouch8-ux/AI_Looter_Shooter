#pragma once

#include "CoreMinimal.h"

/**
 * How long each part of the startup cards lasts (UStartupLogosWidget). Every number is a console variable
 * (Looter.Startup.*), read when the cards start: FromConsole.
 */
struct AI_LOOTER_SHOOTER_API FStartupLogoTiming
{
	/** Dark before the first card: the window and the menu's world settle (a first frame often hitches). */
	float LeadIn = 0.5f;
	float FadeIn = 0.6f;
	/** Long enough to read a name and hear its sting out, short enough that nobody reaches for a key. */
	float Hold = 2.2f;
	float FadeOut = 0.5f;
	/** Dark between two cards, so they read as two. */
	float Gap = 0.3f;
	/** A card cut short by a key goes this fast, from wherever its fade had got to. */
	float SkipFadeOut = 0.2f;
	/** A card can't be skipped before it has shown this long, so a key still held from launching never skips two. */
	float MinShown = 0.35f;
	/** The dark cover's fade off the menu after the last card (the menu arrives under it). */
	float Reveal = 0.9f;

	static FStartupLogoTiming FromConsole();
};

enum class EStartupLogoPhase : uint8
{
	LeadIn,
	FadeIn,
	Hold,
	FadeOut,
	Gap,
	/** The cards are done; the cover fades off the menu. */
	Reveal,
	Done,
};

/**
 * The startup cards' clock, apart from any widget: which card shows and how strongly, how dark the cover is, and what a
 * key does (the user, 2026-10-27: "any key skips one card; first launch shows all"). A skippable run cuts the card on
 * screen short (a key during the dark gap skips the card coming); a run that isn't skippable (the first launch) plays
 * every card in full. Cards fade in, hold and fade out one after another, then the cover fades off the menu.
 */
class AI_LOOTER_SHOOTER_API FStartupLogos
{
public:
	/** From the start, with NumCards cards (none: straight to the cover's fade). */
	void Start(int32 InNumCards, bool bInSkippable, const FStartupLogoTiming& InTiming);

	void Tick(float DeltaSeconds);

	/**
	 * A key or a click: cuts the card on screen short, or (in the dark between two) skips the card coming. False, and
	 * nothing, when the run isn't skippable, before the first card, in a card's first MinShown seconds or once the cards
	 * are done.
	 */
	bool Skip();

	EStartupLogoPhase GetPhase() const { return Phase; }
	/** The card showing or about to (INDEX_NONE before the first and once they're done). */
	int32 GetCard() const;
	/** How strongly the card shows, 0 to 1. */
	float GetCardOpacity() const;
	/** How far the card is through its whole show, 0 to 1 (its slow drift). */
	float GetCardProgress() const;
	/** How dark the cover over the menu is, 0 to 1: full until the cards are done, then it fades. */
	float GetCover() const;
	bool IsSkippable() const { return bSkippable; }
	bool IsRevealing() const { return Phase == EStartupLogoPhase::Reveal || Phase == EStartupLogoPhase::Done; }
	bool IsDone() const { return Phase == EStartupLogoPhase::Done; }

	/** The card whose fade-in began since this was last asked (its sting plays), or INDEX_NONE. */
	int32 TakeStartedCard();

	/** Every second of a run with no keys pressed, the lead-in to the cover gone. */
	static float FullLength(int32 InNumCards, const FStartupLogoTiming& InTiming);

private:
	float PhaseLength() const;
	void Enter(EStartupLogoPhase NewPhase);
	/** After a card's fade-out: the next card's gap, or the cover's fade. */
	void AfterCard();

	FStartupLogoTiming Timing;
	EStartupLogoPhase Phase = EStartupLogoPhase::Done;
	int32 NumCards = 0;
	int32 Card = INDEX_NONE;
	float PhaseTime = 0.f;
	/** Seconds the card on screen has shown, its fade-in included. */
	float CardTime = 0.f;
	/** The opacity the fade-out starts from (below 1 when a key cut the fade-in short). */
	float FadeFrom = 1.f;
	/** This fade-out was a key's: SkipFadeOut long. */
	bool bCutShort = false;
	bool bSkippable = true;
	int32 StartedCard = INDEX_NONE;
};
