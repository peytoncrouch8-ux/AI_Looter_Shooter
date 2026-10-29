// ASetPieceSpawner: springing, its pushes and their beats, and its end (the flag, the shower). Its placed creatures and the
// speakers it shuts are in SetPieceSpawnerPlaced.cpp, its arena's fog gates in SetPieceSpawnerArena.cpp; its rules
// (SetPieceRules) and cvars in SetPieceRules.cpp.

#include "Creatures/SetPieceSpawner.h"
#include "AI_Looter_Shooter.h"
#include "Areas/AreaRulesSubsystem.h"
#include "Audio/LooterSound.h"
#include "Bosses/BossCameraShake.h"
#include "Bosses/BossLootShower.h"
#include "Bosses/BossTypes.h"
#include "Combat/BulletSubsystem.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/EncounterSubsystem.h"
#include "Creatures/SetPieceRules.h"
#include "Loot/LootDropComponent.h"
#include "Loot/LootLibrary.h"
#include "Loot/LootTable.h"
#include "Missions/MissionRunner.h"
#include "Story/CaptionSubsystem.h"
#include "Story/StoryLine.h"
#include "Weapons/WeaponFX.h"
#include "World/WorldQueries.h"
#include "CollisionQueryParams.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"

namespace
{
	/** Dust sifting from a mine's roof: dull, unlit, as the Foreman's rockfall's (ForemanRockFall.cpp). */
	const FLinearColor SetPieceDust(0.3f, 0.27f, 0.23f);

	/** It falls from roofs within this far over the player (cm; the drift is under 6 m high), over this wide a disc round them. */
	constexpr float DustRoofSearch = 1200.f;
	constexpr float DustSpread = 700.f;

	/** The done watch looks this often (s) while its placed creatures outlive its spawned ones. */
	constexpr float DoneWatchSeconds = 0.5f;

	/** The shower waits this long after the last falls (s): the body drops first, as a boss's. */
	constexpr float RewardDelay = 0.8f;
}

ASetPieceSpawner::ASetPieceSpawner()
{
	// Its reward's table and level; it has no health, so nothing drops on its own (ABossLootShower::Throw reads it).
	RewardLoot = CreateDefaultSubobject<ULootDropComponent>(TEXT("RewardLoot"));
	RewardLoot->bDropOnDeath = false;
	RewardLoot->bDropSoulMotes = false;
	// A set piece is pushes by design: the last one's latest (s), should its stragglers hang back out of reach.
	WaveInterval = 30.f;
}

void ASetPieceSpawner::BeginPlay()
{
	// Once a session: off whenever its flag is set (a load after it was won, the rest of this visit).
	if (!DoneFlag.IsNone())
	{
		ActiveWhen.WithoutFlags.AddUnique(DoneFlag);
	}
	Super::BeginPlay();
}

void ASetPieceSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DoneWatch);
	}
	Unhush();
	DestroyGates();
	if (EndPlayReason == EEndPlayReason::Destroyed)
	{
		RemovePlaced(/*bOweThem*/ false);
	}
	Super::EndPlay(EndPlayReason);
}

// ---------------------------------------------------------------------------
// Springing and its pushes
// ---------------------------------------------------------------------------

bool ASetPieceSpawner::CheckApproach()
{
	if (bDone || IsHeld())
	{
		return false;
	}
	const UEncounterSubsystem* Encounters = GetEncounters();
	const APawn* Player = Encounters ? Encounters->GetPlayer() : nullptr;
	if (!Player)
	{
		return false;
	}
	// Near its spot (a mine's drift: the way out runs through it), or walking onto its ground (an ambush's rule: nobody wakes
	// into a fight).
	const bool bSpring = SpringRadius > 0.f
		? FVector::DistSquared(Player->GetActorLocation(), GetActorLocation()) <= FMath::Square(static_cast<double>(SpringRadius))
		: Super::CheckApproach();
	if (bSpring)
	{
		// Its pushes come round here when they come round the player (and again when it springs anew after a retreat).
		SprungAt = Player->GetActorLocation();
		bSprung = true;
	}
	return bSpring;
}

FVector ASetPieceSpawner::GetSpawnCenter() const
{
	return bAroundPlayer && bSprung ? SprungAt : Super::GetSpawnCenter();
}

void ASetPieceSpawner::OnWaveStarted(int32 Wave)
{
	TArray<int32, TInlineAllocator<4>> BeatWaves;
	for (const FSetPieceBeat& Beat : Beats)
	{
		BeatWaves.Add(Beat.Wave);
	}
	const int32 BeatIndex = SetPieceRules::FindBeat(BeatWaves, Wave);
	if (BeatIndex != INDEX_NONE)
	{
		const FSetPieceBeat& Beat = Beats[BeatIndex];
		// This push's own side (the alleys behind, after the ones ahead); round the player when it has none.
		if (Beat.Spots.Num() > 0 || bAroundPlayer)
		{
			SpawnPoints = Beat.Spots;
		}
		// The first is the warning: it cuts in over whatever was being said.
		PlayBeat(Beat, /*bInterrupt*/ Wave == 1);
	}
	if (Wave == 1)
	{
		Hush();
	}
	SpawnPlacedForWave(Wave);
	UE_LOG(LogLooter, Log, TEXT("Set piece %s: push %d of %d."), *GetSpawnerId().ToString(), Wave, GetPlannedWaves());
}

void ASetPieceSpawner::OnCreatureSpawned(ACreatureBase& Creature)
{
	Super::OnCreatureSpawned(Creature);
	HighestLevel = FMath::Max(HighestLevel, Creature.Level);
}

void ASetPieceSpawner::UpdatePack(float DeltaSeconds)
{
	Super::UpdatePack(DeltaSeconds);
	PrunePlaced();
	UpdateArena();
	if (Hushed.Num() > 0 && !SetPieceRules::KeepsHushed(GetState(), GetWavesStarted(), bDone))
	{
		Unhush();
	}
	const int32 Threshold = SetPieceRules::PushThreshold(PushWhenLeft, FSetPieceTuning::Current().PushWhenLeft);
	if (SetPieceRules::IsPushDue(GetState(), GetWavesStarted(), GetPlannedWaves(), NumAlive() + NumOwed(), Threshold))
	{
		// On top of the last one's stragglers, from its own side.
		TriggerWave();
	}
}

void ASetPieceSpawner::OnStateChanged(EEncounterState From, EEncounterState To)
{
	Super::OnStateChanged(From, To);
	const bool bBegun = GetWavesStarted() > 0;
	// The fog gates stand only while it fights: its own creatures down (Cleared), taken away (Waiting) or off, they open.
	if (To != EEncounterState::Engaged)
	{
		DropGates();
	}
	switch (To)
	{
	case EEncounterState::Cleared:
		// Its own creatures are down; done once its placed ones are too.
		if (bBegun && !bDone && !CheckDone())
		{
			StartDoneWatch();
		}
		break;

	case EEncounterState::Waiting:
		// Taken away while the player is far (or dead and gone to a roost): its placed ones go with the rest, owed back.
		if (From == EEncounterState::Engaged && bBegun && DespawnRadius > 0.f && !IsPlayerWithin(DespawnRadius))
		{
			RemovePlaced(/*bOweThem*/ true);
		}
		break;

	case EEncounterState::Engaged:
		// Sprung again on the player's return: what was owed comes back with the rest.
		if (From == EEncounterState::Waiting && bBegun)
		{
			BringPlacedBack();
		}
		break;

	case EEncounterState::Off:
		// Its story ended (its mission turned in, its flag set): nothing of it stays, and the speakers talk again.
		RemovePlaced(/*bOweThem*/ false);
		PlacedOwed.Reset();
		Unhush();
		bSprung = false;
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(DoneWatch);
		}
		break;

	default:
		break;
	}
}

// ---------------------------------------------------------------------------
// Its beats
// ---------------------------------------------------------------------------

void ASetPieceSpawner::PlayBeat(const FSetPieceBeat& Beat, bool bInterrupt)
{
	const FSetPieceTuning Tuning = FSetPieceTuning::Current();
	const UEncounterSubsystem* Encounters = GetEncounters();
	const APawn* Player = Encounters ? Encounters->GetPlayer() : nullptr;
	const FVector Where = Player ? Player->GetActorLocation() : GetActorLocation();
	if (!Beat.Line.IsEmpty())
	{
		if (UCaptionSubsystem* Captions = UCaptionSubsystem::Get(this))
		{
			FStoryLine Said;
			Said.Speaker = Beat.Speaker;
			Said.Text = Beat.Line;
			Captions->Play({ Said }, bInterrupt ? ECaptionPlay::Interrupt : ECaptionPlay::Queue);
		}
	}
	if (!Beat.StingCue.IsNone())
	{
		LooterSound::Play2D(this, Beat.StingCue);
	}
	if (!Beat.WorldCue.IsNone())
	{
		LooterSound::PlayAt(this, Beat.WorldCue, Where);
	}
	const float Shake = SetPieceRules::ShakeStrength(Beat.Shake, Tuning.ShakeScale);
	if (Shake > 0.f)
	{
		BossCameraShake::Kick(this, Where, Shake, FMath::Max(Beat.ShakeSeconds, 0.1f));
	}
	const int32 Puffs = SetPieceRules::DustPuffs(Beat.DustPuffs, Tuning.DustScale);
	if (Puffs > 0)
	{
		SiftDust(Where, Puffs);
	}
}

void ASetPieceSpawner::SiftDust(const FVector& Around, int32 Puffs) const
{
	UWorld* World = GetWorld();
	UBulletSubsystem* Bullets = World ? World->GetSubsystem<UBulletSubsystem>() : nullptr;
	if (!Bullets)
	{
		return;
	}
	FWeaponFX& Effects = Bullets->GetEffects();
	Effects.Initialize(World);
	const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("SetPieceDust"), this, /*bTraceComplex*/ false);
	for (int32 Puff = 0; Puff < Puffs; ++Puff)
	{
		// One look up each, once: dust only where there's a roof to shake it loose (none under the open sky).
		const FVector2D Offset = FMath::RandPointInCircle(DustSpread);
		const FVector Start = Around + FVector(Offset.X, Offset.Y, 50.0);
		FHitResult Roof;
		if (!World->LineTraceSingleByObjectType(Roof, Start, Start + FVector(0.0, 0.0, DustRoofSearch),
			FCollisionObjectQueryParams(ECC_WorldStatic), Params))
		{
			continue;
		}
		const FVector From = Roof.ImpactPoint - FVector(0.0, 0.0, 20.0);
		const FVector Fall(FMath::FRandRange(-25.f, 25.f), FMath::FRandRange(-25.f, 25.f), -FMath::FRandRange(220.f, 340.f));
		Effects.SpawnDustPuff(From, Fall, SetPieceDust, 0.5f, 15.f, 80.f, FMath::FRandRange(1.6f, 2.4f));
		Effects.SpawnGrit(From, FVector(FMath::FRandRange(-30.f, 30.f), FMath::FRandRange(-30.f, 30.f), -140.f), FMath::FRandRange(2.f, 4.f), 2.f);
	}
}

// ---------------------------------------------------------------------------
// Its end
// ---------------------------------------------------------------------------

bool ASetPieceSpawner::CheckDone()
{
	if (bDone)
	{
		return true;
	}
	PrunePlaced();
	if (!SetPieceRules::IsDone(GetState(), GetWavesStarted(), NumPlacedAlive(), PlacedOwed.Num()))
	{
		return false;
	}
	Finish();
	return true;
}

void ASetPieceSpawner::StartDoneWatch()
{
	UWorld* World = GetWorld();
	if (World && !World->GetTimerManager().IsTimerActive(DoneWatch))
	{
		World->GetTimerManager().SetTimer(DoneWatch, this, &ASetPieceSpawner::HandleDoneWatch, DoneWatchSeconds, /*bLoop*/ true);
	}
}

void ASetPieceSpawner::HandleDoneWatch()
{
	// The player gone far with a placed one still up (a Spitter on its rock): it gives up, and the fight counts as won.
	if (NumPlacedAlive() > 0 && DespawnRadius > 0.f && !IsPlayerWithin(DespawnRadius))
	{
		RemovePlaced(/*bOweThem*/ false);
	}
	if (CheckDone())
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(DoneWatch);
		}
	}
}

void ASetPieceSpawner::Finish()
{
	bDone = true;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DoneWatch);
	}
	DropGates();
	Unhush();
	PlayBeat(ClearedBeat, /*bInterrupt*/ false);
	ThrowReward();
	UE_LOG(LogLooter, Log, TEXT("Set piece %s: done (%d killed), its flag %s set."), *GetSpawnerId().ToString(), GetKilled(), *DoneFlag.ToString());
	// Last: the flag tells the missions, and every spawner reads its story again (this one stays cleared for the visit).
	if (!DoneFlag.IsNone())
	{
		if (UMissionRunner* Runner = UMissionRunner::Get(this))
		{
			Runner->SetFlag(DoneFlag);
		}
	}
}

void ASetPieceSpawner::ThrowReward()
{
	if (!bReward || !RewardLoot)
	{
		return;
	}
	// A Strongbox's worth, thrown as a boss's shower: the game's guns by the default table's weights, every one a gun at the
	// reward's luck, chest-full ammo; no silver (silver drops apart from the tables, so the purse doesn't move).
	const FSetPieceTuning Tuning = FSetPieceTuning::Current();
	ULootTable* Table = NewObject<ULootTable>(this, NAME_None, RF_Transient);
	if (const ULootTable* Default = ULootLibrary::GetDefaultLootTable())
	{
		Table->Entries = Default->Entries;
		Table->AmmoTypes = Default->AmmoTypes;
	}
	Table->WeaponDropChance = UAreaRulesSubsystem::DropsGunsAt(this) && Tuning.RewardGuns > 0 ? 1.f : 0.f;
	Table->MinWeaponDrops = Tuning.RewardGuns;
	Table->MaxWeaponDrops = Tuning.RewardGuns;
	Table->Luck = Tuning.RewardLuck;
	Table->AmmoDropChance = Tuning.RewardAmmo > 0 ? 1.f : 0.f;
	Table->MinAmmoDrops = Tuning.RewardAmmo;
	Table->MaxAmmoDrops = Tuning.RewardAmmo;
	Table->KillWeaponAmmoBias = 1.f;
	Table->UseChestAmmoAmount();
	RewardLoot->LootTable = Table;
	RewardLoot->Level = SetPieceRules::RewardLevel(HighestLevel);
	FBossLootShowerSettings Shower;
	Shower.BonusAmmo = Tuning.RewardBonusAmmo;
	Shower.Delay = RewardDelay;
	const FVector From = GetActorTransform().TransformPositionNoScale(RewardSpot);
	if (!ABossLootShower::Throw(*this, From, Shower))
	{
		UE_LOG(LogLooter, Warning, TEXT("Set piece %s: its loot shower couldn't be thrown."), *GetSpawnerId().ToString());
	}
}
