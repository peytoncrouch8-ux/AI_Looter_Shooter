// ACreatureBase: a hook for developer tools and tests (Looter.CastShots photographs each creature in each of its states,
// and needs them on cue rather than when its brain gets round to them).

#include "Creatures/CreatureBase.h"
#include "AI_Looter_Shooter.h"

void ACreatureBase::DevPutInState(ECreatureState NewState, APawn* InTarget, const FVector& Goal)
{
	if (State == ECreatureState::Dead || NewState == ECreatureState::Dead)
	{
		return;
	}
	Target = InTarget;
	WanderGoal = Goal;
	CooldownRemaining = 0.f;
	SetState(NewState);
	if (NewState == ECreatureState::Idle)
	{
		// Idle until something else moves it (its brain would pick a stroll after a few seconds).
		IdleDuration = TNumericLimits<float>::Max();
	}
	UE_LOG(LogLooter, Verbose, TEXT("%s: put in %s for a developer tool."), *GetName(), *UEnum::GetValueAsString(NewState));
}
