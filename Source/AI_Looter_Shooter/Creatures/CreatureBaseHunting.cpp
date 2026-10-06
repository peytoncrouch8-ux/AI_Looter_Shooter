// ACreatureBase: whom it hunts. A living player character, standing on its hunting ground and out of every safe zone
// that's on. Each group fights on its own level of ground and gives up the chase at its edge (Docs/Areas/RansomsRest.md,
// "Encounters stay on one level of ground"), and Delia's salt line keeps the Unpaid off the farm.

#include "Creatures/CreatureBase.h"
#include "Combat/HealthComponent.h"
#include "Creatures/EncounterSubsystem.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"

bool ACreatureBase::IsValidTarget(const APawn* Pawn) const
{
	// Only living characters (not spectator cameras, not other creatures).
	if (!Pawn || !Pawn->IsA<ACharacter>() || Pawn->IsA<ACreatureBase>())
	{
		return false;
	}
	const UHealthComponent* TargetHealth = Pawn->FindComponentByClass<UHealthComponent>();
	if (!TargetHealth || TargetHealth->IsDead())
	{
		return false;
	}
	// Off its ground the chase is over, so a fight never follows a player down a ramp or out over a fence; and nothing hunts
	// a player in a safe zone. Either way it lets go and walks home (UpdatePerception), and no hit or pack call sets it on
	// them until they come back.
	const FVector Where = Pawn->GetActorLocation();
	if (!HuntingGround.Contains(Where, Home.GetLocation()))
	{
		return false;
	}
	return !UEncounterSubsystem::IsSheltered(this, Where);
}
