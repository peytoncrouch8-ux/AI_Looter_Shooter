// ABargeBoarding: where things are on her, in the world: the berths the Lamplighter lies at, the grapples' bites, the
// plank's landing, where a player is put back aboard, the fights' spots, the office's desk, the horns, the hatch (the
// house's door), her deck's and hall's middles, the hose's top. The build scripts call the UFUNCTIONs in the editor; the
// sortie reads the rest live. All from her placed transform and BargeRules (nothing copied into the level but what a
// script places from these).

#include "World/BargeBoarding.h"
#include "World/BargeRules.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

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
	const FVector Keel(BargeRules::BerthX, BargeRules::BerthSide * BargeRules::BerthOut,
		BargeRules::DeckTop - BargeRules::BerthUnderDeck - BargeRules::ShipDeckOverKeel);
	return FTransform(FRotator(0.f, GetActorRotation().Yaw, 0.f), ToWorld(Keel));
}

FTransform ABargeBoarding::GetJumpBerth() const
{
	// Under the gangway's broken end, as she lists and drifts: the ship keeps level (the barge's yaw only).
	const FVector Keel(BargeRules::BerthX, BargeRules::BerthSide * BargeRules::JumpOut,
		BargeRules::DeckTop - BargeRules::JumpUnderDeck - BargeRules::ShipDeckOverKeel);
	return FTransform(FRotator(0.f, GetActorRotation().Yaw, 0.f), ToWorld(Keel));
}

FVector ABargeBoarding::GetGrappleAnchor(int32 Index) const
{
	// On the bulwark's top, fore and aft of its gap.
	const float Along = Index == 0 ? BargeRules::GrappleSpread : -BargeRules::GrappleSpread;
	return ToWorld(FVector(BargeRules::BerthX + Along, BargeRules::BerthSide * (BargeRules::BulwarkHalfBeam - BargeRules::BulwarkThick * 0.5),
		BargeRules::DeckTop + BargeRules::RailHeight));
}

FVector ABargeBoarding::GetBoardingPoint() const
{
	return ToWorld(FVector(BargeRules::BerthX, BargeRules::BerthSide * (BargeRules::DeckHalfBeam - BargeRules::BoardingInset), BargeRules::DeckTop));
}

FTransform ABargeBoarding::GetPlayerStart() const
{
	// On the deck, standing (a capsule's half height over it): by the plank facing inboard until the hatch; after it, on the
	// deck out from the house's door, facing it (the way back down).
	const bool bBelow = StoryStep >= BargeRules::ManifestStep;
	const FVector Local = bBelow ? BargeRules::HatchSpot() + FVector(BargeRules::HatchStartOut, 0.0, BargeRules::StandUp)
		: FVector(BargeRules::BerthX, BargeRules::BerthSide * (BargeRules::DeckHalfBeam - BargeRules::PlankStartIn), BargeRules::DeckTop + BargeRules::StandUp);
	const float Yaw = static_cast<float>(GetActorRotation().Yaw) + (bBelow ? 180.f : (BargeRules::BerthSide < 0.f ? 90.f : -90.f));
	return FTransform(FRotator(0.f, Yaw, 0.f), ToWorld(Local));
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
	else if (Fight == HallFight)
	{
		// Four at each stokehold's door, in two rows: the pushes come through them.
		for (const float Angle : BargeRules::StokeholdAngles)
		{
			for (int32 Index = 0; Index < BargeRules::HallFightSpots / 2; ++Index)
			{
				const float Across = (Index % 2 == 0 ? -1.f : 1.f) * BargeRules::HallSpotAcross;
				const float In = BargeRules::HallRadius - BargeRules::HallSpotIn - (Index / 2) * BargeRules::HallSpotRow;
				const float Turn = FMath::DegreesToRadians(Angle);
				const FVector Out(FMath::Cos(Turn), FMath::Sin(Turn), 0.0);
				const FVector Side(-Out.Y, Out.X, 0.0);
				Spots.Add(ToWorld(BargeRules::HallCenter + Out * In + Side * Across + FVector(0.0, 0.0, BargeRules::HallFloor + BargeRules::SpotUp)));
			}
		}
	}
	return Spots;
}

FVector ABargeBoarding::GetOfficeSpot() const
{
	return ToWorld(BargeRules::DeskTop());
}

TArray<FTransform> ABargeBoarding::GetHornSpots() const
{
	TArray<FTransform> Spots;
	for (const FVector& Spot : BargeRules::HornSpots)
	{
		// Each post's horns turned in over the deck (toward her middle line).
		const float Yaw = static_cast<float>(GetActorRotation().Yaw) + (Spot.Y < 0.0 ? 90.f : -90.f);
		Spots.Add(FTransform(FRotator(0.f, Yaw, 0.f), ToWorld(Spot)));
	}
	return Spots;
}

FVector ABargeBoarding::GetHatchSpot() const
{
	return ToWorld(BargeRules::HatchSpot());
}

FVector ABargeBoarding::GetDeckMiddle() const
{
	return ToWorld(FVector((BargeRules::DeckAft + BargeRules::DeckFore) * 0.5, 0.0, BargeRules::DeckTop + BargeRules::SpotUp));
}

FVector ABargeBoarding::GetHallMiddle() const
{
	return ToWorld(BargeRules::HallPoint(0.f, 0.f, BargeRules::SpotUp));
}

FVector ABargeBoarding::GetHoseTop() const
{
	return GetActorTransform().TransformPosition(BargeRules::DrumUnderKeel);
}
