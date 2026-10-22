// UFightDirectorSubsystem: the turns (who may attack the player now) and the look at every fight that keeps them honest.
// FightDirectorPlaces.cpp is where the waiting ones stand: the ring's slots and the ranged kinds' spots.

#include "Creatures/FightDirectorSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreaturePackComponent.h"
#include "Creatures/EncounterSubsystem.h"
#include "CoreGlobals.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"

bool UFightDirectorSubsystem::bAllowInTestWorlds = false;

namespace
{
	/** A melee holder this many approach distances from its target has gone out of reach: its turn goes back. */
	constexpr float LeaveShare = 1.5f;

	/** The seed of the director's dice (feints, moving on after a shot): any will do, the same each run. */
	constexpr int32 FightRollSeed = 0x0F16D;
}

bool UFightDirectorSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return Super::DoesSupportWorldType(WorldType) || (bAllowInTestWorlds && WorldType == EWorldType::EditorPreview);
}

bool UFightDirectorSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (!Super::ShouldCreateSubsystem(Outer))
	{
		return false;
	}
	const UWorld* World = Cast<UWorld>(Outer);
	if (!World)
	{
		return false;
	}
	const EWorldType::Type Type = World->WorldType;
	return Type == EWorldType::Game || Type == EWorldType::PIE || (bAllowInTestWorlds && Type == EWorldType::EditorPreview);
}

void UFightDirectorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Rolls.Initialize(FightRollSeed);
}

UFightDirectorSubsystem* UFightDirectorSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	UFightDirectorSubsystem* Director = World ? World->GetSubsystem<UFightDirectorSubsystem>() : nullptr;
	return Director && Director->Tuning().Director != 0 ? Director : nullptr;
}

const FFightTuning& UFightDirectorSubsystem::Tuning()
{
	// Read once a frame: every chaser asks several times a frame, and a change in PIE still counts by the next.
	if (TuningFrame != GFrameCounter)
	{
		TuningFrame = GFrameCounter;
		FrameTuning = FFightTuning::Live();
	}
	return FrameTuning;
}

double UFightDirectorSubsystem::Now() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetTimeSeconds() : 0.0;
}

// ---------------------------------------------------------------------------
// Turns
// ---------------------------------------------------------------------------

bool UFightDirectorSubsystem::TryTakeToken(const ACreatureBase& Creature, const APawn& Target, EFightRole Role)
{
	RefreshIfDue();
	const FFightTuning& Numbers = Tuning();
	FTargetFight& Fight = Fights.FindOrAdd(&Target);
	const uint32 Id = Creature.GetUniqueID();
	if (Fight.Melee.Holds(Id) || Fight.Ranged.Holds(Id))
	{
		return true;
	}
	const bool bRanged = Role == EFightRole::Ranged;
	FFightTokens& Tokens = bRanged ? Fight.Ranged : Fight.Melee;
	const bool bTaken = Tokens.TryTake(Id, Now(), bRanged ? Numbers.RangedTokens : Numbers.MeleeTokens, Numbers.TurnKeep,
		bRanged ? Numbers.RangedGap : 0.f);
	if (bTaken)
	{
		UE_LOG(LogLooter, Verbose, TEXT("%s takes a %s turn at %s (%d held)."), *Creature.GetName(), bRanged ? TEXT("ranged") : TEXT("melee"),
			*Target.GetName(), Tokens.NumHeld());
	}
	return bTaken;
}

bool UFightDirectorSubsystem::HoldsToken(const ACreatureBase& Creature) const
{
	const uint32 Id = Creature.GetUniqueID();
	for (const TPair<TWeakObjectPtr<const APawn>, FTargetFight>& Each : Fights)
	{
		if (Each.Value.Melee.Holds(Id) || Each.Value.Ranged.Holds(Id))
		{
			return true;
		}
	}
	return false;
}

void UFightDirectorSubsystem::EndTurn(const ACreatureBase& Creature)
{
	const FFightTuning& Numbers = Tuning();
	const uint32 Id = Creature.GetUniqueID();
	const double Time = Now();
	for (TPair<TWeakObjectPtr<const APawn>, FTargetFight>& Each : Fights)
	{
		FTargetFight& Fight = Each.Value;
		if (Fight.Melee.Holds(Id))
		{
			Fight.Melee.Release(Id, Time, Numbers.TokenRest);
		}
		if (Fight.Ranged.Holds(Id))
		{
			Fight.Ranged.Release(Id, Time, Numbers.TokenRest);
			// A shot fired: now and then it moves on to a new spot, so it's never a fixed turret.
			if (FPlace* Place = Fight.Places.Find(Id))
			{
				Place->bRelocate = Rolls.FRand() < Numbers.RangedRelocate;
			}
		}
	}
}

void UFightDirectorSubsystem::ReleaseToken(const ACreatureBase& Creature)
{
	const uint32 Id = Creature.GetUniqueID();
	const double Time = Now();
	for (TPair<TWeakObjectPtr<const APawn>, FTargetFight>& Each : Fights)
	{
		Each.Value.Melee.Release(Id, Time, 0.f);
		Each.Value.Ranged.Release(Id, Time, 0.f);
	}
}

void UFightDirectorSubsystem::GiveBackForHit(const ACreatureBase& Creature)
{
	const uint32 Id = Creature.GetUniqueID();
	const double Time = Now();
	for (TPair<TWeakObjectPtr<const APawn>, FTargetFight>& Each : Fights)
	{
		if (Each.Value.Melee.ReleaseKeepTurn(Id, Time) || Each.Value.Ranged.ReleaseKeepTurn(Id, Time))
		{
			UE_LOG(LogLooter, Verbose, TEXT("%s is hit off its turn; it keeps its place in line."), *Creature.GetName());
		}
	}
}

void UFightDirectorSubsystem::ForgetCreature(const ACreatureBase& Creature)
{
	const uint32 Id = Creature.GetUniqueID();
	for (TPair<TWeakObjectPtr<const APawn>, FTargetFight>& Each : Fights)
	{
		Each.Value.Melee.Forget(Id);
		Each.Value.Ranged.Forget(Id);
		Each.Value.Places.Remove(Id);
	}
}

// ---------------------------------------------------------------------------
// The look at every fight
// ---------------------------------------------------------------------------

void UFightDirectorSubsystem::RefreshNow()
{
	Refresh(Now());
}

void UFightDirectorSubsystem::RefreshIfDue()
{
	const double Time = Now();
	if (Time - LastRefresh >= Tuning().RingRefresh)
	{
		Refresh(Time);
	}
}

void UFightDirectorSubsystem::Refresh(double Time)
{
	const float Elapsed = LastRefresh < -1.0e8 ? 0.f : static_cast<float>(FMath::Max(Time - LastRefresh, 0.0));
	LastRefresh = Time;
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Who fights whom: every creature that takes turns, alive, after someone, chasing or attacking. The encounters' list of
	// the level's creatures (as the packs read it); a level without one is walked.
	TMap<const APawn*, TArray<ACreatureBase*>> ByTarget;
	auto Consider = [&ByTarget](ACreatureBase* Creature)
	{
		if (Creature && !Creature->IsDead() && !Creature->IsActorBeingDestroyed() && Creature->GetTarget()
			&& ACreatureBase::IsFightState(Creature->GetCreatureState()) && Creature->UsesFightDirector())
		{
			ByTarget.FindOrAdd(Creature->GetTarget()).Add(Creature);
		}
	};
	if (const UEncounterSubsystem* Encounters = World->GetSubsystem<UEncounterSubsystem>())
	{
		for (const TWeakObjectPtr<ACreatureBase>& Each : Encounters->GetTrackedCreatures())
		{
			Consider(Each.Get());
		}
	}
	else
	{
		for (TActorIterator<ACreatureBase> It(World); It; ++It)
		{
			Consider(*It);
		}
	}

	for (const TPair<const APawn*, TArray<ACreatureBase*>>& Each : ByTarget)
	{
		Fights.FindOrAdd(Each.Key);
	}
	const TArray<ACreatureBase*> Nobody;
	for (auto It = Fights.CreateIterator(); It; ++It)
	{
		const APawn* Target = It.Key().Get();
		if (!Target)
		{
			It.RemoveCurrent();
			continue;
		}
		const TArray<ACreatureBase*>* Fighters = ByTarget.Find(Target);
		RefreshTarget(*Target, It.Value(), Fighters ? *Fighters : Nobody, Time, Elapsed);
		if (!Fighters && It.Value().Melee.IsEmpty() && It.Value().Ranged.IsEmpty())
		{
			It.RemoveCurrent();
		}
	}
}

void UFightDirectorSubsystem::RefreshTarget(const APawn& Target, FTargetFight& Fight, const TArray<ACreatureBase*>& Fighters, double Time,
	float Elapsed)
{
	const FFightTuning& Numbers = Tuning();
	TMap<uint32, ACreatureBase*> ById;
	ById.Reserve(Fighters.Num());
	for (ACreatureBase* Creature : Fighters)
	{
		ById.Add(Creature->GetUniqueID(), Creature);
	}
	KeepUpTurns(Target, Fight.Melee, true, ById, Time);
	KeepUpTurns(Target, Fight.Ranged, false, ById, Time);

	// Places of those no longer in this fight go.
	for (auto It = Fight.Places.CreateIterator(); It; ++It)
	{
		if (!ById.Contains(It.Key()))
		{
			It.RemoveCurrent();
		}
	}

	TArray<ACreatureBase*> Melee;
	TArray<ACreatureBase*> Ranged;
	for (ACreatureBase* Creature : Fighters)
	{
		const EFightRole Role = Creature->GetFightRole();
		// One breaking off runs: it holds no place in the ring.
		if (Role == EFightRole::Melee && !Creature->GetPack()->IsRetreating())
		{
			Melee.Add(Creature);
		}
		else if (Role == EFightRole::Ranged)
		{
			Ranged.Add(Creature);
		}
	}
	Fight.Slots = Numbers.RingSlots;
	Fight.RingTurn = FMath::UnwindDegrees(Fight.RingTurn + Numbers.RingDrift * Elapsed);
	PlaceRing(Target, Fight, Melee, Time);
	PlaceRanged(Target, Fight, Ranged, Melee);
}

void UFightDirectorSubsystem::KeepUpTurns(const APawn& Target, FFightTokens& Tokens, bool bMelee, const TMap<uint32, ACreatureBase*>& ById,
	double Time)
{
	const FFightTuning& Numbers = Tuning();
	const FVector Center = Target.GetActorLocation();
	const TArray<FFightTokens::FHolder> Holders = Tokens.GetHolders();
	for (const FFightTokens::FHolder& Holder : Holders)
	{
		ACreatureBase* const* Found = ById.Find(Holder.Id);
		FightRules::FHolderLook Look;
		Look.bFighting = Found != nullptr;
		Look.HeldSeconds = static_cast<float>(Time - Holder.Since);
		if (Found)
		{
			const ACreatureBase& Creature = **Found;
			Look.bHeldByHit = Creature.IsHeldByHit();
			Look.bAttacking = Creature.GetCreatureState() == ECreatureState::Attack;
			Look.Distance = static_cast<float>(FVector::Dist2D(Creature.GetActorLocation(), Center));
			Look.LeaveDistance = bMelee && Creature.GetFightRole() == EFightRole::Melee ? GetApproachDistance(Creature) * LeaveShare : 0.f;
		}
		switch (FightRules::CheckHolder(Look, Numbers.TokenMaxSeconds))
		{
		case FightRules::ETokenCheck::Return:
			Tokens.Release(Holder.Id, Time, 0.f);
			break;
		case FightRules::ETokenCheck::KeepTurn:
			Tokens.ReleaseKeepTurn(Holder.Id, Time);
			break;
		case FightRules::ETokenCheck::TakeBack:
			UE_LOG(LogLooter, Verbose, TEXT("%s held its turn %.1f s: taken back."), Found ? *(*Found)->GetName() : TEXT("A creature"), Look.HeldSeconds);
			Tokens.Release(Holder.Id, Time, Numbers.TokenRest);
			break;
		case FightRules::ETokenCheck::Keep:
			break;
		}
	}
	Tokens.Prune(Time, Numbers.TurnKeep);
}

// ---------------------------------------------------------------------------
// Tests and tools
// ---------------------------------------------------------------------------

int32 UFightDirectorSubsystem::CountTokens(const APawn& Target, EFightRole Role) const
{
	const FTargetFight* Fight = Fights.Find(&Target);
	if (!Fight)
	{
		return 0;
	}
	return Role == EFightRole::Ranged ? Fight->Ranged.NumHeld() : Fight->Melee.NumHeld();
}

bool UFightDirectorSubsystem::IsRingUp(const APawn& Target) const
{
	const FTargetFight* Fight = Fights.Find(&Target);
	return Fight && Fight->bRing;
}

int32 UFightDirectorSubsystem::GetSlot(const ACreatureBase& Creature, const APawn& Target) const
{
	const FTargetFight* Fight = Fights.Find(&Target);
	const FPlace* Place = Fight ? Fight->Places.Find(Creature.GetUniqueID()) : nullptr;
	return Fight && Fight->bRing && Place ? Place->Slot : INDEX_NONE;
}
