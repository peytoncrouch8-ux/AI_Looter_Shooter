#pragma once

#include "CoreMinimal.h"
#include "Weapons/VfxTypes.h"

struct FVfxTuning;

/**
 * The VFX pass's plain rules (Saved/Vfx/PLAN.md), apart from any world so the tests (Looter.Vfx.*) read them: how much of a
 * look is thrown by distance and in a stream of fire, how heavy a gun's hit and a kill look, what each tier adds to a
 * kill, each gun kind's muzzle, a blast's layers by its reach, and the curves the rings and fireballs grow and cool by.
 */
namespace VfxRules
{
	/** The share of a look's pieces thrown Distance cm from the camera: 1 within Near, easing down to Min at Far. */
	AI_LOOTER_SHOOTER_API float DetailAt(float Distance, const FVfxTuning& Tuning);

	/**
	 * The share a hit in a stream of fire throws, RecentHits (this one too) having landed in the stream's window and reach:
	 * the first two in full, then 1 / sqrt(hits - 1) down to a quarter (an LMG's tenth round into a wall costs a third).
	 */
	AI_LOOTER_SHOOTER_API float StreamDetail(int32 RecentHits);

	/** Full pieces at Amount (detail times scale): none at 0, else at least one. */
	AI_LOOTER_SHOOTER_API int32 Count(int32 Full, float Amount);

	/**
	 * How heavy a round's hit looks, from its gun's mark scale (FBulletMarks::GunScale: shotgun pellet 0.6, SMG 0.8, rifle
	 * and launcher 1, LMG 1.05, revolver 1.25, sniper 1.5): the pop's size and the chunks' share.
	 */
	AI_LOOTER_SHOOTER_API float HitWeight(float MarkScale);

	/** A heavy round (the revolver's, the sniper's): its hit adds a ring and a bigger exit spray. */
	AI_LOOTER_SHOOTER_API bool IsHeavyRound(float MarkScale);

	/**
	 * How much bigger a world hit's puffs and chunks are drawn Distance cm from the camera (VFX-2, 2026-10-22): 1 within
	 * 10 m, growing to 1.8 by 30 m and held there, so a round's dirt or stone dust still reads as a hit at 10-30 m instead of
	 * a few pale pixels (the pieces keep roughly their screen size; their counts are DetailAt's).
	 */
	AI_LOOTER_SHOOTER_API float ImpactViewScale(float Distance);

	/**
	 * How big a kill's layers are (1 a rifle's body kill): by the gun (a shotgun's or a rocket's throws more, an SMG's a
	 * little less), a crit (more), and how far past dead the killing hit went (Share over 1 adds up to 0.3).
	 */
	AI_LOOTER_SHOOTER_API float KillWeight(EWeaponKind Gun, bool bCritical, float Share);

	/** How much a kill's gore is thrown along the killing shot (0 every way, 1 all with it): a shotgun's and a rocket's most. */
	AI_LOOTER_SHOOTER_API float KillCarry(EWeaponKind Gun);

	/** What a tier adds to a kill (none for a Regular; Restless a ring; Gravebound two and a star; the top two small under their show). */
	AI_LOOTER_SHOOTER_API FVfxTierLayers TierLayers(ECreatureRank Rank);

	/** A gun kind's muzzle (SpawnMuzzleBurst's layers, and the flash GUNS-CODE draws: the proposal). */
	AI_LOOTER_SHOOTER_API FVfxMuzzleLook MuzzleLook(EWeaponKind Kind);

	/**
	 * A gun kind's tracer for the round RoundsLeft in its magazine: an SMG's thin short needles, a sniper's thick bright line,
	 * a revolver's a little wider, a pellet's short; an LMG's every third round a full tracer and the rest half as bright (the
	 * belt's read). Times the definition's width, length and colour.
	 */
	AI_LOOTER_SHOOTER_API FVfxTracerLook TracerLook(EWeaponKind Kind, int32 RoundsLeft);

	/** A blast's layers for its reach (cm): more fireballs and smoke for a bigger one, never past the pool's share. */
	AI_LOOTER_SHOOTER_API FVfxExplosionLook ExplosionLook(float Radius);

	/** A shock ring's radius along its life (0 at T 0, 1 at T 1): fast out, easing (1 - (1 - T)^3). */
	AI_LOOTER_SHOOTER_API float RingGrowth(float T);

	/** A shock ring's strength along its life: full at once, gone by the end ((1 - T)^2). */
	AI_LOOTER_SHOOTER_API float RingFade(float T);

	/** A fireball's heat along its life: white-hot at first, cooling fast then slowly ((1 - T)^0.7). */
	AI_LOOTER_SHOOTER_API float FireballHeat(float T);

	/** A fireball's size along its life (0 at Start, 1 at End): blooming fast (sqrt(T)). */
	AI_LOOTER_SHOOTER_API float FireballGrowth(float T);

	/** The heat colour of a fireball at Heat (1 white-hot, 0.5 Color, 0 smouldering dark red), for the stand-in without M_FX_Shape. */
	AI_LOOTER_SHOOTER_API FLinearColor HeatColor(const FLinearColor& Color, float Heat);

	/** A ring drawn in streaks (no M_FX_Shape): this many pieces for a radius (cm), 10 to 28. */
	AI_LOOTER_SHOOTER_API int32 RingPieces(float Radius);
}
