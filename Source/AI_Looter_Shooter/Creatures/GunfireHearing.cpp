#include "Creatures/GunfireHearing.h"

bool GunfireHearing::MayHear(const FGunfireListener& Listener, float Radius)
{
	// Only a calm one: idle or strolling, after nobody. One already hunting knows where the player is; one walking home has
	// just given someone up (too far, out of its reach) and a shot shouldn't turn it straight back.
	const bool bCalm = (Listener.State == ECreatureState::Idle || Listener.State == ECreatureState::Wander) && !Listener.bHasTarget;
	// Held back, or held in its entrance (buried, perched, climbing out): that has its own pacing and its own senses.
	const bool bFree = !Listener.bPassive && !Listener.bBrainHeld && !Listener.bHidden;
	return Radius > 0.f && bCalm && bFree && Listener.Distance <= Radius;
}

bool GunfireHearing::Hears(const FGunfireListener& Listener, float Radius)
{
	// A boss's fight starts its own way; nothing but a boss notices anyone in a scene; and it hunts only one it may hunt, so a
	// shot from a safe zone or off its ground (a ridge over its yard) leaves it be, as a sight of them would.
	return MayHear(Listener, Radius) && !Listener.bBoss && !Listener.bSceneHolding && Listener.bShooterHuntable && !Listener.bSpurned;
}
