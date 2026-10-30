#include "Creatures/CreatureBase.h"
#include "Areas/AreaDefinition.h"
#include "Areas/AreaRulesSubsystem.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureRankSettings.h"
#include "Creatures/CreatureTierLookComponent.h"
#include "Creatures/CreatureTouched.h"
#include "Creatures/CreatureTraitComponent.h"
#include "Loot/LootDropComponent.h"
#include "Loot/LootLibrary.h"
#include "Loot/KillLootRules.h"
#include "Loot/LootTable.h"
#include "Progression/ProgressionSettings.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Math/RandomStream.h"
#include "Misc/Crc.h"
#include "Templates/TypeHash.h"
#include "TimerManager.h"

// ---------------------------------------------------------------------------
// Rank and level
// ---------------------------------------------------------------------------

/** As play begins: a promotion its area may give a placed Basic creature this arrival, a level from the area's band (or the one it was placed or spawned at), and their stats, loot and size. */
void ACreatureBase::BeginRankAndLevel()
{
	// The level it was placed or spawned at is its own until its area says otherwise (and where the area has no band).
	GivenLevel = FMath::Max(Level, 1);
	// Its area reads its level off its home (UAreaRulesSubsystem::RollLevelFor): here first, where it stands, before
	// BeginPlay settles it on the ground and keeps the settled spot as home.
	Home = GetActorTransform();
	const ECreatureRank ArrivalRank = RollArrivalRank();
	TakeRankAndLevel(ArrivalRank, RollOwnLevel(ArrivalRank));
}

/** Coming back: as its StartingRank (a promotion lasts one life), at a level its area rolls again for the player's now. */
void ACreatureBase::RespawnRankAndLevel()
{
	TierRoll = 0;
	// A new life rolls its touch afresh (a chosen one lasted the life it was chosen for).
	bTouchedChosen = false;
	++TouchedLives;
	TakeRankAndLevel(StartingRank, RollOwnLevel(StartingRank));
}

/**
 * The rank it begins play with: StartingRank, or for a placed Basic creature its area's promotion this arrival (kept under
 * the tiers' caps), whose roll its traits' seed keeps.
 */
ECreatureRank ACreatureBase::RollArrivalRank()
{
	TierRoll = 0;
	// Only placed Basic creatures are promoted on arrival: a placed rank is the level designer's, and spawners roll their own.
	if (StartingRank != ECreatureRank::Basic || bSpawnedAtRuntime)
	{
		return StartingRank;
	}
	UAreaRulesSubsystem* Rules = GetWorld() ? GetWorld()->GetSubsystem<UAreaRulesSubsystem>() : nullptr;
	return Rules ? Rules->RollPromotion(this, &TierRoll) : StartingRank;
}

float ACreatureBase::GetDefense() const
{
	return Health ? Health->GetDefense() : 0.f;
}

int32 ACreatureBase::GetTierSeed() const
{
	// A placed one's name in its level is the same at every load (and in play in the editor); a spawned one never comes back,
	// so its object's id will do. The name's characters, not the FName's own hash, which changes from run to run.
	const uint32 Id = bSpawnedAtRuntime ? GetUniqueID() : FCrc::StrCrc32(*GetName());
	return static_cast<int32>(HashCombineFast(Id, static_cast<uint32>(TierRoll)));
}

/** Its tier's traits from its seed: none below Gravebound, and never a Legendary monster's or a boss's (theirs are their own moves). */
void ACreatureBase::ApplyTraits()
{
	if (!TraitComponent)
	{
		return;
	}
	const int32 Count = LooterRanks::IsTier(CurrentRank) ? UCreatureRankSettings::Get(CurrentRank).TraitCount : 0;
	TraitComponent->SetTraits(Count > 0
		? UCreatureTraitComponent::RollTraits(Count, CurrentRank == ECreatureRank::Ethereal, *this, GetTierSeed())
		: TArray<ECreatureTrait>());
}

/**
 * Its touch this life (CreatureTouched; the user's call, 2026-10-29 night: rare in Ransom's Rest, at most a few alive): only
 * a Basic, Restless or Gravebound creature of a kind with a touch, that may be touched (never a set piece's, a bounty's, a
 * spider fight's or a boss's), in an area that rolls them (not a practice one). From its tier seed and its lives, so a load
 * rolls the same and each new life afresh. A touch SetTouched chose is kept.
 */
void ACreatureBase::ApplyTouched()
{
	if (bTouchedChosen)
	{
		return;
	}
	TouchedElement = ELooterElement::None;
	const ELooterElement Element = CreatureTouched::ElementFor(GetClass());
	const bool bTier = CurrentRank == ECreatureRank::Basic || CurrentRank == ECreatureRank::Rare || CurrentRank == ECreatureRank::Epic;
	UWorld* World = GetWorld();
	if (!bMayBeTouched || Element == ELooterElement::None || !bTier || IsBossCreature() || !World)
	{
		return;
	}
	const UAreaRulesSubsystem* Rules = World->GetSubsystem<UAreaRulesSubsystem>();
	const UAreaDefinition* Area = Rules ? Rules->GetArea() : nullptr;
	if (!Area || Area->bPractice || Area->TouchedChance <= 0.f)
	{
		return;
	}
	int32 Alive = 0;
	for (TActorIterator<ACreatureBase> It(World); It; ++It)
	{
		Alive += (*It != this && !It->IsDead() && It->GetTouchedElement() != ELooterElement::None) ? 1 : 0;
	}
	// Its own dice (salted apart from its traits', which use the same seed).
	constexpr uint32 TouchedSalt = 0x70C4u;
	const FRandomStream Random(static_cast<int32>(HashCombineFast(static_cast<uint32>(GetTierSeed()), TouchedSalt + static_cast<uint32>(TouchedLives))));
	if (CreatureTouched::Rolls(Random.FRand(), Area->TouchedChance, Alive, Area->MaxTouchedAlive))
	{
		TouchedElement = Element;
	}
}

FString ACreatureBase::GetTouchedWord() const
{
	return CreatureTouched::Word(TouchedElement);
}

void ACreatureBase::SetTouched(ELooterElement Element)
{
	bTouchedChosen = true;
	TouchedElement = Element;
	// In play its ward, look and the rest follow now (its stats keep their share); before play BeginPlay applies it.
	if (HasActorBegunPlay() && bBaseCaptured)
	{
		ApplyRank();
	}
}

/** Its own level before its rank's: from where it stands in its area, or GivenLevel (no band, or spawned at one). */
int32 ACreatureBase::RollOwnLevel(ECreatureRank Rank) const
{
	UAreaRulesSubsystem* Rules = !bKeepsGivenLevel && GetWorld() ? GetWorld()->GetSubsystem<UAreaRulesSubsystem>() : nullptr;
	return Rules ? Rules->RollLevelFor(*this, GivenLevel) : GivenLevel;
}

/** Takes Rank with OwnLevel plus the rank's levels, and applies them (ApplyRank). */
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

/** Keeps what the class or the level gave it, once, before any rank changes it. */
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

/** Health, damage and experience from the base ones: grown for its level, then times its rank's multipliers. */
void ACreatureBase::ApplyStats()
{
	const FCreatureRankInfo& Info = UCreatureRankSettings::Get(CurrentRank);

	// Its level grows its health and damage, compounding (+9% a level), from what its class or the level designer gave it
	// (as a gun's damage grows with the gun's level, so a gun of its level always takes the same hits); its rank multiplies
	// that again. Its experience is its rank's share here; the kill adds its level's growth (FLevelRules::KillXP). Changed
	// while hurt, it keeps its share of health; a dead one gets its full health back as it respawns.
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
	// Its tier's defense takes a share of every hit from here on (a crit pierces half: UHealthComponent).
	Health->SetDefense(Info.Defense);
}

/** Applies CurrentRank: stats from the base ones, loot table, size and traits. */
void ACreatureBase::ApplyRank()
{
	CaptureBaseStats();
	ApplyStats();

	// Its own table as Basic (the default one, unless it was given another); a ranked creature carries its rank's, and a kind
	// that pays above its rank (a Stone Sentinel: KillLootRules) that rank's.
	ULootTable* OwnTable = BaseLootTable ? BaseLootTable.Get() : ULootLibrary::GetDefaultLootTable();
	const ECreatureRank LootRank = KillLootRules::LootRankFor(*this, CurrentRank);
	Loot->LootTable = LootRank == ECreatureRank::Basic ? OwnTable : UCreatureRankSettings::GetLootTable(LootRank);

	ApplySize();
	// Its touch before its traits (a Touched creature's ward is its own element) and before its look (the touched glow).
	ApplyTouched();
	// Before OnRankChanged, so the subclass's own rank looks see its traits.
	ApplyTraits();
	if (TraitComponent)
	{
		// The same traits again keep their ward: its element follows a touch that changed with the rank.
		TraitComponent->RefreshWardElement();
	}
	OnRankChanged();
	// Last, so its look sees the rank, size and traits as they are now: the tier's glow, ghost-rim and death (a Regular
	// creature gets nothing; Creatures/CreatureTierLookComponent.h).
	UCreatureTierLookComponent::Refresh(*this);
}

// ---------------------------------------------------------------------------
// Size
// ---------------------------------------------------------------------------

void ACreatureBase::SetBodyScale(float NewBodyScale)
{
	BodyScale = FMath::Max(NewBodyScale, 0.05f);
	ApplySize();
}

/** Scales the actor to BodyScale times its rank's size, its feet staying put, and what the scale doesn't reach. */
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
	// Before play begins: its touch is rolled as its rank is applied there.
	Creature->bMayBeTouched = Creature->bMayBeTouched && Spawn.bMayBeTouched;
	// Before play begins, so its base health (what its level and rank multiply, CaptureBaseStats) includes it.
	if (!FMath::IsNearlyEqual(Spawn.HealthScale, 1.f))
	{
		Creature->Health->MaxHealth = FMath::Max(1.f, Creature->Health->MaxHealth * FMath::Max(Spawn.HealthScale, 0.01f));
	}
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

// ---------------------------------------------------------------------------
// Boss fights: holding back and going home
// ---------------------------------------------------------------------------

void ACreatureBase::SetPassive(bool bInPassive)
{
	bPassive = bInPassive;
	// It lets go at once, unless it's mid-attack: the attack plays out, then its senses let go (UpdatePerception).
	if (bPassive && Target.IsValid() && State != ECreatureState::Attack && State != ECreatureState::Dead)
	{
		Target.Reset();
		SetState(ECreatureState::Return);
	}
}

void ACreatureBase::ResetToHome()
{
	// As Respawn brings it back, but at once and with its rank and level as they are: a boss fight starting over.
	FTimerManager& Timers = GetWorldTimerManager();
	Timers.ClearTimer(HideTimer);
	Timers.ClearTimer(RespawnTimer);
	Target.Reset();
	SetActorLocationAndRotation(Home.GetLocation(), Home.GetRotation(), false, nullptr, ETeleportType::ResetPhysics);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->StopMovementImmediately();
	Movement->SetMovementMode(MOVE_Walking);
	Health->ResetHealth();
	SetHitVolumesEnabled(true);
	SetActorHiddenInGame(false);
	CooldownRemaining = 0.f;
	HealthBarTime = 0.f;
	SetState(ECreatureState::Idle);
	IdleDuration = 2.f;
	OnRespawned();
}
