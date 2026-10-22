// LootLabelLayout: loot labels on screen moved up or down, the least they can, so none draws over another.

#include "UI/World/LootLabelLayout.h"

namespace
{
	/** Rounding slack (widget units): a box put exactly Gap above another isn't counted as crowding it. */
	constexpr double OverlapSlack = 0.01;

	FBox2D BoxAt(const FLootLabelBox& Label, double Shift)
	{
		const FVector2D Min(Label.Anchor.X - Label.Size.X * 0.5, Label.Anchor.Y - Label.Size.Y + Shift);
		const FVector2D Max(Label.Anchor.X + Label.Size.X * 0.5, Label.Anchor.Y + Shift);
		return FBox2D(Min, Max);
	}

	bool Across(const FBox2D& A, const FBox2D& B)
	{
		return A.Min.X < B.Max.X && B.Min.X < A.Max.X;
	}
}

bool LootLabelLayout::Crowds(const FBox2D& A, const FBox2D& B, float Gap)
{
	const double Room = FMath::Max(Gap, 0.f) - OverlapSlack;
	return Across(A, B) && A.Min.Y < B.Max.Y + Room && B.Min.Y < A.Max.Y + Room;
}

TArray<float> LootLabelLayout::Arrange(TConstArrayView<FLootLabelBox> Boxes, float Gap, float KeepWeight)
{
	TArray<float> Shifts;
	Shifts.SetNumZeroed(Boxes.Num());

	// The focused label first (it stays put), then the nearest: a near label is the bigger and the likelier to be read.
	TArray<int32> Order;
	for (int32 Index = 0; Index < Boxes.Num(); ++Index)
	{
		Order.Add(Index);
	}
	Order.StableSort([&Boxes](int32 A, int32 B)
	{
		if (Boxes[A].bFocused != Boxes[B].bFocused)
		{
			return Boxes[A].bFocused;
		}
		return Boxes[A].Distance < Boxes[B].Distance;
	});

	const double Space = FMath::Max(Gap, 0.f);
	TArray<FBox2D> Placed;
	for (const int32 Index : Order)
	{
		const FLootLabelBox& Label = Boxes[Index];
		const FBox2D Home = BoxAt(Label, 0.0);
		// Where it may go: where it is, or just above or just below each placed label it shares a column with.
		TArray<double, TInlineAllocator<16>> Candidates;
		Candidates.Add(0.0);
		for (const FBox2D& Other : Placed)
		{
			if (Across(Home, Other))
			{
				Candidates.Add(Other.Min.Y - Space - Home.Max.Y);
				Candidates.Add(Other.Max.Y + Space - Home.Min.Y);
			}
		}
		const auto Cost = [&Label, KeepWeight](double Shift)
		{
			return FMath::Abs(Shift) + FMath::Max(KeepWeight, 0.f) * FMath::Abs(Shift - Label.Previous);
		};
		Candidates.Sort([&Cost](double A, double B) { return Cost(A) < Cost(B); });

		// Over every placed label as a last resort (a clear place always exists there).
		double Highest = Home.Min.Y;
		for (const FBox2D& Other : Placed)
		{
			Highest = FMath::Min(Highest, Other.Min.Y);
		}
		double Chosen = Highest - Space - Home.Max.Y;
		for (const double Shift : Candidates)
		{
			const FBox2D Moved = BoxAt(Label, Shift);
			if (!Placed.ContainsByPredicate([&Moved, Gap](const FBox2D& Other) { return Crowds(Moved, Other, Gap); }))
			{
				Chosen = Shift;
				break;
			}
		}
		Shifts[Index] = static_cast<float>(Chosen);
		Placed.Add(BoxAt(Label, Chosen));
	}
	return Shifts;
}
