#include "Bestiary/BestiaryTiers.h"
#include "Creatures/CreatureRankSettings.h"
#include "Creatures/CreatureTierLook.h"
#include "Progression/PlayerProgressData.h"

TArray<BestiaryTiers::FTierCount> BestiaryTiers::Pieces(const FTierDefeats& Defeats)
{
	TArray<FTierCount> Result;
	// LooterRanks::Tiers is in order, lowest first; the first is Regular, which the line leaves to the count above it.
	for (const ECreatureRank Tier : LooterRanks::Tiers)
	{
		const int32 Count = Defeats.Count(Tier);
		if (Tier == ECreatureRank::Basic || Count <= 0)
		{
			continue;
		}
		FTierCount& Piece = Result.AddDefaulted_GetRef();
		Piece.Tier = Tier;
		Piece.Count = Count;
		Piece.Word = UCreatureRankSettings::GetTierName(Tier);
		Piece.Color = CreatureTierLook::TagColor(Tier);
		Piece.bMark = CreatureTierLook::IsTopTier(Tier);
	}
	return Result;
}

FString BestiaryTiers::PieceWords(const FTierCount& Piece)
{
	return FString::Printf(TEXT("%s %s"), *Piece.Word, *FText::AsNumber(Piece.Count).ToString());
}

FString BestiaryTiers::Words(const TArray<FTierCount>& Pieces)
{
	FString Line;
	for (const FTierCount& Piece : Pieces)
	{
		Line += (Line.IsEmpty() ? FString() : FString(Separator)) + PieceWords(Piece);
	}
	return Line;
}
