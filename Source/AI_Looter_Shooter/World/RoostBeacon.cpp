// URoostBeaconComponent: a Hob's roost's beacon, the column of light seen from across the map, and its show as it's found.

#include "World/RoostBeacon.h"
#include "World/RoostKit.h"
#include "World/RoostRules.h"
#include "World/RoostSettings.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"
#include "Math/RotationMatrix.h"

namespace
{
	/** The haze stands this share of the core's height: a soft sleeve low down, the core alone reaching the sky. */
	constexpr float HazeShare = 0.72f;

	/** How bright the pulse is at its brightest, and how big against the flame. */
	constexpr float PulseGlow = 9.f;
	constexpr float PulseScale = 2.2f;

	/** The halo's own glow against the flame's. */
	constexpr float HaloGlow = 0.6f;

	/** A flare's swell of the flame (a share of its size) and its brightening, at its peak (a find; a mark again is the rest's). */
	constexpr float FlareSwell = 0.9f;
	constexpr float FlareBright = 2.5f;

	/** A change in the near fade smaller than this isn't worth a render update. */
	constexpr float FadeStep = 0.01f;

	/** An upright card of the glow's streak, Length tall and Width wide, turned Degrees about the vertical: its bright head (the
	 *  plane's +X end) at the bottom, on the post, fading upward. The engine's plane is 100 x 100 in XY, facing +Z. */
	FTransform UprightStreak(float Degrees, float Length, float Width)
	{
		const float Radians = FMath::DegreesToRadians(Degrees);
		const FVector Facing(FMath::Cos(Radians), FMath::Sin(Radians), 0.f);
		const FQuat Turn = FRotationMatrix::MakeFromXZ(FVector(0.f, 0.f, -1.f), Facing).ToQuat();
		const float Tall = FMath::Max(Length, 1.f);
		return FTransform(Turn, FVector(0.f, 0.f, Tall * 0.5f), FVector(Tall / 100.f, FMath::Max(Width, 1.f) / 100.f, 1.f));
	}

	/** A round card about Where: Which 0 lies flat, 1 stands along X, 2 along Y (so the glow reads from every side). */
	FTransform RoundCard(int32 Which, const FVector& Where, float Size)
	{
		static const FRotator Turns[] = { FRotator(0.0, 0.0, 0.0), FRotator(0.0, 0.0, 90.0), FRotator(90.0, 0.0, 0.0) };
		return FTransform(Turns[FMath::Clamp(Which, 0, 2)], Where, FVector(FMath::Max(Size, 1.f) / 100.f));
	}
}

URoostBeaconComponent::URoostBeaconComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Ticks only for its show or a camera near enough to fade it (WatchView).
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	static UStaticMesh* const Plane = RoostKit::FindIfMade<UStaticMesh>(RoostKit::PlanePath);
	static UMaterialInterface* const Glow = RoostKit::FindIfMade<UMaterialInterface>(RoostKit::GlowPath);
	SetStaticMesh(Plane);
	SetMaterial(0, Glow);
	SetMobility(EComponentMobility::Movable);
	SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	SetGenerateOverlapEvents(false);
	SetCanEverAffectNavigation(false);
	SetCastShadow(false);
	bReceivesDecals = false;
	bAffectDistanceFieldLighting = false;
	bVisibleInRayTracing = false;
	// Seen from across the map: never culled by distance, the component or its cards.
	bNeverDistanceCull = true;
	LDMaxDrawDistance = 0.f;
	bAllowCullDistanceVolume = false;
	InstanceStartCullDistance = 0;
	InstanceEndCullDistance = 0;
	NumCustomDataFloats = RoostKit::GlowFloats;
	// Dark until the roost is found.
	SetVisibility(false);
}

void URoostBeaconComponent::BuildCards()
{
	if (GetInstanceCount() == CardCount)
	{
		Apply();
		return;
	}
	ClearInstances();
	SetNumCustomDataFloats(RoostKit::GlowFloats);
	TArray<FTransform> Cards;
	Cards.Init(FTransform(FQuat::Identity, FVector::ZeroVector, FVector(0.01f)), CardCount);
	AddInstances(Cards, /*bShouldReturnIndices*/ false, /*bWorldSpace*/ false, /*bUpdateNavigation*/ false);
	// Every card's colour and shape stays; Apply sets where they are and how bright. The flame's hot core in the column's
	// core, the flame's heart and the pulse; the roost's own colour in the haze and the halo round them.
	const URoostSettings& Settings = URoostSettings::Get();
	const FLinearColor Colour = Settings.GetBeaconColor();
	const FLinearColor Core = Settings.GetFlameCoreColor();
	for (int32 Card = 0; Card < CardCount; ++Card)
	{
		const bool bStreak = Card < FlameFirst;
		const bool bHot = Card < HazeFirst || (Card >= FlameFirst && Card < HaloFirst) || Card >= PulseFirst;
		const FLinearColor& Tint = bHot ? Core : Colour;
		const float Data[] = { Tint.R, Tint.G, Tint.B, 0.f, bStreak ? RoostKit::StreakShape : RoostKit::RoundShape };
		SetCustomData(Card, Data);
	}
	LengthShown = -1.f;
	Apply();
}

void URoostBeaconComponent::SetLit(bool bInLit)
{
	bLit = bInLit;
	RiseAge = -1.f;
	FlareAge = -1.f;
	PulseAge = -1.f;
	SetVisibility(bLit && GetStaticMesh() != nullptr);
	Apply();
	SetComponentTickEnabled(bLit && bWatching);
}

void URoostBeaconComponent::Ignite()
{
	bLit = true;
	RiseAge = 0.f;
	FlareAge = 0.f;
	FlareKind = EGraveMark::First;
	PulseAge = 0.f;
	SetVisibility(GetStaticMesh() != nullptr);
	Apply();
	SetComponentTickEnabled(true);
}

void URoostBeaconComponent::Pulse()
{
	if (!bLit)
	{
		return;
	}
	PulseAge = 0.f;
	// A flame already up lifts again, but never cuts short a find's own flare.
	if (FlareAge < 0.f || FlareKind != EGraveMark::First)
	{
		FlareAge = 0.f;
		FlareKind = EGraveMark::Again;
	}
	Apply();
	SetComponentTickEnabled(true);
}

bool URoostBeaconComponent::IsShowing() const
{
	return RiseAge >= 0.f || FlareAge >= 0.f || PulseAge >= 0.f;
}

void URoostBeaconComponent::Advance(float DeltaSeconds)
{
	if (!IsShowing())
	{
		return;
	}
	auto Step = [DeltaSeconds](float& Age, float Length)
	{
		if (Age >= 0.f)
		{
			Age += DeltaSeconds;
			if (Age >= Length)
			{
				Age = -1.f;
			}
		}
	};
	Step(RiseAge, RoostRules::RiseSeconds);
	Step(FlareAge, GraveMarkRules::FlareSeconds(FlareKind));
	Step(PulseAge, RoostRules::PulseSeconds);
	Apply();
}

void URoostBeaconComponent::SetViewDistance(float Distance)
{
	const URoostSettings& Settings = URoostSettings::Get();
	const float Fade = RoostRules::NearFade(Distance, Settings.NearGone, Settings.NearFull);
	if (FMath::Abs(Fade - NearFade) < FadeStep && !(Fade == 1.f && NearFade != 1.f) && !(Fade == 0.f && NearFade != 0.f))
	{
		return;
	}
	NearFade = Fade;
	Apply();
}

void URoostBeaconComponent::WatchView(bool bWatch)
{
	if (bWatch == bWatching)
	{
		return;
	}
	bWatching = bWatch;
	if (!bWatching && NearFade != 1.f)
	{
		// Out of the camera's reach: the column whole again for whoever sees it from afar.
		NearFade = 1.f;
		Apply();
	}
	SetComponentTickEnabled(bLit && (bWatching || IsShowing()));
}

void URoostBeaconComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (bWatching)
	{
		const UWorld* World = GetWorld();
		const APlayerController* Viewer = World ? World->GetFirstPlayerController() : nullptr;
		if (Viewer && Viewer->PlayerCameraManager)
		{
			SetViewDistance(static_cast<float>(FVector::Dist(Viewer->PlayerCameraManager->GetCameraLocation(), GetComponentLocation())));
		}
	}
	Advance(DeltaTime);
	if (!IsShowing() && !bWatching)
	{
		SetComponentTickEnabled(false);
	}
}

float URoostBeaconComponent::GetBeamLength() const
{
	return bLit ? LengthShown : 0.f;
}

void URoostBeaconComponent::Apply()
{
	if (GetInstanceCount() != CardCount)
	{
		return;
	}
	const URoostSettings& Settings = URoostSettings::Get();
	const float Rise = RoostRules::BeamRise(RiseAge);
	const float Flare = GraveMarkRules::FlarePulse(FlareAge, FlareKind);
	float PulseHeight = 0.f;
	float PulseStrength = 0.f;
	const bool bPulse = RoostRules::PulseAt(PulseAge, PulseHeight, PulseStrength);
	const float Length = Settings.BeamHeight * Rise;
	const float FlameSize = Settings.FlameSize * (1.f + FlareSwell * Flare);

	// Where each card stands: moved only while the column grows or the flame swells (most of the time nothing moves).
	if (!FMath::IsNearlyEqual(Length, LengthShown, 1.f) || FlareAge >= 0.f || bPulse || PulseShown >= 0.f)
	{
		TArray<FTransform> Cards;
		Cards.Reserve(CardCount);
		for (int32 Card = 0; Card < 3; ++Card)
		{
			Cards.Add(UprightStreak(60.f * Card, Length, Settings.BeamWidth));
		}
		for (int32 Card = 0; Card < 3; ++Card)
		{
			Cards.Add(UprightStreak(30.f + 60.f * Card, Length * HazeShare, Settings.HazeWidth));
		}
		for (int32 Card = 0; Card < 3; ++Card)
		{
			Cards.Add(RoundCard(Card, FVector::ZeroVector, FlameSize));
		}
		const float HaloSize = Settings.FlameSize * Settings.HaloScale * (1.f + 0.5f * Flare);
		Cards.Add(RoundCard(1, FVector::ZeroVector, HaloSize));
		Cards.Add(RoundCard(2, FVector::ZeroVector, HaloSize));
		// The pulse climbs the column as it stands (a pulse on a rising column rides its top).
		const FVector PulseAt(0.f, 0.f, PulseHeight * Settings.BeamHeight * Rise);
		const float PulseSize = bPulse ? Settings.FlameSize * PulseScale : 1.f;
		Cards.Add(RoundCard(1, PulseAt, PulseSize));
		Cards.Add(RoundCard(2, PulseAt, PulseSize));
		BatchUpdateInstancesTransforms(0, Cards, /*bWorldSpace*/ false, /*bMarkRenderStateDirty*/ false, /*bTeleport*/ true);
		LengthShown = Length;
	}

	// How bright: the column by the camera's nearness and its rise; the flame stays (a little dimmer near), flaring on a find.
	const float Near = bLit ? NearFade : 0.f;
	BeamShown = Settings.BeamStrength * Near * (0.35f + 0.65f * Rise) * (1.f + 1.2f * Flare);
	const float Haze = Settings.HazeStrength * Near * Rise * (1.f + 0.8f * Flare);
	FlameShown = bLit ? Settings.FlameStrength * FMath::Lerp(0.6f, 1.f, NearFade) * (1.f + FlareBright * Flare) : 0.f;
	const float Halo = bLit ? Settings.FlameStrength * HaloGlow * FMath::Lerp(0.4f, 1.f, NearFade) * (1.f + 1.5f * Flare) : 0.f;
	const float PulseGlowNow = bLit && bPulse ? PulseGlow * PulseStrength * FMath::Lerp(0.3f, 1.f, NearFade) : 0.f;
	PulseShown = bPulse ? PulseHeight : -1.f;
	for (int32 Card = 0; Card < CardCount; ++Card)
	{
		const float Strength = Card < HazeFirst ? BeamShown : Card < FlameFirst ? Haze : Card < HaloFirst ? FlameShown
			: Card < PulseFirst ? Halo : PulseGlowNow;
		SetCustomDataValue(Card, RoostKit::StrengthFloat, Strength, /*bMarkRenderStateDirty*/ false);
	}
	MarkRenderStateDirty();
}
