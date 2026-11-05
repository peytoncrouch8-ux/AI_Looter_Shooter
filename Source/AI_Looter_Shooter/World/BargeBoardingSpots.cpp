// ABargeBoarding: where things are on her, in the world: the berths the Lamplighter lies at, the grapples' bites, the
// plank's landing, where a player is put back aboard, the fights' spots, the office's desk, the horns, the hatch (its
// stair's top on the well deck), her deck's and hall's middles, the hose's top. The build scripts call the UFUNCTIONs in the editor; the
// sortie reads the rest live. All from her placed transform and BargeRules (nothing copied into the level but what a
// script places from these).

#include "World/BargeBoarding.h"
#include "World/BargeRules.h"
#include "CollisionQueryParams.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/HitResult.h"
#include "Engine/StaticMesh.h"

namespace
{
	/** The droppers' ropes of a fight (the roof's or the tower's), or none. */
	TConstArrayView<BargeRules::FDropRope> DropsOf(FName Fight)
	{
		if (Fight == ABargeBoarding::RoofFight)
		{
			return BargeRules::RoofDrops;
		}
		if (Fight == ABargeBoarding::TowerFight)
		{
			return BargeRules::TowerDrops;
		}
		return {};
	}
}

FVector ABargeBoarding::ToWorld(const FVector& Local) const
{
	return GetActorTransform().TransformPosition(Local + FVector(0.0, 0.0, KitLift));
}

FTransform ABargeBoarding::RestTransform() const
{
	return bPlacedKnown ? Placed : GetActorTransform();
}

bool ABargeBoarding::HasHull() const
{
	const UStaticMesh* Mesh = Hull ? Hull->GetStaticMesh() : nullptr;
	return Mesh && Mesh->GetPathName() == HullPath;
}

FTransform ABargeBoarding::GetBerth() const
{
	// Her keel aft of the gap by her plank's hinge (ShipPlankHingeX), so the plank comes square across it.
	const FVector Keel(BargeRules::BerthX - BargeRules::ShipPlankHingeX, BargeRules::BerthSide * BargeRules::BerthOut,
		BargeRules::WellDeckZ - BargeRules::BerthUnderDeck - BargeRules::ShipDeckOverKeel);
	return FTransform(FRotator(0.f, GetActorRotation().Yaw, 0.f), ToWorld(Keel));
}

FTransform ABargeBoarding::GetJumpBerth() const
{
	// Under the gangway's broken end, as she lists and drifts: the ship keeps level (the barge's yaw only), lined up as at the
	// berth (her open deck fore of the console and the locker under the jump).
	const FVector Keel(BargeRules::BerthX - BargeRules::ShipPlankHingeX, BargeRules::BerthSide * BargeRules::JumpOut,
		BargeRules::WellDeckZ - BargeRules::JumpUnderDeck - BargeRules::ShipDeckOverKeel);
	return FTransform(FRotator(0.f, GetActorRotation().Yaw, 0.f), ToWorld(Keel));
}

FVector ABargeBoarding::GetGrappleAnchor(int32 Index) const
{
	// On her rail's top fore and aft of the Lamplighter's middle (her keel lies ShipPlankHingeX aft of the gap: GetBerth), so
	// the chains run square to Junie's launchers.
	const float Along = Index == 0 ? BargeRules::GrappleSpread : -BargeRules::GrappleSpread;
	return ToWorld(FVector(BargeRules::BerthX - BargeRules::ShipPlankHingeX + Along, BargeRules::BerthSide * (BargeRules::BulwarkHalfBeam - BargeRules::BulwarkThick * 0.5),
		BargeRules::WellDeckZ + BargeRules::RailHeight));
}

FVector ABargeBoarding::GetBoardingPoint() const
{
	return ToWorld(FVector(BargeRules::BerthX, BargeRules::BerthSide * (BargeRules::WellDeckHalfBeam - BargeRules::BoardingInset), BargeRules::WellDeckZ));
}

FTransform ABargeBoarding::GetPlayerStart() const
{
	// On the deck, standing (a capsule's half height over it): by the plank facing inboard until the hatch; after it, on the
	// deck behind the hatch's stair, facing down it (the way back down).
	const bool bBelow = StoryStep >= BargeRules::ManifestStep;
	const FVector2D Way = BargeRules::HatchWay.GetSafeNormal();
	const FVector Local = bBelow ? BargeRules::HatchSpot() - FVector(Way, 0.0) * BargeRules::HatchStartOut + FVector(0.0, 0.0, BargeRules::StandUp)
		: FVector(BargeRules::BerthX, BargeRules::BerthSide * (BargeRules::WellDeckHalfBeam - BargeRules::PlankStartIn), BargeRules::WellDeckZ + BargeRules::StandUp);
	const float Facing = bBelow ? static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(Way.Y, Way.X))) : (BargeRules::BerthSide < 0.f ? 90.f : -90.f);
	return FTransform(FRotator(0.f, static_cast<float>(GetActorRotation().Yaw) + Facing, 0.f), ToWorld(Local));
}

TArray<FVector> ABargeBoarding::GetFightSpots(FName Fight) const
{
	TArray<FVector> Spots;
	if (Fight == DeckFight)
	{
		for (const FVector& Spot : BargeRules::DeckFightSpots)
		{
			Spots.Add(ToWorld(Spot + FVector(0.0, 0.0, BargeRules::SpotUp)));
		}
	}
	else if (Fight == RoofFight || Fight == TowerFight)
	{
		// The droppers' landings, in their ropes' order (a spot's turn is its rope's).
		for (const BargeRules::FDropRope& Rope : DropsOf(Fight))
		{
			Spots.Add(ToWorld(Rope.Feet + FVector(0.0, 0.0, BargeRules::SpotUp)));
		}
	}
	else if (Fight == SternFight)
	{
		for (const FVector& Spot : BargeRules::SternFightSpots)
		{
			Spots.Add(ToWorld(Spot + FVector(0.0, 0.0, BargeRules::SpotUp)));
		}
	}
	else if (Fight == HallFight)
	{
		// Four at each stokehold's door, in two rows: the pushes come through them.
		for (const float Angle : BargeRules::StokeholdAngles)
		{
			for (int32 Index = 0; Index < BargeRules::HallFightSpots / 2; ++Index)
			{
				const float Across = (Index % 2 == 0 ? -1.f : 1.f) * BargeRules::HallSpotAcross;
				const float In = BargeRules::HallRadius - BargeRules::HallSpotIn - (Index / 2) * BargeRules::HallSpotRow;
				const FVector Out = BargeRules::HallPoint(Angle, 1.f) - BargeRules::HallPoint(Angle, 0.f);
				const FVector Side(-Out.Y, Out.X, 0.0);
				Spots.Add(ToWorld(BargeRules::HallPoint(Angle, In, BargeRules::SpotUp) + Side * Across));
			}
		}
	}
	return Spots;
}

TArray<FVector> ABargeBoarding::GetDropRopeTops(FName Fight) const
{
	// Each top a hand off the face its man comes over, found on her hull (Tripo's faces aren't flat: the house's upper storey
	// is rounded at its top and its sides): off its foremost point over the height he hangs at, so his body clears it too.
	TArray<FVector> Tops;
	const FCollisionQueryParams Params(SCENE_QUERY_STAT(BargeDropTop), false);
	for (const BargeRules::FDropRope& Rope : DropsOf(Fight))
	{
		const double Way = BargeRules::DropFaceWay(Rope);
		// The face's X times Way: least is foremost (toward the open deck he drops onto).
		double Foremost = TNumericLimits<double>::Max();
		for (int32 Try = 0; Hull && Try < BargeRules::DropFaceTries; ++Try)
		{
			const FVector From = Rope.Top + FVector(-Way * BargeRules::DropTopOff, 0.0, -Try * BargeRules::DropFaceStepDown);
			const FVector To = From + FVector(Way * (BargeRules::DropTopOff + BargeRules::DropFaceSearch), 0.0, 0.0);
			FHitResult Hit;
			if (Hull->LineTraceComponent(Hit, ToWorld(From), ToWorld(To), Params) && !Hit.bStartPenetrating)
			{
				Foremost = FMath::Min(Foremost, Way * GetActorTransform().InverseTransformPosition(Hit.ImpactPoint).X);
			}
		}
		FVector Top = Rope.Top;
		if (Foremost < TNumericLimits<double>::Max())
		{
			Top.X = Way * Foremost - Way * BargeRules::DropTopOut;
		}
		Tops.Add(ToWorld(Top));
	}
	return Tops;
}

FVector ABargeBoarding::GetOfficeSpot() const
{
	// The desk as laid: on the bench's work top, the clear middle its Gun socket marks (seen from the walkway: the bench
	// faces it); a bench without it, its work top's height over its middle; without the bench the rules' block's top.
	if (Desk && Desk->GetStaticMesh())
	{
		if (Desk->DoesSocketExist(BargeRules::DeskTopSocket()))
		{
			return Desk->GetSocketLocation(BargeRules::DeskTopSocket()) - FVector(0.0, 0.0, BargeRules::DeskSocketOverTop);
		}
		const FBox Box = Desk->CalcBounds(Desk->GetComponentTransform()).GetBox();
		return FVector(Box.GetCenter().X, Box.GetCenter().Y, Box.Min.Z + BargeRules::DeskWorkTop);
	}
	return ToWorld(BargeRules::DeskTop());
}

TArray<FTransform> ABargeBoarding::GetHornSpots() const
{
	TArray<FTransform> Spots;
	for (const BargeRules::FHornSpot& Spot : BargeRules::HornSpots)
	{
		// Each post's horns (its +X) the spot's way: fore over the deck from the house, in over it from the rim.
		Spots.Add(FTransform(FRotator(0.f, static_cast<float>(GetActorRotation().Yaw) + Spot.Yaw, 0.f), ToWorld(Spot.At)));
	}
	return Spots;
}

FVector ABargeBoarding::GetHatchSpot() const
{
	return ToWorld(BargeRules::HatchSpot());
}

FVector ABargeBoarding::GetDeckMiddle() const
{
	return ToWorld(BargeRules::DeckMiddle() + FVector(0.0, 0.0, BargeRules::SpotUp));
}

FVector ABargeBoarding::GetHallMiddle() const
{
	return ToWorld(BargeRules::HallPoint(0.f, 0.f, BargeRules::SpotUp));
}

FVector ABargeBoarding::GetHoseTop() const
{
	return GetActorTransform().TransformPosition(BargeRules::DrumUnderKeel);
}
