// ASlimeCreature's fight pass (FIGHTS.md section 3, lane 4, 2026-10-28): a Restless or better slime dies into slimelets, the pack's
// leaps keep a drumbeat (the per-kind melee gap the fight director reads), and a slime that takes quick hits while it waits its
// turn hops aside. The numbers and the plain-data rules are SlimeFightRules.h; the hops are SlimeCreature.cpp's.

#include "Creatures/SlimeCreature.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Creatures/CreatureRankSettings.h"
#include "Creatures/EncounterSettings.h"
#include "Creatures/EncounterSubsystem.h"
#include "Creatures/FightDirectorSubsystem.h"
#include "Creatures/FightEvent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
	/** The split's gel burst: its cue (Creature.Slime.Split, Art/Sounds/recipes/fight_kit.py). */
	const FName SplitCue(TEXT("Creature.Slime.Split"));
	/** The slimelets' name on their tag (they keep the slime's bestiary page, which is by class). */
	const TCHAR* SlimeletName = TEXT("Slimelet");
	/** A thrown piece starts this share of its way out from the corpse, so the two pieces don't spawn inside each other. */
	constexpr float StartShare = 0.25f;
	/** The side hop's height as a share of its length: a full-size slime's chase hop (70 to 90 cm over 200 to 260). */
	constexpr float ThrowHeightShare = 1.f / 3.f;
}

float ASlimeCreature::GetMeleeGap() const
{
	return FSlimeFightTuning::Live().MeleeGap;
}

// ---------------------------------------------------------------------------
// Slimelets
// ---------------------------------------------------------------------------

void ASlimeCreature::OnRankChanged()
{
	Super::OnRankChanged();
	// The rank has just set damage and experience from the class's base: a slimelet's shares go on top, every time.
	if (bSlimelet)
	{
		ApplySlimeletStats();
	}
}

void ASlimeCreature::ApplySlimeletStats()
{
	AttackDamage *= SlimeletDamageShare;
	XPReward = FMath::Max(0, FMath::RoundToInt32(XPReward * SlimeletXPShare));
}

void ASlimeCreature::MarkSlimelet(const FSlimeFightTuning& Tuning)
{
	bSlimelet = true;
	SlimeletDamageShare = Tuning.SplitDamage;
	SlimeletXPShare = Tuning.SplitXP;
	DisplayName = FText::FromString(SlimeletName);
	// Spawned in a played level its BeginPlay has already set its stats, so the shares go on now; in a level that hasn't begun
	// play (a test's) they go on when OnRankChanged runs as it begins.
	if (HasActorBegunPlay())
	{
		ApplySlimeletStats();
	}
}

void ASlimeCreature::ThrowNow(const FVector& Direction, float Length)
{
	HopDirection = Direction.GetSafeNormal2D();
	if (HopDirection.IsNearlyZero())
	{
		HopDirection = FVector::ForwardVector;
	}
	bShuffleHop = false;
	bLeaping = false;
	Hop = EHop::Air;
	HopTime = 0.f;
	LaunchedFrom = GetActorLocation();
	PlannedLength = Length;
	HopLaunchVelocity = HopVelocity(HopDirection, Length, Length * ThrowHeightShare, -GetCharacterMovement()->GetGravityZ());
	LaunchCharacter(HopLaunchVelocity, true, true);
	Squash.Ramp(1.3f, 0.08f);
}

void ASlimeCreature::SplitOnDeath()
{
	UWorld* World = GetWorld();
	const FSlimeFightTuning Tuning = FSlimeFightTuning::Live();
	if (!World || !Tuning.bSplit || bSlimelet)
	{
		return;
	}

	// The caps count the pieces as they stand: the slimes alive in the level (this one is dead already), the creatures near the
	// player (the level's cap on them), and the slime's class cap where it has one.
	const UEncounterSubsystem* Encounters = UEncounterSubsystem::Get(this);
	APawn* Player = Encounters ? Encounters->GetPlayer() : nullptr;
	const UEncounterSettings& Settings = UEncounterSettings::Get();
	const int32 SlimesAlive = Encounters ? Encounters->CountAlive(ASlimeCreature::StaticClass()) : 0;
	const int32 NearAlive = Encounters && Player ? Encounters->CountAliveNear(Player->GetActorLocation(), Settings.NearPlayerRadius) : 0;
	const int32 Pieces = SlimeFightRules::SplitPieces(Tuning, GetRank(), bSlimelet, SlimesAlive, NearAlive,
		Player ? Settings.MaxCreaturesNearPlayer : 0);
	if (Pieces <= 0)
	{
		return;
	}
	const UClass* Counted = nullptr;
	const int32 ClassCap = Settings.FindClassCap(GetClass(), &Counted);
	if (Encounters && ClassCap > 0 && Encounters->CountAlive(Counted) + Pieces > ClassCap)
	{
		return;
	}

	const FCreatureRuntimeSpawn Setup = SlimeFightRules::SlimeletSpawn(Tuning, Level, UCreatureRankSettings::Get(GetRank()).LevelOffset);
	const FVector Feet = GetActorLocation() - FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	const float Yaw = GetActorRotation().Yaw;
	const TArray<FVector> Offsets = SlimeFightRules::SplitOffsets(Pieces, Tuning.SplitSpread, FMath::FRandRange(0.f, 360.f));
	int32 Made = 0;
	for (const FVector& Offset : Offsets)
	{
		ASlimeCreature* Piece = Cast<ASlimeCreature>(
			ACreatureBase::SpawnAtRuntime(World, ASlimeCreature::StaticClass(), Feet + Offset * StartShare, Yaw, Setup));
		if (!Piece)
		{
			continue;
		}
		++Made;
		Piece->MarkSlimelet(Tuning);
		// In a level that isn't playing yet (a test level), it starts as play would start it.
		if (!Piece->HasActorBegunPlay())
		{
			Piece->DispatchBeginPlay();
		}
		// Thrown out to its spot: the pieces land Spread apart. They join the slimes' pack (its tag), and turn on the player at once.
		Piece->ThrowNow(Offset, Offset.Size2D() * (1.f - StartShare));
		if (Player && FVector::DistSquared(Player->GetActorLocation(), Feet) <= FMath::Square(PackAlertRadius))
		{
			Piece->AlertTo(Player);
		}
	}
	if (Made == 0)
	{
		return;
	}
	LooterSound::PlayAt(this, SplitCue, Feet + FVector(0.f, 0.f, 30.f));
	if (UFightDirectorSubsystem* Director = UFightDirectorSubsystem::Get(this))
	{
		Director->NoteEvent(*this, EFightEvent::Split, static_cast<float>(Made));
	}
}

// ---------------------------------------------------------------------------
// The dodge hop
// ---------------------------------------------------------------------------

void ASlimeCreature::NoteHitForDodge()
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	const FSlimeFightTuning Tuning = FSlimeFightTuning::Live();
	const APawn* Hunted = GetTarget();
	const ECreatureState CreatureState = GetCreatureState();
	// Waiting its turn on the ring (chasing, not holding a turn); not busy: mid-hop, in its leap's telegraph or attack, held by a hit.
	const bool bWaiting = Hunted && CreatureState == ECreatureState::Chase && IsWaitingTurn();
	const bool bBusy = Hop != EHop::Ground || bTelegraph || bLeaping || CreatureState == ECreatureState::Attack || IsHeldByHit();
	if (!DodgeTracker.Note(World->GetTimeSeconds(), Tuning, bWaiting, bBusy, FMath::FRand()) || !Hunted)
	{
		return;
	}

	// A side hop across the line to its target, to whichever side has ground (never off a ledge).
	const float Scale = GetSizeScale();
	const float Length = Tuning.DodgeLength * Scale;
	const FVector ToTarget = Hunted->GetActorLocation() - GetActorLocation();
	const bool bFirstRight = FMath::RandBool();
	for (int32 Try = 0; Try < 2; ++Try)
	{
		const FVector Way = SlimeFightRules::DodgeDirection(ToTarget, bFirstRight != (Try == 1));
		FVector Ground;
		if (!FindGround(GetActorLocation() + Way * Length, 300.f * Scale, 600.f * Scale, Ground))
		{
			continue;
		}
		ThrowNow(Way, Length);
		if (UFightDirectorSubsystem* Director = UFightDirectorSubsystem::Get(this))
		{
			Director->NoteEvent(*this, EFightEvent::Dodge);
		}
		return;
	}
}
