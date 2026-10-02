#include "Creatures/CreatureBase.h"
#include "Areas/AreaRulesSubsystem.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureRankSettings.h"
#include "Loot/LootDropComponent.h"
#include "Loot/LootLibrary.h"
#include "Loot/LootTable.h"
#include "Progression/ProgressionSettings.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"

// ---------------------------------------------------------------------------
// Rank and level
// ---------------------------------------------------------------------------

void ACreatureBase::BeginRankAndLevel()
{
	// The level it was placed or spawned at is its own until its area says otherwise (and where the area has no band).
	GivenLevel = FMath::Max(Level, 1);
	const ECreatureRank ArrivalRank = RollArrivalRank();
	TakeRankAndLevel(ArrivalRank, RollOwnLevel(ArrivalRank));
}

void ACreatureBase::RespawnRankAndLevel()
{
	TakeRankAndLevel(StartingRank, RollOwnLevel(StartingRank));
}

ECreatureRank ACreatureBase::RollArrivalRank() const
{
	// Only placed Basic creatures are promoted on arrival: a placed rank is the level designer's, and spawners roll their own.
	if (StartingRank != ECreatureRank::Basic || bSpawnedAtRuntime)
	{
		return StartingRank;
	}
	UAreaRulesSubsystem* Rules = GetWorld() ? GetWorld()->GetSubsystem<UAreaRulesSubsystem>() : nullptr;
	return Rules ? Rules->RollPromotion() : StartingRank;
}

int32 ACreatureBase::RollOwnLevel(ECreatureRank Rank) const
{
	UAreaRulesSubsystem* Rules = !bKeepsGivenLevel && GetWorld() ? GetWorld()->GetSubsystem<UAreaRulesSubsystem>() : nullptr;
	return Rules ? Rules->RollLevel(GivenLevel, Rank) : GivenLevel;
}

void ACreatureBase::TakeRankAndLevel(ECreatureRank Rank, int32 OwnLevel)
{
	CurrentRank = Rank;
	SetLevel(OwnLevel + UCreatureRankSettings::Get(Rank).LevelOffset);
	ApplyRank();
}

void ACreatureBase::SetRank(ECreatureRank NewRank)
{
	if (!HasActorBegunPlay() && !IsActorBeginningPlay())
	{
		// Not in play yet: it starts with this rank, which BeginPlay applies.
		StartingRank = NewRank;
		return;
	}
	// The level moves by the difference between the ranks' offsets, so the level it was given stays its own.
	const int32 OffsetChange = UCreatureRankSettings::Get(NewRank).LevelOffset - UCreatureRankSettings::Get(CurrentRank).LevelOffset;
	CurrentRank = NewRank;
	SetLevel(Level + OffsetChange);
	ApplyRank();
}

void ACreatureBase::SetLevel(int32 NewLevel)
{
	Level = FMath::Max(NewLevel, 1);
	// The guns it drops are its level (before levels reached the loot, every creature dropped level 1 guns).
	if (Loot)
	{
		Loot->Level = Level;
	}
	// In play its health and damage follow; before play, BeginPlay applies them with its rank.
	if (bBaseCaptured)
	{
		ApplyStats();
	}
}

void ACreatureBase::CaptureBaseStats()
{
	if (bBaseCaptured)
	{
		return;
	}
	bBaseCaptured = true;
	BaseMaxHealth = Health->MaxHealth;
	BaseAttackDamage = AttackDamage;
	BaseXPReward = XPReward;
	BaseLootTable = Loot->LootTable;
	BaseStepHeight = GetCharacterMovement()->MaxStepHeight;
}

void ACreatureBase::ApplyStats()
{
	const FCreatureRankInfo& Info = UCreatureRankSettings::Get(CurrentRank);

	// Its level grows its health and damage linearly from what its class or the level designer gave it (as a gun's damage
	// grows with the gun's level, so a gun of its level always takes the same hits); its rank multiplies that again. Its
	// experience is its rank's share here; the kill adds its level's growth (FLevelRules::KillXP). Changed while hurt, it
	// keeps its share of health; a dead one gets its full health back as it respawns.
	const float LevelScale = GetDefault<UProgressionSettings>()->GetLevelRules().EnemyScale(Level);
	const bool bKeepShare = Health->HasBegunPlay() && !Health->IsDead();
	const float HealthShare = Health->GetHealthPercent();
	Health->MaxHealth = FMath::Max(1.f, BaseMaxHealth * LevelScale * Info.HealthMultiplier);
	if (bKeepShare)
	{
		Health->SetHealth(HealthShare * Health->MaxHealth);
	}
	AttackDamage = BaseAttackDamage * LevelScale * Info.DamageMultiplier;
	XPReward = FMath::Max(0, FMath::RoundToInt32(BaseXPReward * Info.XPMultiplier));
}

void ACreatureBase::ApplyRank()
{
	CaptureBaseStats();
	ApplyStats();

	// Its own table as Basic (the default one, unless it was given another); a ranked creature carries its rank's.
	ULootTable* OwnTable = BaseLootTable ? BaseLootTable.Get() : ULootLibrary::GetDefaultLootTable();
	Loot->LootTable = CurrentRank == ECreatureRank::Basic ? OwnTable : UCreatureRankSettings::GetLootTable(CurrentRank);

	ApplySize();
}

// ---------------------------------------------------------------------------
// Size
// ---------------------------------------------------------------------------

void ACreatureBase::SetBodyScale(float NewBodyScale)
{
	BodyScale = FMath::Max(NewBodyScale, 0.05f);
	ApplySize();
}

void ACreatureBase::ApplySize()
{
	CaptureBaseStats();
	const float RankSize = FMath::Max(UCreatureRankSettings::Get(CurrentRank).Size, 0.05f);

	// The whole actor scales: the capsule (and so its movement), the model with its hit zones, and the health bar's height
	// over it. Its feet stay where they stand, so it grows up from the ground rather than into it.
	const UCapsuleComponent* Capsule = GetCapsuleComponent();
	const float OldHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	SizeScale = FMath::Max(BodyScale, 0.05f) * RankSize;
	SetActorScale3D(FVector(SizeScale));
	const float Rise = Capsule->GetScaledCapsuleHalfHeight() - OldHalfHeight;
	if (!FMath::IsNearlyZero(Rise))
	{
		AddActorWorldOffset(FVector(0.f, 0.f, Rise), false, nullptr, ETeleportType::TeleportPhysics);
	}

	// What the scale doesn't reach: the step it can walk up. Every other distance in the code reads GetSizeScale().
	GetCharacterMovement()->MaxStepHeight = BaseStepHeight * SizeScale;
	OnSizeChanged();
}

float ACreatureBase::GetStrikeReach() const
{
	// The strike lands a little past where the attack starts: a target backing off as it lunges still gets hit.
	return (AttackRange + 70.f) * SizeScale;
}

// ---------------------------------------------------------------------------
// Packs and respawns
// ---------------------------------------------------------------------------

bool ACreatureBase::SharesPackWith(const ACreatureBase& Other) const
{
	if (PackTag.IsNone() || Other.PackTag.IsNone())
	{
		return Other.GetClass() == GetClass();
	}
	return Other.PackTag == PackTag;
}

float ACreatureBase::GetPackCallRadius() const
{
	return FMath::Max(PackAlertRadius, UCreatureRankSettings::Get(CurrentRank).PackCallRadius);
}

bool ACreatureBase::WillRespawn() const
{
	// Only placed creatures come back, as their starting rank, and never a Legendary monster or a boss: their area decides.
	return bRespawns && !bSpawnedAtRuntime && UCreatureRankSettings::Get(StartingRank).bRespawns;
}

ACreatureBase* ACreatureBase::SpawnAtRuntime(UWorld* World, TSubclassOf<ACreatureBase> Class, const FVector& Feet, float Yaw,
	const FRuntimeSpawn& Spawn)
{
	if (!World || !Class)
	{
		return nullptr;
	}
	const ACreatureBase* Defaults = Class->GetDefaultObject<ACreatureBase>();
	const float StartScale = Spawn.BodyScale > 0.f ? Spawn.BodyScale : Defaults->BodyScale;
	// On the ground at the size it will be (BeginPlay settles it on the ground again once it's scaled).
	const float HalfHeight = Defaults->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() * StartScale
		* UCreatureRankSettings::Get(Spawn.Rank).Size;
	const FTransform SpawnTransform(FRotator(0.f, Yaw, 0.f), Feet + FVector(0.f, 0.f, HalfHeight + 2.f));
	ACreatureBase* Creature = World->SpawnActorDeferred<ACreatureBase>(Class, SpawnTransform, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (!Creature)
	{
		return nullptr;
	}
	Creature->StartingRank = Spawn.Rank;
	Creature->BodyScale = StartScale;
	if (Spawn.Level > 0)
	{
		// A level asked for is kept; otherwise the area's band gives it one, as it gives a placed creature.
		Creature->Level = Spawn.Level;
		Creature->bKeepsGivenLevel = true;
	}
	// Everything spawners, egg sacs and commands make is gone for good once killed: only placed creatures come back.
	Creature->bRespawns = false;
	Creature->bSpawnedAtRuntime = true;
	Creature->FinishSpawning(SpawnTransform);
	return Creature;
}
