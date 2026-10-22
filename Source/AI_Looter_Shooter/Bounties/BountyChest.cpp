// ABountyChest: a won bounty's strongbox: made on the ground, dropping in, landing, telling the board when it's looted.

#include "Bounties/BountyChest.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Bosses/BossCameraShake.h"
#include "Bounties/BountyRules.h"
#include "Bounties/BountyTuning.h"
#include "Combat/BulletSubsystem.h"
#include "Weapons/WeaponFX.h"
#include "World/WorldQueries.h"
#include "CollisionQueryParams.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"

#define LOCTEXT_NAMESPACE "LooterBounty"

namespace
{
	/** Where it looks for the ground under its spot: this far over and under it (cm). */
	constexpr double GroundAbove = 300.0;
	constexpr double GroundBelow = 900.0;

	/** Its fall: gravity's pull (cm/s per s) from a standing start, so a 2.6 m drop takes about 0.7 s. */
	constexpr float Gravity = 1100.f;

	/** The landing: a little kick of the view near it, its dust (dull earth, as the set pieces'). */
	constexpr float LandShake = 0.12f;
	constexpr float LandShakeSeconds = 0.35f;
	constexpr float LandShakeRadius = 1500.f;
	const FLinearColor LandDust(0.36f, 0.31f, 0.25f);
	constexpr int32 LandPuffs = 7;
}

ABountyChest::ABountyChest()
{
	Kind = EChestKind::Strongbox;
	Prompt = LOCTEXT("ChestPrompt", "Open the bounty's strongbox");
}

ABountyChest* ABountyChest::SpawnFor(UWorld* World, FName BoardId, const FBountyChestOwed& Owed, const FVector& Spot, float Yaw, bool bDrop)
{
	if (!World || Owed.Serial <= 0)
	{
		return nullptr;
	}
	// On the ground under the spot: the terrain and what stands on it, never a creature or the playable area's walls.
	FVector Ground = Spot;
	FHitResult Hit;
	const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("BountyChest"), nullptr, /*bTraceComplex*/ false);
	if (World->LineTraceSingleByObjectType(Hit, Spot + FVector(0.0, 0.0, GroundAbove), Spot - FVector(0.0, 0.0, GroundBelow),
		FCollisionObjectQueryParams(ECC_WorldStatic), Params))
	{
		Ground = Hit.ImpactPoint;
	}
	const FBountyTuning Tuning = FBountyTuning::Current();
	const float Drop = bDrop ? Tuning.ChestDrop : 0.f;
	const FTransform At(FRotator(0.f, Yaw, 0.f), Ground + FVector(0.0, 0.0, Drop));
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.ObjectFlags |= RF_Transient;
	SpawnParams.bDeferConstruction = true;
	ABountyChest* Chest = World->SpawnActor<ABountyChest>(ABountyChest::StaticClass(), At, SpawnParams);
	if (!Chest)
	{
		return nullptr;
	}
	// Its kind and loot before its construction (the kind's models), its id for the session's list of opened chests.
	Chest->Serial = Owed.Serial;
	Chest->ChestId = FName(*FString::Printf(TEXT("Bounty_%s_%d"), *BoardId.ToString(), Owed.Serial));
	Chest->Kind = EChestKind::Strongbox;
	Chest->LootOverride = BountyRules::ChestLoot(Owed.Tier, Tuning);
	Chest->GroundSpot = Ground;
	Chest->DropLeft = Drop;
	Chest->DropSpeed = 0.f;
	Chest->FinishSpawning(At);
	if (!Chest->HasActorBegunPlay())
	{
		Chest->DispatchBeginPlay();
	}
	if (Drop > 0.f)
	{
		Chest->SetActorTickEnabled(true);
	}
	UE_LOG(LogLooter, Log, TEXT("Bounty %d: its strongbox %s at (%.0f, %.0f, %.0f)."), Owed.Serial, Drop > 0.f ? TEXT("drops in") : TEXT("stands"),
		Ground.X, Ground.Y, Ground.Z);
	return Chest;
}

FInteractionOptions ABountyChest::GetInteractionOptions(const UInteractionComponent& User) const
{
	FInteractionOptions Options = Super::GetInteractionOptions(User);
	// Not while it's still falling: it opens where it lands.
	Options.bUsable = Options.bUsable && !IsLanding();
	return Options;
}

void ABountyChest::AdvanceDrop(float DeltaSeconds)
{
	if (DropLeft <= 0.f)
	{
		return;
	}
	DropSpeed += Gravity * FMath::Max(DeltaSeconds, 0.f);
	DropLeft = FMath::Max(DropLeft - DropSpeed * FMath::Max(DeltaSeconds, 0.f), 0.f);
	SetActorLocation(GroundSpot + FVector(0.0, 0.0, DropLeft));
	if (DropLeft <= 0.f)
	{
		Land();
	}
}

void ABountyChest::Land()
{
	// Heard and felt: the box into the dirt, its hasp rattling; dust thrown out round its foot.
	LooterSound::PlayAt(this, LooterSoundCue::Bounty::ChestLand, GroundSpot);
	BossCameraShake::Kick(this, GroundSpot, LandShake, LandShakeSeconds, LandShakeRadius);
	UWorld* World = GetWorld();
	UBulletSubsystem* Bullets = World ? World->GetSubsystem<UBulletSubsystem>() : nullptr;
	if (!Bullets)
	{
		return;
	}
	FWeaponFX& Effects = Bullets->GetEffects();
	Effects.Initialize(World);
	for (int32 Puff = 0; Puff < LandPuffs; ++Puff)
	{
		const float Angle = 2.f * UE_PI * (Puff + FMath::FRand() * 0.5f) / LandPuffs;
		const FVector Out(FMath::Cos(Angle), FMath::Sin(Angle), 0.f);
		Effects.SpawnDustPuff(GroundSpot + Out * 45.0 + FVector(0.0, 0.0, 10.0), Out * FMath::FRandRange(140.f, 220.f) + FVector(0.0, 0.0, 40.0),
			LandDust, 0.45f, 25.f, 110.f, FMath::FRandRange(0.9f, 1.4f));
		Effects.SpawnGrit(GroundSpot + Out * 35.0 + FVector(0.0, 0.0, 15.0), Out * FMath::FRandRange(150.f, 260.f) + FVector(0.0, 0.0, 260.f),
			FMath::FRandRange(2.f, 3.5f), 1.6f);
	}
}

void ABountyChest::Tick(float DeltaSeconds)
{
	AdvanceDrop(DeltaSeconds);
	// The chest's own opening (it turns its tick off once it's shut and still, or open for good).
	Super::Tick(DeltaSeconds);
	if (IsLanding())
	{
		SetActorTickEnabled(true);
	}
	CheckLooted();
}

void ABountyChest::CheckLooted()
{
	// Its loot is out (partway up the lid): the board forgets it was owed.
	if (!bLootTold && HasGivenLoot())
	{
		bLootTold = true;
		OnLooted.Broadcast(*this);
	}
}

#undef LOCTEXT_NAMESPACE
