#pragma once

#include "CoreMinimal.h"
#include "Creatures/CreatureRank.h"

struct FTierDefeats;

/**
 * The line under a bestiary page's defeat count (2026-10-20, with the enemy tiers; Docs/Story.md, "Enemy ranks and
 * legendary drops"): which tiers of this kind the player has defeated and how many, in each tier's colour, such as
 * "Restless 3 · Gravebound 1 · Hallowed 1". Only tiers met are listed, lowest first; Regular is left out, since the count
 * above it is mostly Regular and says it. Plain rules, so the words and colours are tested without a widget; the page
 * draws the pieces (BestiaryWidgetDetails.cpp).
 */
namespace BestiaryTiers
{
	/** One tier on the line. */
	struct FTierCount
	{
		ECreatureRank Tier = ECreatureRank::Rare;
		int32 Count = 0;
		/** The word its tag shows ("Restless"), from the rank settings. */
		FString Word;
		/** The tier's colour, the tag's own. */
		FLinearColor Color = FLinearColor::White;
		/** A small mark in front: the two top tiers, Hallowed and Ethereal, whose first defeat is an event. */
		bool bMark = false;
	};

	/** What stands between two tiers on the line. */
	inline const TCHAR* const Separator = TEXT(" · ");

	/** The pieces for Defeats: each tier above Regular with a defeat, lowest tier first. Empty when there are none. */
	TArray<FTierCount> Pieces(const FTierDefeats& Defeats);

	/** "Restless 3 · Gravebound 1 · Hallowed 1": the pieces as words alone; empty when there are none. */
	FString Words(const TArray<FTierCount>& Pieces);

	/** One piece as words: "Hallowed 1". */
	FString PieceWords(const FTierCount& Piece);
}
