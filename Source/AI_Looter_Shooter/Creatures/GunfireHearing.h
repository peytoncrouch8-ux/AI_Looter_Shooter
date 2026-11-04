#pragma once

#include "CoreMinimal.h"
#include "Creatures/CreatureTypes.h"

/**
 * Hearing a player's gunfire (the audit's change 4, 2026-11-03): a player's shot carries Looter.Fight.HearGunfire (30 m), and a
 * calm creature that hears it turns and hunts the shooter as if it had seen them. Before, only its eyes (one trace a fifth of a
 * second) and a hurt packmate's call set a creature on the player, so a fight never spilled over to the pack next door.
 *
 * The rule is plain data so the tests check it without a level (Looter.Creatures.Pursuit.Hearing). ACreatureBase::HearGunfire
 * (CreatureBaseHunting.cpp) fills a listener per creature and acts on it; UBulletSubsystem calls it once a shot a player fires
 * (a shotgun's pellets are one shot). MayHear is the cheap half, asked of every creature at every shot; Hears adds the dearer
 * looks (a boss, a scene, its ground and the safe zones, a spurn), asked only of the few that pass it.
 */
struct FGunfireListener
{
	/** What its brain is doing. Only Idle and Wander hear: a hunt has its own senses, Return has just given someone up. */
	ECreatureState State = ECreatureState::Idle;
	/** After someone already. */
	bool bHasTarget = false;
	/** Held back (ACreatureBase::SetPassive: a boss under a spell, a spider waiting in its den). */
	bool bPassive = false;
	/** Its brain held (HoldBrain): buried for a rise, on a lurk's perch, climbing out. Its entrance keeps its own pacing. */
	bool bBrainHeld = false;
	/** Hidden in the game: not in the world for the player (a body sunk away, one held for its area to bring back). */
	bool bHidden = false;
	/** From the shot to it (cm). */
	float Distance = 0.f;

	// The dearer looks (Hears only).
	/** A boss, or a creature a boss fight is built round: its fight begins its own way (its arena, its intro). */
	bool bBoss = false;
	/** A scene plays or holds the player: nothing but a boss notices anyone then. */
	bool bSceneHolding = false;
	/** The shooter is one it may hunt: alive, on its hunting ground, out of every safe zone (ACreatureBase::CheckHunt). */
	bool bShooterHuntable = false;
	/** It gave the shooter up as out of its reach and they haven't moved since (FCreaturePursuit::IsSpurned). */
	bool bSpurned = false;
};

namespace GunfireHearing
{
	/** The cheap half: within Radius (0 or less: nobody hears), idle or wandering, after nobody, free, its brain its own, in the world. */
	AI_LOOTER_SHOOTER_API bool MayHear(const FGunfireListener& Listener, float Radius);

	/** Whether it hears the shot and hunts the shooter: MayHear, not a boss, no scene, the shooter one it may hunt and not spurned. */
	AI_LOOTER_SHOOTER_API bool Hears(const FGunfireListener& Listener, float Radius);
}
