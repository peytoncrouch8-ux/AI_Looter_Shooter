// ACreatureBase: whom it hunts. A living player character, standing on its hunting ground and out of every safe zone
// that's on. Each group fights on its own level of ground and won't follow off it (Docs/Areas/RansomsRest.md, "Encounters
// stay on one level of ground": it waits at the edge watching a while, CreatureBasePursuit.cpp), and Delia's salt line keeps
// the Unpaid off the farm. And what it hears: a player's gunfire near it (HearGunfire, 2026-11-03).

#include "Creatures/CreatureBase.h"
#include "AI_Looter_Shooter.h"
#include "Combat/HealthComponent.h"
#include "Creatures/EncounterSubsystem.h"
#include "Creatures/FightTuning.h"
#include "Creatures/GunfireHearing.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "Scenes/SceneSubsystem.h"
#include "World/SwimWaterSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

bool ACreatureBase::IsValidTarget(const APawn* Pawn) const
{
	return CheckHunt(Pawn) == EHuntCheck::Huntable;
}

ACreatureBase::EHuntCheck ACreatureBase::CheckHunt(const APawn* Pawn) const
{
	// Only living characters (not spectator cameras, not other creatures).
	if (!Pawn || !Pawn->IsA<ACharacter>() || Pawn->IsA<ACreatureBase>())
	{
		return EHuntCheck::NotPrey;
	}
	const UHealthComponent* TargetHealth = Pawn->FindComponentByClass<UHealthComponent>();
	if (!TargetHealth || TargetHealth->IsDead())
	{
		return EHuntCheck::NotPrey;
	}
	// Nothing hunts a player in a safe zone: it lets go and walks home, and no hit or pack call sets it on them there.
	const FVector Where = Pawn->GetActorLocation();
	if (UEncounterSubsystem::IsSheltered(this, Where))
	{
		return EHuntCheck::Sheltered;
	}
	// In deep water (a swimmer) is off every creature's ground: it's watched from the shore (World/SwimWaterSubsystem).
	if (USwimWaterSubsystem::IsDeepAt(this, Where))
	{
		return EHuntCheck::OffGround;
	}
	// Off its ground the chase can't follow, so a fight never runs down a ramp or out over a fence (unless pressed: a little
	// way past the edge, at the same height).
	return IsOnItsGround(Where) ? EHuntCheck::Huntable : EHuntCheck::OffGround;
}

bool ACreatureBase::IsOnItsGround(const FVector& Where) const
{
	return HuntingGround.Contains(Where, Home.GetLocation(), GetGroundReach());
}

float ACreatureBase::GetGroundReach() const
{
	return PressedLeft > 0.f ? GetPressReach() : 0.f;
}

bool ACreatureBase::NotePressed(const APawn* By)
{
	// Only a player just past its edge, at its ground's own height (the band holds: never down a ramp), and never a boss.
	const float Reach = GetPressReach();
	if (Reach <= 0.f || !By || !HuntingGround.IsSet() || KeepsBossRules() || UEncounterSubsystem::IsSheltered(this, By->GetActorLocation())
		|| !HuntingGround.Contains(By->GetActorLocation(), Home.GetLocation(), Reach))
	{
		return false;
	}
	PressedLeft = FMath::Max(PressedLeft, GetPressSeconds());
	return true;
}

/** A scene plays or holds the player: nothing but a boss hunts them meanwhile. */
bool ACreatureBase::IsSceneHoldingPlayer() const
{
	// Asked five times a second by every creature looking about: straight to the level, no lookups.
	const UWorld* World = GetWorld();
	const USceneSubsystem* Scenes = World ? World->GetSubsystem<USceneSubsystem>() : nullptr;
	return Scenes && (Scenes->IsPlaying() || Scenes->IsHoldingPlayer());
}

// ---------------------------------------------------------------------------
// Hearing a player's gunfire (the audit's change 4, 2026-11-03; the rule: GunfireHearing)
// ---------------------------------------------------------------------------

void ACreatureBase::HearGunfire(APawn* Shooter, const FVector& Where)
{
	const float Radius = FFightTuning::LiveHearGunfire();
	UWorld* World = Shooter && Radius > 0.f ? Shooter->GetWorld() : nullptr;
	const UEncounterSubsystem* Encounters = World ? UEncounterSubsystem::Get(World) : nullptr;
	// Only the player's shots carry (their pawn, or a test level's stand-in for it): a Dredger's rifle or Delia's pistol is part
	// of a fight already. Every played world keeps its creatures in the encounters' list; without one nobody listens, so no shot
	// ever walks the level's actors.
	if (!Encounters || !(Shooter->IsPlayerControlled() || Encounters->GetPlayer() == Shooter))
	{
		return;
	}
	const double Now = World->GetTimeSeconds();
	const FVector ShooterAt = Shooter->GetActorLocation();
	const float ResumeShift = FCreaturePursuit::FRules().ResumeShift;
	// Who hears it first, then they turn: turning one plays its notice (its kind's cry), which mustn't run while the list it
	// came from is walked. The cheap half of the rule for every creature; its dearer looks only for the few near and calm.
	TArray<TWeakObjectPtr<ACreatureBase>, TInlineAllocator<16>> Hearers;
	for (const TWeakObjectPtr<ACreatureBase>& Each : Encounters->GetTrackedCreatures())
	{
		ACreatureBase* Creature = Each.Get();
		if (!Creature || Creature->IsActorBeingDestroyed())
		{
			continue;
		}
		FGunfireListener Listener;
		Listener.State = Creature->State;
		Listener.bHasTarget = Creature->Target.IsValid();
		Listener.bPassive = Creature->bPassive;
		Listener.bBrainHeld = Creature->BrainHold > 0.f;
		Listener.bHidden = Creature->IsHidden();
		Listener.Distance = static_cast<float>(FVector::Dist(Creature->GetActorLocation(), Where));
		if (!GunfireHearing::MayHear(Listener, Radius))
		{
			continue;
		}
		Listener.bBoss = Creature->KeepsBossRules();
		Listener.bSceneHolding = Creature->IsSceneHoldingPlayer();
		Listener.bShooterHuntable = Creature->CheckHunt(Shooter) == EHuntCheck::Huntable;
		Listener.bSpurned = Creature->Pursuit.IsSpurned(Shooter->GetUniqueID(), ShooterAt, Now, ResumeShift * Creature->SizeScale);
		if (GunfireHearing::Hears(Listener, Radius))
		{
			Hearers.Add(Creature);
		}
	}
	for (const TWeakObjectPtr<ACreatureBase>& Each : Hearers)
	{
		ACreatureBase* Creature = Each.Get();
		if (!Creature || Creature->IsDead())
		{
			continue;
		}
		// As if its own eyes had found them (UpdatePerception): the hunt, the beat that shows it before it comes, its page.
		const ECreatureState Before = Creature->State;
		Creature->BeginHunt(Shooter);
		Creature->BeginNotice(Before);
		UPlayerProgressionSubsystem::RecordEncounter(Shooter->GetController(), Creature);
		UE_LOG(LogLooter, Verbose, TEXT("%s heard %s's shot %.0f m off: it hunts them."), *Creature->GetName(), *Shooter->GetName(),
			FVector::Dist(Creature->GetActorLocation(), Where) / 100.0);
	}
}
