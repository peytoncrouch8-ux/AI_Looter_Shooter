// An Unpaid's hat knocked off (HITS, 2026-10-28): the plain flight (FUnpaidHatFlight) and the hat component it moves. The
// hat is the Unpaid's own mesh component, detached and moved by hand: no physics, no new mesh, nothing at all while it sits
// on a head. At most UnpaidHat::MaxOff are off in a world at once; past that, one just goes in a puff of dust.

#include "Creatures/UnpaidHat.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureReactComponent.h"
#include "World/WorldQueries.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/HitResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Math/RandomStream.h"

namespace
{
	/** Off along the shot this fast (cm/s, level, at random between), this fast up, tumbling this fast (degrees a second). */
	constexpr float LaunchSpeed[2] = { 350.f, 500.f };
	constexpr float LaunchUp = 250.f;
	constexpr float Spin = 540.f;
	constexpr float Gravity = 980.f;
	/** Its one bounce keeps this much of its speed; a surface this level or more is ground (its normal's Z), steeper a wall. */
	constexpr float BounceKeep = 0.3f;
	constexpr float GroundNormalZ = 0.6f;
	/** A step longer than this (s, a hitch) is cut short: the trace covers the move anyway, but the arc would come out wrong. */
	constexpr float MaxStep = 0.1f;
	/** Down, it lies tipped this far (degrees) toward the way it was flying: a hat dropped, not set down. */
	constexpr float LieTilt = 6.f;

	/** The hats off heads now, every world's (the cap counts a world's own; a destroyed Unpaid's drops out by itself). */
	TArray<TWeakObjectPtr<UStaticMeshComponent>>& OffHats()
	{
		static TArray<TWeakObjectPtr<UStaticMeshComponent>> Hats;
		return Hats;
	}

	void Forget(const UStaticMeshComponent& Hat)
	{
		OffHats().RemoveAll([&Hat](const TWeakObjectPtr<UStaticMeshComponent>& Each) { return !Each.IsValid() || Each.Get() == &Hat; });
	}
}

// ---------------------------------------------------------------------------
// The flight
// ---------------------------------------------------------------------------

void FUnpaidHatFlight::Launch(const FVector& Start, const FQuat& StartTurn, const FVector& Way, float InBrim, FRandomStream& Dice)
{
	*this = FUnpaidHatFlight();
	Stage = EStage::Flying;
	Location = Start;
	Turn = StartTurn;
	Brim = InBrim;
	StartZ = static_cast<float>(Start.Z);
	// Level along the shot (a shot from below or above still throws it off sideways, not into the ground or the sky).
	FVector Level(Way.X, Way.Y, 0.0);
	if (!Level.Normalize())
	{
		Level = -StartTurn.GetForwardVector();
		Level.Z = 0.0;
		if (!Level.Normalize())
		{
			Level = FVector::BackwardVector;
		}
	}
	Velocity = Level * Dice.FRandRange(LaunchSpeed[0], LaunchSpeed[1]) + FVector(0.0, 0.0, LaunchUp);
	// Tumbling brim over crown away from the shot, a little askew so no two go the same.
	SpinAxis = (FVector::CrossProduct(FVector::UpVector, Level) + FVector(0.0, 0.0, Dice.FRandRange(-0.4f, 0.4f))).GetSafeNormal();
	SpinSpeed = Spin * Dice.FRandRange(0.85f, 1.15f);
}

bool FUnpaidHatFlight::Step(float Dt, FTrace Trace)
{
	if (!IsShown())
	{
		return false;
	}
	const float Seconds = FMath::Clamp(Dt, 0.f, MaxStep);
	Time += Seconds;
	switch (Stage)
	{
	case EStage::Flying:
	{
		Velocity.Z -= Gravity * Seconds;
		const FVector Move = Velocity * Seconds;
		// The trace follows its lowest point (the brim, about Brim under the pivot whichever way it tumbles).
		const FVector Low(0.0, 0.0, -Brim);
		FVector Point;
		FVector Normal;
		if (!Move.IsNearlyZero() && Trace(Location + Low, Location + Low + Move, Point, Normal))
		{
			if (Normal.Z >= GroundNormalZ && Bounces == 0)
			{
				// Its one bounce, most of its speed lost.
				Location = Point - Low + Normal * 0.5;
				Velocity = (Velocity - 2.0 * FVector::DotProduct(Velocity, Normal) * Normal) * BounceKeep;
				SpinSpeed *= 0.5f;
				++Bounces;
			}
			else if (Normal.Z >= GroundNormalZ)
			{
				// Down: it lies crown up, tipped a little toward where it was going.
				const float Yaw = static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(Velocity.Y, Velocity.X)));
				Stage = EStage::Lying;
				Time = 0.f;
				Location = Point - FVector(0.0, 0.0, -Brim);
				Turn = FRotator(-LieTilt, Yaw, 0.f).Quaternion();
				Velocity = FVector::ZeroVector;
				return true;
			}
			else
			{
				// A wall: it stops against it and drops down it.
				Location = Point - Low + Normal * 2.0;
				Velocity = FVector(0.0, 0.0, FMath::Min(Velocity.Z, 0.0)) + Normal * 30.0;
				SpinSpeed *= 0.3f;
			}
		}
		else
		{
			Location += Move;
		}
		Turn = (FQuat(SpinAxis, FMath::DegreesToRadians(SpinSpeed * Seconds)) * Turn).GetNormalized();
		if (Time > MaxFlightSeconds || Location.Z < StartZ - MaxFall)
		{
			// Over a cliff, or never came down: gone.
			Stage = EStage::Gone;
			return false;
		}
		return true;
	}
	case EStage::Lying:
		if (Time >= LieSeconds)
		{
			Stage = EStage::Sinking;
			Time = 0.f;
		}
		return true;
	case EStage::Sinking:
		Location.Z -= SinkDepth * Seconds / SinkSeconds;
		if (Time >= SinkSeconds)
		{
			Stage = EStage::Gone;
			return false;
		}
		return true;
	default:
		return false;
	}
}

// ---------------------------------------------------------------------------
// The hat component
// ---------------------------------------------------------------------------

bool UnpaidHat::KnockOff(UStaticMeshComponent& Hat, FUnpaidHatFlight& Flight, const FVector& Way, FRandomStream& Dice)
{
	const UStaticMesh* Model = Hat.GetStaticMesh();
	if (Flight.IsOff() || !Model || !Hat.IsVisible() || !Hat.GetAttachParent())
	{
		return false;
	}
	const FTransform Worn = Hat.GetComponentTransform();
	Hat.DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	UWorld* World = Hat.GetWorld();
	if (NumOff(World) >= MaxOff)
	{
		// Enough hats about already: this one goes in a puff of dust off the head.
		Flight.Stage = FUnpaidHatFlight::EStage::Gone;
		Hat.SetVisibility(false);
		const ACreatureBase* Owner = Cast<ACreatureBase>(Hat.GetOwner());
		if (UCreatureReactComponent* Reacts = Owner ? Owner->GetReact() : nullptr)
		{
			Reacts->KickFootfall(Worn.GetLocation(), Way * 120.0, 0.6f);
		}
		return true;
	}
	// Its lowest point under its pivot, as it sits upright (the hat is modelled in the body's axes, Z up).
	const float Brim = static_cast<float>(-Model->GetBoundingBox().Min.Z * Worn.GetScale3D().Z);
	Flight.Launch(Worn.GetLocation(), Worn.GetRotation(), Way, Brim, Dice);
	OffHats().AddUnique(&Hat);
	return true;
}

void UnpaidHat::Tick(UStaticMeshComponent& Hat, FUnpaidHatFlight& Flight, float Dt)
{
	if (!Flight.IsShown())
	{
		return;
	}
	UWorld* World = Hat.GetWorld();
	bool bShown = false;
	if (World)
	{
		const AActor* Owner = Hat.GetOwner();
		bool bParamsMade = false;
		FCollisionQueryParams Params;
		bShown = Flight.Step(Dt, [&](const FVector& From, const FVector& To, FVector& OutPoint, FVector& OutNormal)
		{
			// The level's own ground and walls (no volumes, not the playable area's walls), only while it flies.
			if (!bParamsMade)
			{
				Params = LooterWorld::StaticGeometryParams(World, TEXT("UnpaidHat"), Owner);
				bParamsMade = true;
			}
			FHitResult Hit;
			if (!World->LineTraceSingleByObjectType(Hit, From, To, FCollisionObjectQueryParams(ECC_WorldStatic), Params))
			{
				return false;
			}
			OutPoint = Hit.ImpactPoint;
			OutNormal = Hit.ImpactNormal;
			return true;
		});
	}
	if (bShown)
	{
		Hat.SetWorldLocationAndRotation(Flight.Location, Flight.Turn);
	}
	else
	{
		Flight.Stage = FUnpaidHatFlight::EStage::Gone;
		Hat.SetVisibility(false);
		Forget(Hat);
	}
}

void UnpaidHat::PutBack(UStaticMeshComponent& Hat, FUnpaidHatFlight& Flight)
{
	Flight.Reset();
	Forget(Hat);
}

int32 UnpaidHat::NumOff(const UWorld* World)
{
	TArray<TWeakObjectPtr<UStaticMeshComponent>>& Hats = OffHats();
	Hats.RemoveAll([](const TWeakObjectPtr<UStaticMeshComponent>& Each) { return !Each.IsValid(); });
	int32 Count = 0;
	for (const TWeakObjectPtr<UStaticMeshComponent>& Each : Hats)
	{
		Count += Each->GetWorld() == World ? 1 : 0;
	}
	return Count;
}
