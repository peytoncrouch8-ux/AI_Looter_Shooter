#include "Player/Animation/BodySwing.h"
#include "ReferenceSkeleton.h"

// FBodySwing's ring: the cloth between neighbouring skirt chains kept off the limbs too (BodySwing.cpp steps the chains).

namespace
{
	/** The coat's skirt chains in order round the body, front left to front right (its open front isn't joined). */
	const TCHAR* const SkirtRing[] = { TEXT("skirt_fl"), TEXT("skirt_sl"), TEXT("skirt_bl"), TEXT("skirt_bli"), TEXT("skirt_bm"),
		TEXT("skirt_bri"), TEXT("skirt_br"), TEXT("skirt_sr"), TEXT("skirt_fr") };

	/**
	 * The most the cloth between two chains moves a point in a frame (cm, full size): a limb coming through can't fling it
	 * (at 20 a stride folded the back hem's corner over).
	 */
	constexpr double MaxRingPush = 12.0;

	/**
	 * Points[Level] moved by Delta and the points below re-hung from it, each bone keeping its length (the floor may have
	 * shortened the last); Points[Level - 1], or Anchor at the top, stays.
	 */
	void MoveLevel(TArray<FVector>& Points, const FVector& Anchor, int32 Level, const FVector& Delta)
	{
		TArray<double, TInlineAllocator<4>> Lengths;
		for (int32 Index = Level; Index < Points.Num(); ++Index)
		{
			Lengths.Add(FVector::Dist(Points[Index], Index > 0 ? Points[Index - 1] : Anchor));
		}
		const FVector Old = Points[Level];
		const FVector Head = Level > 0 ? Points[Level - 1] : Anchor;
		Points[Level] = Head + (Old + Delta - Head).GetSafeNormal(UE_SMALL_NUMBER, FVector::DownVector) * Lengths[0];
		for (int32 Index = Level + 1; Index < Points.Num(); ++Index)
		{
			Points[Index] = Points[Index - 1] + (Points[Index] - Points[Index - 1]).GetSafeNormal(UE_SMALL_NUMBER, FVector::DownVector)
				* Lengths[Index - Level];
		}
	}
}

void FBodySwing::BindRing(const FReferenceSkeleton& Skeleton, TConstArrayView<FTransform> Rest, TConstArrayView<FVector> LimbA,
	TConstArrayView<FVector> LimbB, TConstArrayView<float> LimbMax)
{
	Ring.Reset();
	TArray<int32> Order;
	for (const TCHAR* Name : SkirtRing)
	{
		const int32 Found = Chains.IndexOfByPredicate([Name](const FChain& Chain) { return Chain.Name == Name; });
		if (Found != INDEX_NONE)
		{
			Order.Add(Found);
		}
	}
	// A chain's points at rest (component space), its first bone's head before them.
	auto RestPoints = [&](const FChain& Chain)
	{
		TArray<FVector> Points;
		for (int32 Index = 0; Index < Chain.Bones.Num(); ++Index)
		{
			const int32 Bone = Skeleton.FindBoneIndex(Chain.Bones[Index].BoneName);
			if (Bone == INDEX_NONE)
			{
				return TArray<FVector>();
			}
			if (Index == 0)
			{
				Points.Add(Rest[Bone].GetLocation());
			}
			Points.Add(Rest[Bone].TransformPosition(Chain.TipInBone[Index]));
		}
		return Points;
	};
	auto Clearance = [&](const FVector& Place, int32 Limb)
	{
		const double Distance = FVector::Dist(Place, FMath::ClosestPointOnSegment(Place, LimbA[Limb], LimbB[Limb]));
		return FMath::Min(static_cast<float>(Distance) * ClearanceShare, LimbMax[Limb]);
	};
	for (int32 Next = 1; Next < Order.Num(); ++Next)
	{
		const TArray<FVector> PointsA = RestPoints(Chains[Order[Next - 1]]);
		const TArray<FVector> PointsB = RestPoints(Chains[Order[Next]]);
		if (PointsA.IsEmpty() || PointsB.IsEmpty())
		{
			continue;
		}
		FRingPair& Pair = Ring.AddDefaulted_GetRef();
		Pair.A = Order[Next - 1];
		Pair.B = Order[Next];
		const int32 Levels = FMath::Min(PointsA.Num(), PointsB.Num()) - 1;
		for (int32 Level = 0; Level < Levels; ++Level)
		{
			// Index 0 is the head: level L's points are at L + 1.
			const FVector Mid = (PointsA[Level + 1] + PointsB[Level + 1]) * 0.5;
			const FVector Center = (PointsA[Level] + PointsB[Level] + PointsA[Level + 1] + PointsB[Level + 1]) * 0.25;
			for (int32 Limb = 0; Limb < LimbA.Num(); ++Limb)
			{
				Pair.MidClearances.Add(Clearance(Mid, Limb));
				Pair.CenterClearances.Add(Clearance(Center, Limb));
			}
		}
	}
}

void FBodySwing::ApplyRing(TConstArrayView<FBodySwingCapsule> LimbsNow, double Scale)
{
	const int32 NumLimbs = LimbsNow.Num();
	for (const FRingPair& Pair : Ring)
	{
		FChain& A = Chains[Pair.A];
		FChain& B = Chains[Pair.B];
		const int32 Levels = FMath::Min(A.Sim.Shown.Num(), B.Sim.Shown.Num());
		if (!A.bLive || !B.bLive || NumLimbs == 0 || Pair.MidClearances.Num() != Levels * NumLimbs)
		{
			continue;
		}
		const FVector Outward = (A.Sim.Outward + B.Sim.Outward).GetSafeNormal();
		for (int32 Level = 0; Level < Levels; ++Level)
		{
			const FVector AboveA = Level > 0 ? A.Sim.Shown[Level - 1] : A.Anchor;
			const FVector AboveB = Level > 0 ? B.Sim.Shown[Level - 1] : B.Anchor;
			FVector DeltaA = FVector::ZeroVector;
			FVector DeltaB = FVector::ZeroVector;
			for (int32 Limb = 0; Limb < NumLimbs; ++Limb)
			{
				FBodySwingCapsule Capsule = LimbsNow[Limb];
				const FVector PointA = A.Sim.Shown[Level] + DeltaA;
				const FVector PointB = B.Sim.Shown[Level] + DeltaB;
				// The nearer of the two points takes more of the push (both taking all of it folded a corner another limb
				// had already pushed out right over), level (the cloth isn't lifted: a limb under it lifts its chain).
				const double NearA = FVector::Dist(PointA, FMath::ClosestPointOnSegment(PointA, Capsule.A, Capsule.B));
				const double NearB = FVector::Dist(PointB, FMath::ClosestPointOnSegment(PointB, Capsule.A, Capsule.B));
				const double ShareA = NearA + NearB > UE_KINDA_SMALL_NUMBER ? NearB / (NearA + NearB) : 0.5;
				auto Spread = [&](FVector Push, double Corners)
				{
					Push.Z = 0.0;
					DeltaA += Push * (Corners * ShareA);
					DeltaB += Push * (Corners * (1.0 - ShareA));
				};
				// Halfway between them moves by half what both move together; the patch's middle by a quarter.
				Capsule.Radius = Pair.MidClearances[Level * NumLimbs + Limb] * static_cast<float>(Scale);
				Spread(FBodySwingPoints::PushPlaceOut((PointA + PointB) * 0.5, Capsule, Capsule.bShortestWay ? FVector::ZeroVector : Outward), 2.0);
				Capsule.Radius = Pair.CenterClearances[Level * NumLimbs + Limb] * static_cast<float>(Scale);
				const FVector Center = (AboveA + AboveB + A.Sim.Shown[Level] + DeltaA + B.Sim.Shown[Level] + DeltaB) * 0.25;
				Spread(FBodySwingPoints::PushPlaceOut(Center, Capsule, Capsule.bShortestWay ? FVector::ZeroVector : Outward), 4.0);
			}
			DeltaA = DeltaA.GetClampedToMaxSize(MaxRingPush * Scale);
			DeltaB = DeltaB.GetClampedToMaxSize(MaxRingPush * Scale);
			if (!DeltaA.IsNearlyZero(0.01))
			{
				MoveLevel(A.Sim.Shown, A.Anchor, Level, DeltaA);
			}
			if (!DeltaB.IsNearlyZero(0.01))
			{
				MoveLevel(B.Sim.Shown, B.Anchor, Level, DeltaB);
			}
		}
	}
}
