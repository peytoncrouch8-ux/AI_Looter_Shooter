#include "World/ChickenYardRules.h"

namespace
{
	/** How finely a way is walked to check it (cm): well under a body's room, so no corner is cut unseen. */
	constexpr double WayStep = 4.0;

	/** How many of the best-scored spots a run's way is looked for (each a few checks): enough to find one round a block. */
	constexpr int32 RunTries = 12;

	/** Another bird nearer a spot than this (cm) makes it a worse place to run to. */
	constexpr double CrowdRoom = 30.0;

	/** Within a block's frame: P relative to its middle, turned back by its yaw. */
	FVector2D InBlock(const FChickenPenBlock& Block, const FVector2D& P)
	{
		const FVector2D Offset = P - Block.Center;
		const double Radians = FMath::DegreesToRadians(static_cast<double>(Block.Yaw));
		const double C = FMath::Cos(Radians);
		const double S = FMath::Sin(Radians);
		return FVector2D(Offset.X * C + Offset.Y * S, -Offset.X * S + Offset.Y * C);
	}

	FVector2D FromBlock(const FChickenPenBlock& Block, const FVector2D& Local)
	{
		const double Radians = FMath::DegreesToRadians(static_cast<double>(Block.Yaw));
		const double C = FMath::Cos(Radians);
		const double S = FMath::Sin(Radians);
		return Block.Center + FVector2D(Local.X * C - Local.Y * S, Local.X * S + Local.Y * C);
	}

	double DistanceToSegment(const FVector2D& P, const FVector2D& A, const FVector2D& B)
	{
		const FVector2D AB = B - A;
		const double Length2 = AB.SizeSquared();
		const double T = Length2 > UE_SMALL_NUMBER ? FMath::Clamp(FVector2D::DotProduct(P - A, AB) / Length2, 0.0, 1.0) : 0.0;
		return FVector2D::Distance(P, A + AB * T);
	}

	bool InsidePolygon(const TArray<FVector2D>& Outline, const FVector2D& P)
	{
		bool bInside = false;
		for (int32 I = 0, J = Outline.Num() - 1; I < Outline.Num(); J = I++)
		{
			const FVector2D& A = Outline[I];
			const FVector2D& B = Outline[J];
			if ((A.Y > P.Y) != (B.Y > P.Y) && P.X < (B.X - A.X) * (P.Y - A.Y) / (B.Y - A.Y) + A.X)
			{
				bInside = !bInside;
			}
		}
		return bInside;
	}
}

namespace ChickenPens
{
	bool IsInside(const FChickenPen& Pen, const FVector2D& P, float Margin)
	{
		if (Pen.Outline.Num() < 3 || !InsidePolygon(Pen.Outline, P))
		{
			return false;
		}
		for (int32 I = 0, J = Pen.Outline.Num() - 1; I < Pen.Outline.Num(); J = I++)
		{
			if (DistanceToSegment(P, Pen.Outline[J], Pen.Outline[I]) < Margin)
			{
				return false;
			}
		}
		for (const FChickenPenBlock& Block : Pen.Blocks)
		{
			const FVector2D Local = InBlock(Block, P);
			if (FMath::Abs(Local.X) < Block.HalfSize.X + Margin && FMath::Abs(Local.Y) < Block.HalfSize.Y + Margin)
			{
				return false;
			}
		}
		return true;
	}

	bool IsWayClear(const FChickenPen& Pen, const FVector2D& A, const FVector2D& B, float Margin)
	{
		const double Length = FVector2D::Distance(A, B);
		const int32 Steps = FMath::Max(1, FMath::CeilToInt(Length / WayStep));
		for (int32 Step = 0; Step <= Steps; ++Step)
		{
			if (!IsInside(Pen, FMath::Lerp(A, B, static_cast<double>(Step) / Steps), Margin))
			{
				return false;
			}
		}
		return true;
	}

	FBox2D Bounds(const FChickenPen& Pen)
	{
		FBox2D Box(ForceInit);
		for (const FVector2D& Corner : Pen.Outline)
		{
			Box += Corner;
		}
		return Box;
	}

	TArray<FVector2D> Spots(const FChickenPen& Pen, float Step, float Margin)
	{
		TArray<FVector2D> Found;
		const FBox2D Box = Bounds(Pen);
		if (!Box.bIsValid || Step <= 1.f)
		{
			return Found;
		}
		for (double X = Box.Min.X + Step * 0.5; X < Box.Max.X; X += Step)
		{
			for (double Y = Box.Min.Y + Step * 0.5; Y < Box.Max.Y; Y += Step)
			{
				const FVector2D P(X, Y);
				if (IsInside(Pen, P, Margin))
				{
					Found.Add(P);
				}
			}
		}
		return Found;
	}

	TArray<FVector2D> Corners(const FChickenPen& Pen, float Margin)
	{
		TArray<FVector2D> Found;
		for (const FChickenPenBlock& Block : Pen.Blocks)
		{
			for (const FVector2D& Sign : { FVector2D(1.0, 1.0), FVector2D(-1.0, 1.0), FVector2D(-1.0, -1.0), FVector2D(1.0, -1.0) })
			{
				// Out along the corner's diagonal by a little more than a body's room.
				const FVector2D Local = (Block.HalfSize + FVector2D(Margin + 6.0, Margin + 6.0)) * Sign;
				const FVector2D P = FromBlock(Block, Local);
				if (IsInside(Pen, P, Margin))
				{
					Found.Add(P);
				}
			}
		}
		return Found;
	}

	FVector2D Middle(const FChickenPen& Pen, float Margin)
	{
		const FBox2D Box = Bounds(Pen);
		const FVector2D Centre = Box.bIsValid ? Box.GetCenter() : FVector2D::ZeroVector;
		const TArray<FVector2D> Found = Spots(Pen, 10.f, Margin);
		FVector2D Best = Centre;
		double BestDistance = TNumericLimits<double>::Max();
		for (const FVector2D& P : Found)
		{
			const double Distance = FVector2D::DistSquared(P, Centre);
			if (Distance < BestDistance)
			{
				BestDistance = Distance;
				Best = P;
			}
		}
		return Best;
	}

	FFleeRun FindFleeRun(const FChickenPen& Pen, TConstArrayView<FVector2D> InSpots, TConstArrayView<FVector2D> InCorners,
		const FVector2D& From, const FVector2D& Threat, TConstArrayView<FVector2D> Others, float Margin, FRandomStream& Random)
	{
		// Every spot scored as the crow flies (far from the threat, not too far to run, away from the others, a little
		// chance), then the best few tried for a way there: straight, or round one of the blocks' corners.
		struct FScored
		{
			int32 Spot;
			double Score;
		};
		TArray<FScored> Scored;
		Scored.Reserve(InSpots.Num());
		for (int32 Index = 0; Index < InSpots.Num(); ++Index)
		{
			const FVector2D& P = InSpots[Index];
			const double Run = FVector2D::Distance(P, From);
			if (Run < 20.0)
			{
				continue;
			}
			double Score = FVector2D::Distance(P, Threat) - 0.25 * Run + Random.FRandRange(0.f, 15.f);
			for (const FVector2D& Other : Others)
			{
				Score -= FVector2D::Distance(P, Other) < CrowdRoom ? 60.0 : 0.0;
			}
			Scored.Add({ Index, Score });
		}
		Scored.Sort([](const FScored& A, const FScored& B) { return A.Score > B.Score; });

		FFleeRun Best;
		double BestScore = -TNumericLimits<double>::Max();
		const int32 Tries = FMath::Min(Scored.Num(), RunTries);
		for (int32 Try = 0; Try < Tries; ++Try)
		{
			const FVector2D& P = InSpots[Scored[Try].Spot];
			if (IsWayClear(Pen, From, P, Margin))
			{
				// Straight there: the best that is reached so is the run (the list is in order).
				if (Scored[Try].Score > BestScore)
				{
					Best.Target = P;
					Best.bVia = false;
					Best.bFound = true;
				}
				break;
			}
			for (const FVector2D& Corner : InCorners)
			{
				// Round a corner costs its extra length.
				const double Extra = FVector2D::Distance(From, Corner) + FVector2D::Distance(Corner, P) - FVector2D::Distance(From, P);
				const double Score = Scored[Try].Score - 0.25 * Extra;
				if (Score > BestScore && IsWayClear(Pen, From, Corner, Margin) && IsWayClear(Pen, Corner, P, Margin))
				{
					BestScore = Score;
					Best.Target = P;
					Best.Via = Corner;
					Best.bVia = true;
					Best.bFound = true;
				}
			}
		}
		return Best;
	}
}
