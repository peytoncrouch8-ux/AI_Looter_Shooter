#pragma once

#include "CoreMinimal.h"

/**
 * A brown spider's cunning in a fight (the user, 2026-10-31: their AI is "lackluster"; ASpiderCreature plays it in
 * SpiderCreatureHunt.cpp), as plain rules the tests check without a world (Looter.Creatures.Spider.Hunt.*):
 * - it circles before it lunges: given its turn, it often sidles round the player a moment, a few metres out, facing
 *   them, working toward their side or back, then rushes in and bites;
 * - it backs off when hurt: a burst of damage sends it skittering back, still facing the player, before it comes again;
 *   now and then after a bite it skips back out of reach (hit and run);
 * - spiders don't all bite at once: one given its turn while a packmate's bite at the same player began less than
 *   BiteSpacing ago circles first (on top of the director's three turns at once; its family gap stays none: the director's
 *   own rules and tests are untouched).
 * Its health and bite are untouched (the user's numbers since 2026-10-22: 230, 11). Cvars Looter.Spider.Cunning.*.
 */
struct AI_LOOTER_SHOOTER_API FSpiderHuntTuning
{
	/** Looter.Spider.Cunning: 1 the brown spider circles, backs off and spaces its bites; 0 it fights as before. */
	int32 Enabled = 1;
	/** Given its turn within this (s) of a packmate's bite starting at the same player, it circles first, at least this long. */
	float BiteSpacing = 0.6f;
	/** On a fresh turn it circles first with this chance, for CircleSeconds (s, by its dice), round at CircleRadius (cm at size 1). */
	float CircleChance = 0.65f;
	float CircleSeconds[2] = { 0.55f, 1.15f };
	float CircleRadius = 330.f;
	/** It circles at this share of its chase speed, and pulls in or out to its radius at this rate (1/s). */
	float CircleSpeedShare = 0.6f;
	float CircleRadialGain = 1.5f;
	/** It circles only from within this many of its radii (farther, it closes in first). */
	float CircleFrom = 2.2f;
	/** Hurt by this share of its health within about BackOffWindow (s), it backs off with BackOffChance. */
	float BackOffShare = 0.12f;
	float BackOffWindow = 1.2f;
	float BackOffChance = 0.75f;
	/** Its skitter back: this long (s), this fast (cm/s at size 1), and not again for BackOffCooldown (s). */
	float BackOffSeconds = 0.55f;
	float BackOffSpeed = 520.f;
	float BackOffCooldown = 4.f;
	/** After a bite it skips back out of reach with this chance, for this long (s). */
	float HitAndRunChance = 0.35f;
	float HitAndRunSeconds = 0.35f;

	/** The live numbers (the Looter.Spider.Cunning.* cvars). */
	static FSpiderHuntTuning Live();
};

/** A spider's cunning as it goes (ASpiderCreature keeps one). */
struct FSpiderHuntState
{
	/** Its turn's plan: made (this turn), its circle's time left and its way round (SpiderHuntRules::CircleSide). */
	bool bTurnPlanned = false;
	float CircleLeft = 0.f;
	float CircleSide = 1.f;
	/** When it last held a turn (world seconds): a gap means a fresh turn. */
	double LastTurnTime = -100.0;
	/** Circling into something: how long it has been held up, and whether it has turned the other way round already. */
	float Blocked = 0.f;
	bool bFlipped = false;
	/** Its health share taken lately (fading over BackOffWindow), its skitter's time left, since its last, and which way. */
	float RecentHurt = 0.f;
	float SkitterLeft = 0.f;
	float SinceSkitter = 100.f;
	float SkitterSpeed = 0.f;
};

namespace SpiderHuntRules
{
	/** On a fresh turn Distance (cm, flat) from its target: circle first? Always when a packmate's bite just began (bCrowded). */
	AI_LOOTER_SHOOTER_API bool WantsCircle(float Distance, float Scale, float Dice01, bool bCrowded, const FSpiderHuntTuning& Tuning);
	/** Its circle's length for a roll (crowded: at least BiteSpacing). */
	AI_LOOTER_SHOOTER_API float CircleSeconds(float Dice01, bool bCrowded, const FSpiderHuntTuning& Tuning);
	/**
	 * Which way round it circles (+1: the way that turns +X toward +Y round them): on toward the player's back, by where it
	 * stands against their facing; straight in front of them, by Dice01.
	 */
	AI_LOOTER_SHOOTER_API float CircleSide(const FVector& Here, const FVector& Prey, const FVector& PreyFacing, float Dice01);
	/** Its way round (flat, unit): along the circle by Side, pulled in or out toward Radius. */
	AI_LOOTER_SHOOTER_API FVector CircleWay(const FVector& Here, const FVector& Prey, float Side, float Radius, float RadialGain);
	/** Its recent hurt after Seconds more, and with a hit's Share on top. */
	AI_LOOTER_SHOOTER_API float FadeHurt(float RecentHurt, float Seconds, const FSpiderHuntTuning& Tuning);
	/** Hurt that much lately, its last skitter SinceSkitter (s) ago: back off now? (Dice01: its roll.) */
	AI_LOOTER_SHOOTER_API bool WantsBackOff(float RecentHurt, float SinceSkitter, float Dice01, const FSpiderHuntTuning& Tuning);
	/** After a bite: skip back? */
	AI_LOOTER_SHOOTER_API bool WantsHitAndRun(float SinceSkitter, float Dice01, const FSpiderHuntTuning& Tuning);
	/** Its way back from Prey (flat, unit), a little to one side (Side) so it doesn't back into the same line. */
	AI_LOOTER_SHOOTER_API FVector BackOffWay(const FVector& Here, const FVector& Prey, float Side);
}
