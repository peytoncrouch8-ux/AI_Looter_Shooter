#include "UI/Cards/WardCardIcons.h"

// The ward card's glyphs: strokes as polylines (and the spark a convex fill, clockwise on screen) in a 16-unit box, as
// GunCardIcons draws the gun card's.

using LooterUI::FVectorIcon;
using WardCardRules::EStat;

namespace
{
	FVectorIcon Make(float Stroke)
	{
		FVectorIcon Icon;
		Icon.ViewBox = FVector2D(16.0, 16.0);
		Icon.StrokeWidth = Stroke;
		return Icon;
	}

	FVectorIcon MakeCapacity()
	{
		// A ward's shield outline: what it holds.
		FVectorIcon Icon = Make(1.8f);
		Icon.Strokes = { { {8, 1}, {14, 3.5}, {13.2, 10}, {8, 15}, {2.8, 10}, {2, 3.5}, {8, 1} } };
		return Icon;
	}

	FVectorIcon MakeDelay()
	{
		// An hourglass: the wait before it fills.
		FVectorIcon Icon = Make(1.8f);
		Icon.Strokes = { { {3, 1.5}, {13, 1.5}, {8, 8}, {13, 14.5}, {3, 14.5}, {8, 8}, {3, 1.5} } };
		return Icon;
	}

	FVectorIcon MakeRate()
	{
		// Two chevrons rising: how fast it fills.
		FVectorIcon Icon = Make(2.f);
		Icon.Strokes = { { {3, 8.5}, {8, 3.5}, {13, 8.5} }, { {3, 14}, {8, 9}, {13, 14} } };
		return Icon;
	}

	FVectorIcon MakeBuild()
	{
		// The belt box and its handle.
		FVectorIcon Icon = Make(1.7f);
		Icon.Strokes = { { {2.5, 5}, {13.5, 5}, {13.5, 14.5}, {2.5, 14.5}, {2.5, 5} }, { {5.5, 5}, {5.5, 2}, {10.5, 2}, {10.5, 5} } };
		return Icon;
	}

	FVectorIcon MakeTrait()
	{
		// A four-point spark: its one special thing.
		FVectorIcon Icon = Make(1.6f);
		Icon.Fills = { { {8, 1}, {10, 6}, {15, 8}, {10, 10}, {8, 15}, {6, 10}, {1, 8}, {6, 6} } };
		return Icon;
	}
}

const LooterUI::FVectorIcon& WardCardIcons::Stat(EStat Stat)
{
	static const FVectorIcon Capacity = MakeCapacity();
	static const FVectorIcon Delay = MakeDelay();
	static const FVectorIcon Rate = MakeRate();
	switch (Stat)
	{
	case EStat::Delay: return Delay;
	case EStat::Rate:  return Rate;
	default:           return Capacity;
	}
}

FName WardCardIcons::StatName(EStat Stat)
{
	switch (Stat)
	{
	case EStat::Delay: return FName(TEXT("WardCardStat_Delay"));
	case EStat::Rate:  return FName(TEXT("WardCardStat_Rate"));
	default:           return FName(TEXT("WardCardStat_Capacity"));
	}
}

const LooterUI::FVectorIcon& WardCardIcons::Build()
{
	static const FVectorIcon Icon = MakeBuild();
	return Icon;
}

const LooterUI::FVectorIcon& WardCardIcons::Trait()
{
	static const FVectorIcon Icon = MakeTrait();
	return Icon;
}
