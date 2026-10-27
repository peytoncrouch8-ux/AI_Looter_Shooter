// UMinimapSubsystem's roads (the user's note 16, 2026-10-27: "the map doesn't really prompt the player to use roads, it
// feels like a free for all"): every AMapRoad painted into the bake, so the HUD minimap and the map page show the way.
#include "World/MinimapSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "World/MapRoad.h"
#include "Engine/World.h"
#include "EngineUtils.h"

namespace
{
	// The minimap's land is teal (MinimapPaint): the roads are warm so they stand out of it at a glance. A road is packed
	// sand with a dark edge (it reads as a band even where the land's tint is light), a path paler and thinner with no
	// edge (it's a lesser way), a rail dark steel.
	const FColor RoadFill(206, 176, 122, 245);
	const FColor RoadEdge(84, 66, 44, 240);
	const FColor PathFill(184, 164, 126, 235);
	const FColor RailFill(56, 60, 68, 245);

	/** Distance (texels) from P to the segment A-B. */
	float SegmentDistance(const FVector2D& P, const FVector2D& A, const FVector2D& B)
	{
		const FVector2D D = B - A;
		const double Length2 = D.SizeSquared();
		const double T = Length2 > 1e-8 ? FMath::Clamp(FVector2D::DotProduct(P - A, D) / Length2, 0.0, 1.0) : 0.0;
		return static_cast<float>(FVector2D::Distance(P, A + D * T));
	}

	FColor Mix(const FColor& From, const FColor& To, float Amount)
	{
		const float A = FMath::Clamp(Amount, 0.f, 1.f);
		auto Channel = [A](uint8 X, uint8 Y) { return static_cast<uint8>(FMath::RoundToInt(FMath::Lerp(float(X), float(Y), A))); };
		return FColor(Channel(From.R, To.R), Channel(From.G, To.G), Channel(From.B, To.B), FMath::Max(From.A, Channel(From.A, To.A)));
	}
}

FColor UMinimapSubsystem::RoadFillColor(EMapRoadKind Kind)
{
	switch (Kind)
	{
	case EMapRoadKind::Path:
		return PathFill;
	case EMapRoadKind::Rail:
		return RailFill;
	default:
		return RoadFill;
	}
}

FColor UMinimapSubsystem::RoadEdgeColor()
{
	return RoadEdge;
}

void UMinimapSubsystem::PaintRoadLines(TArray<FColor>& Colors, int32 N, const FBox2D& MapBounds,
	TConstArrayView<FMinimapRoadLine> Lines, TConstArrayView<EMinimapTexel> Kinds)
{
	if (N <= 0 || Colors.Num() != N * N || Lines.Num() == 0)
	{
		return;
	}
	const double CmPerTexel = FMath::Max(MapBounds.GetSize().X, 1.0) / N;
	const bool bKinds = Kinds.Num() == N * N;

	// Every texel's distance (texels) past the nearest road's side (negative inside it) and that road's kind: the union
	// of all the roads, so where two meet they're one shape and the edge runs only round its outside.
	constexpr float Far = TNumericLimits<float>::Max();
	TArray<float> Past;
	Past.Init(Far, N * N);
	TArray<uint8> KindOf;
	KindOf.Init(0, N * N);
	for (const FMinimapRoadLine& Line : Lines)
	{
		if (Line.Points.Num() < 2)
		{
			continue;
		}
		const float Least = (Line.Kind == EMapRoadKind::Path ? MinPathTexels : MinRoadTexels) * 0.5f;
		const float Half = FMath::Max(static_cast<float>(Line.Width * 0.5 / CmPerTexel), Least);
		const float Reach = Half + (Line.Kind == EMapRoadKind::Road ? RoadEdgeTexels : 0.f) + 1.f;
		TArray<FVector2D> Texels;
		Texels.Reserve(Line.Points.Num());
		for (const FVector2D& Point : Line.Points)
		{
			Texels.Add(WorldToMapUV(MapBounds, FVector(Point.X, Point.Y, 0.0)) * N);
		}
		for (int32 S = 0; S + 1 < Texels.Num(); ++S)
		{
			const FVector2D& A = Texels[S];
			const FVector2D& B = Texels[S + 1];
			const int32 U0 = FMath::Max(FMath::FloorToInt(FMath::Min(A.X, B.X) - Reach), 0);
			const int32 U1 = FMath::Min(FMath::CeilToInt(FMath::Max(A.X, B.X) + Reach), N - 1);
			const int32 V0 = FMath::Max(FMath::FloorToInt(FMath::Min(A.Y, B.Y) - Reach), 0);
			const int32 V1 = FMath::Min(FMath::CeilToInt(FMath::Max(A.Y, B.Y) + Reach), N - 1);
			for (int32 V = V0; V <= V1; ++V)
			{
				for (int32 U = U0; U <= U1; ++U)
				{
					const int32 Index = V * N + U;
					const float Side = SegmentDistance(FVector2D(U + 0.5, V + 0.5), A, B) - Half;
					if (Side < Past[Index])
					{
						Past[Index] = Side;
						KindOf[Index] = static_cast<uint8>(Line.Kind);
					}
				}
			}
		}
	}

	for (int32 Index = 0; Index < N * N; ++Index)
	{
		const float Side = Past[Index];
		if (Side == Far || Colors[Index].A == 0 || (bKinds && Kinds[Index] != EMinimapTexel::Ground))
		{
			continue;
		}
		const EMapRoadKind Kind = static_cast<EMapRoadKind>(KindOf[Index]);
		// A texel's coverage from how far past the side its center is (half a texel either way: smooth edges).
		FColor Painted = Colors[Index];
		if (Kind == EMapRoadKind::Road)
		{
			Painted = Mix(Painted, RoadEdge, RoadEdgeTexels + 0.5f - Side);
		}
		Colors[Index] = Mix(Painted, RoadFillColor(Kind), 0.5f - Side);
	}
}

void UMinimapSubsystem::PaintRoads(TArray<FColor>& Colors) const
{
	TArray<FMinimapRoadLine> Lines;
	for (TActorIterator<AMapRoad> It(GetWorld()); It; ++It)
	{
		const AMapRoad* Road = *It;
		if (Road->Points.Num() < 2)
		{
			continue;
		}
		FMinimapRoadLine& Line = Lines.AddDefaulted_GetRef();
		Line.Points = Road->Points;
		Line.Width = Road->Width;
		Line.Kind = Road->Kind;
	}
	PaintRoadLines(Colors, Resolution, Bounds, Lines, Kinds);
	UE_LOG(LogLooter, Log, TEXT("Minimap: %d roads"), Lines.Num());
}
