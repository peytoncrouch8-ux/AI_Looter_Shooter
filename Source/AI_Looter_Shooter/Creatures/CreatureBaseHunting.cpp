// ACreatureBase: whom it hunts. A living player character, standing on its hunting ground and out of every safe zone
// that's on. Each group fights on its own level of ground and won't follow off it (Docs/Areas/RansomsRest.md, "Encounters
// stay on one level of ground": it waits at the edge watching a while, CreatureBasePursuit.cpp), and Delia's salt line keeps
// the Unpaid off the farm.

#include "Creatures/CreatureBase.h"
#include "Combat/HealthComponent.h"
#include "Creatures/EncounterSubsystem.h"
#include "Scenes/SceneSubsystem.h"
#include "World/SwimWaterSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
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
