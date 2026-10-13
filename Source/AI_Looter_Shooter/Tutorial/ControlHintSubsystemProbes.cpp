// UControlHintSubsystem: its looks into the world, for the senses (ControlHintSubsystemSenses.cpp): a ledge ahead, a
// target under the crosshair, a gunsmith's bench near with a pair to trade. Each runs only while its hint could still
// show, so a player who knows the game costs none of them.

#include "Tutorial/ControlHintSubsystem.h"
#include "Combat/HealthComponent.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Missions/MissionTargets.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "World/GunsmithBench.h"
#include "World/WorldQueries.h"
#include "Components/CapsuleComponent.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"

namespace
{
	/** The ledge probe's two heights over the feet (cm): blocked at the knee, open over the chest. */
	constexpr float KneeHeight = 40.f;
	constexpr float OverChestHeight = 170.f;

	/** A face steeper than this (its normal's Z) is a wall or a fence, not a slope; a top flatter than TopFlatness is one to stand on. */
	constexpr float WallSteepness = 0.5f;
	constexpr float TopFlatness = 0.7f;

	/** How far past the ledge's face the look for its top comes down (cm). */
	constexpr float TopInset = 20.f;

	/** What the ledge probe sees: anything solid placed in the level, static or moving. */
	FCollisionObjectQueryParams SolidObjects()
	{
		FCollisionObjectQueryParams Objects;
		Objects.AddObjectTypesToQuery(ECC_WorldStatic);
		Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
		return Objects;
	}
}

bool UControlHintSubsystem::ProbeLedge(const ACharacter& Character, const FVector& Heading) const
{
	const UWorld* World = GetWorld();
	const FVector Forward = Heading.GetSafeNormal2D();
	if (!World || Forward.IsNearlyZero())
	{
		return false;
	}
	const UCapsuleComponent* Capsule = Character.GetCapsuleComponent();
	const float Radius = Capsule ? Capsule->GetScaledCapsuleRadius() : 30.f;
	const float HalfHeight = Capsule ? Capsule->GetScaledCapsuleHalfHeight() : 80.f;
	const FVector Feet = Character.GetActorLocation() - FVector(0.0, 0.0, HalfHeight);
	const FVector Ahead = Forward * (Radius + LedgeReach);
	// The level's solid pieces: volumes, the playable area's invisible walls and the scatter's grass are no ledges.
	const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("ControlHintLedge"), &Character, /*bTraceComplex*/ false);
	const FCollisionObjectQueryParams Objects = SolidObjects();

	// Something steep at knee height ahead (a wall, a fence, a crate's side; not a slope)...
	FHitResult Low;
	const FVector LowStart = Feet + FVector(0.0, 0.0, KneeHeight);
	if (!World->LineTraceSingleByObjectType(Low, LowStart, LowStart + Ahead, Objects, Params) || Low.ImpactNormal.Z > WallSteepness)
	{
		return false;
	}
	// ...open over the chest (a building's wall would block it too)...
	FHitResult High;
	const FVector HighStart = Feet + FVector(0.0, 0.0, OverChestHeight);
	if (World->LineTraceSingleByObjectType(High, HighStart, HighStart + Ahead, Objects, Params))
	{
		return false;
	}
	// ...and a top to stand on just past its face, from knee to chest height.
	const FVector Over = Low.ImpactPoint + Forward * TopInset;
	FHitResult Top;
	if (!World->LineTraceSingleByObjectType(Top, FVector(Over.X, Over.Y, HighStart.Z), FVector(Over.X, Over.Y, LowStart.Z - 10.0), Objects, Params))
	{
		return false;
	}
	const float Height = static_cast<float>(Top.ImpactPoint.Z - Feet.Z);
	return Top.ImpactNormal.Z >= TopFlatness && Height >= LedgeMinHeight && Height <= LedgeMaxHeight;
}

float UControlHintSubsystem::ProbeTarget(const APlayerController& Controller, const APawn& Pawn, const AWeaponBase& Gun) const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return -1.f;
	}
	// Along the crosshair, on the gun's own channel: what a shot would hit (pawn capsules let it through to the bodies).
	FVector Eye = FVector::ZeroVector;
	FRotator Facing = FRotator::ZeroRotator;
	Controller.GetPlayerViewPoint(Eye, Facing);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ControlHintTarget), /*bTraceComplex*/ false, &Pawn);
	Params.AddIgnoredActor(&Gun);
	FHitResult Hit;
	if (!World->LineTraceSingleByChannel(Hit, Eye, Eye + Facing.Vector() * TargetProbeLength, Gun.TraceChannel, Params))
	{
		return -1.f;
	}
	const AActor* Target = Hit.GetActor();
	if (!Target || Target == &Pawn || !Target->FindComponentByClass<UHealthComponent>() || !MissionTargets::IsAlive(*Target))
	{
		return -1.f;
	}
	return static_cast<float>(FVector::Dist(Pawn.GetActorLocation(), Hit.ImpactPoint));
}

bool UControlHintSubsystem::IsAtBenchWithPair(const APawn& Pawn, const UWeaponManagerComponent& Weapons)
{
	// Two guns of one kind among the slots and the backpack: the bench trades parts between guns of a kind.
	TArray<const UObject*, TInlineAllocator<16>> Kinds;
	bool bPair = false;
	auto Count = [&Kinds, &bPair](const UObject* Kind)
	{
		if (Kind)
		{
			bPair |= Kinds.Contains(Kind);
			Kinds.Add(Kind);
		}
	};
	for (const AWeaponBase* Gun : Weapons.GetWeapons())
	{
		Count(Gun ? Gun->GetInstance().Definition.Get() : nullptr);
	}
	for (const FWeaponInstanceData& Item : Weapons.GetBackpack())
	{
		Count(Item.Definition.Get());
	}
	if (!bPair)
	{
		return false;
	}
	if (!bBenchesFound)
	{
		// Benches are placed with the level and never move: found once.
		bBenchesFound = true;
		for (TActorIterator<AGunsmithBench> It(GetWorld()); It; ++It)
		{
			Benches.Add(*It);
		}
	}
	const FVector Where = Pawn.GetActorLocation();
	for (const TWeakObjectPtr<AActor>& Bench : Benches)
	{
		if (Bench.IsValid() && FVector::Dist(Bench->GetActorLocation(), Where) <= BenchDistance)
		{
			return true;
		}
	}
	return false;
}
