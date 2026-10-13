#include "World/RoostCrows.h"
#include "World/FaunaCues.h"
#include "World/RoostKit.h"
#include "World/RoostSettings.h"
#include "Engine/StaticMesh.h"

ARoostCrows::ARoostCrows()
{
	// The ambient crows' pieces (build_area_fauna.py's crow), found only once they're in this checkout: without them the
	// flock keeps its time and draws nothing.
	static UStaticMesh* const CrowPerched = RoostKit::FindIfMade<UStaticMesh>(RoostKit::CrowPerchedPath);
	static UStaticMesh* const CrowHead = RoostKit::FindIfMade<UStaticMesh>(RoostKit::CrowHeadPath);
	static UStaticMesh* const CrowFlying = RoostKit::FindIfMade<UStaticMesh>(RoostKit::CrowFlyingPath);
	static UStaticMesh* const CrowWingL = RoostKit::FindIfMade<UStaticMesh>(RoostKit::CrowWingLPath);
	static UStaticMesh* const CrowWingR = RoostKit::FindIfMade<UStaticMesh>(RoostKit::CrowWingRPath);
	static UStaticMesh* const CrowOuterL = RoostKit::FindIfMade<UStaticMesh>(RoostKit::CrowOuterLPath);
	static UStaticMesh* const CrowOuterR = RoostKit::FindIfMade<UStaticMesh>(RoostKit::CrowOuterRPath);
	PerchedMesh = CrowPerched;
	HeadMesh = CrowHead;
	FlyingMesh = CrowFlying;
	WingMeshL = CrowWingL;
	WingMeshR = CrowWingR;
	OuterWingMeshL = CrowOuterL;
	OuterWingMeshR = CrowOuterR;

	Mode = EFaunaFlockMode::Perching;
	BirdCount = 4;
	// Hob's crows know Ellis: nobody walking about the roost scares them, and they come back while the player still stands on
	// its spot (1.7 m out from the mast, under the perches' spread); only gunfire and the roost's own moments lift them.
	FleeRadius = 0.f;
	GunfireRadius = 3000.f;
	ImpactRadius = 900.f;
	// A tight wheel about the post, low, and quick to settle: a flourish, not a flight.
	FlightSpeed = 850.f;
	CircleRadius = 650.f;
	CircleHeight = 600.f;
	CalmSecondsMin = 2.4f;
	CalmSecondsMax = 4.2f;
	LeaveAfterSeconds = 45.f;
	HopSecondsMin = 9.f;
	HopSecondsMax = 20.f;
	bHopsOnGround = false;
	HeadSecondsMin = 0.5f;
	HeadSecondsMax = 1.8f;
	WingbeatsPerSecond = 4.5f;
	WingbeatDegrees = 38.f;
	GlideDegrees = 5.f;
	GlideShare = 0.3f;
	CallCue = FaunaCue::CrowCaw;
	TakeOffCue = FaunaCue::CrowTakeOff;
	CallRange = 2500.f;
	CallSecondsMin = 9.f;
	CallSecondsMax = 22.f;
	CullDistance = 7000.f;
	BirdScale = 1.f;
	ScaleJitter = 0.08f;
}

void ARoostCrows::SetRoostPerches(const TArray<FTransform>& InPerches)
{
	Perches.Reset();
	for (const FTransform& Each : InPerches)
	{
		FFaunaPerch& Perch = Perches.AddDefaulted_GetRef();
		Perch.Location = Each.GetLocation();
		Perch.Yaw = static_cast<float>(Each.Rotator().Yaw);
		// A bar's top: the bird faces across it either way (the upper bar's ends and the crossbar alike).
		Perch.Kind = EFaunaPerchKind::Rail;
	}
	BirdCount = FMath::Clamp(URoostSettings::Get().CrowCount, 0, Perches.Num());
}

void ARoostCrows::MakeCircling(const FVector& Beacon, int32 Count)
{
	// The fauna's aerial flock (the hawks' thermal circles) round the beacon: a few crows riding the fire's heat over the
	// basket, mostly gliding, now and then a caw.
	Mode = EFaunaFlockMode::Aerial;
	Perches.Reset();
	BirdCount = FMath::Max(Count, 0);
	AerialArea.Center = Beacon;
	AerialArea.Radius = 650.f;
	AerialArea.MinHeight = 300.f;
	AerialArea.MaxHeight = 1200.f;
	AerialFigure = 1;
	FlightSpeed = 700.f;
	WingbeatsPerSecond = 3.8f;
	GlideShare = 0.6f;
	CallSecondsMin = 12.f;
	CallSecondsMax = 30.f;
	CallRange = 3000.f;
	CullDistance = URoostSettings::Get().CirclingCullDistance;
}

void ARoostCrows::Lift(const FVector& From)
{
	Startle(From);
}

void ARoostCrows::Scatter(const FVector& From)
{
	Startle(From);
}
