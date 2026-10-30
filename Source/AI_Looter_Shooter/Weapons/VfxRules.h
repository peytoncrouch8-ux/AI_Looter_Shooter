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

	// --- A world hit's weight by its gun's kind (the gun feel pass, GF-IMPACT 2026-10-22) ---

	/**
	 * How heavy a gun kind's round lands on the world (Looter.Vfx.Impact.Weight.<Kind>): SMG 0.6, rifle 1, LMG 1.05, revolver
	 * 1.3, sniper 1.8, a shotgun pellet 0.5 (its share of its blast); a rocket's or no gun's 1. The one-argument form reads the
	 * cvars as set now.
	 */
	AI_LOOTER_SHOOTER_API float ImpactWeight(EWeaponKind Kind, const FVfxTuning& Tuning);
	AI_LOOTER_SHOOTER_API float ImpactWeight(EWeaponKind Kind);

	/**
	 * How a round of Kind lands on Material, RoundsLeft in its magazine as it was fired: its pieces' count and size by its
	 * weight; a pellet's count only (it keeps its mark's size; its blast's sheet carries the blast); an LMG's full round
	 * every Impact.LMGFullEvery with a chunk burst, the rest Impact.LMGLight of it; a chunk burst from Impact.ChunkFrom on a
	 * hard surface; a punch from Impact.PunchFrom. Kind None: today's look (all 1, nothing added).
	 */
	AI_LOOTER_SHOOTER_API FVfxImpactLook ImpactLook(EWeaponKind Kind, int32 RoundsLeft, EImpactMaterial Material, const FVfxTuning& Tuning);

	/** A surface a heavy round chips and splits (stone, gravel, wood, metal, sheet metal, glass, bone), not one it sinks into. */
	AI_LOOTER_SHOOTER_API bool IsHardSurface(EImpactMaterial Material);

	/** What a blast's dust sheet adds there: sparks off what sparks (gravel, stone, metal), spray off water, a fan off the rest. */
	AI_LOOTER_SHOOTER_API EVfxSheet SheetOf(EImpactMaterial Material);

	/**
	 * What a known gun's hit on a surface nothing names is drawn as (the play-check, 2026-10-23: a sniper round into unmapped
	 * ground showed the old puff alone): ground facing up (Normal.Z from 0.6) as dirt, anything else as stone. A named surface
	 * stays itself.
	 */
	AI_LOOTER_SHOOTER_API EImpactMaterial GuessSurface(EImpactMaterial Material, const FVector& Normal);

	// --- A creature hit's glows (the play-check, 2026-10-23: never a white disc over the target) ---

	/** The share of its light a glow at one wound gives when RecentHits (this one too) landed there together: 1 / n. */
	AI_LOOTER_SHOOTER_API float GlowShare(int32 RecentHits);

	/** A glow's size (cm across) Distance cm from the eye, never wider on screen than MaxRadians (0: as given). */
	AI_LOOTER_SHOOTER_API float HitFlashSizeAt(float Size, float Distance, float MaxRadians);

	/** A blast's sheet's radius (cm) for its pellets' Spread (their root-mean-square distance from their middle): 40 to 220. */
	AI_LOOTER_SHOOTER_API float SheetRadius(float Spread);

	/** The sheet's low puffs for its radius: 3 to 5 (wider patterns, more). */
	AI_LOOTER_SHOOTER_API int32 SheetPuffs(float Radius);

	/**
	 * How big a kill's layers are (1 a rifle's body kill): by the gun (a shotgun's or a rocket's throws more, an SMG's a
	 * little less), a crit (more), and how far past dead the killing hit went (Share over 1 adds up to 0.6; capped at 2.4).
	 */
	AI_LOOTER_SHOOTER_API float KillWeight(EWeaponKind Gun, bool bCritical, float Share);

	/** How much a kill's gore is thrown along the killing shot (0 every way, 1 all with it): a shotgun's and a rocket's most. */
	AI_LOOTER_SHOOTER_API float KillCarry(EWeaponKind Gun);

	/** What a tier adds to a kill (none for a Regular; Restless a ring; Gravebound two and a star; the top two small under their show). */
	AI_LOOTER_SHOOTER_API FVfxTierLayers TierLayers(ECreatureRank Rank);

	// --- The muzzle (VfxRulesMuzzle.cpp; the flash reworked 2026-10-22) ---

	/** A gun kind's muzzle: SpawnMuzzleBurst's layers and the flash's (shotgun big, SMG small and quick, sniper sharp, launcher a puff). */
	AI_LOOTER_SHOOTER_API FVfxMuzzleLook MuzzleLook(EWeaponKind Kind);

	/**
	 * A look's flash quads at scale 1 (appended to Out): a white-hot core and a soft orange bloom facing back down the barrel,
	 * a star of thin spikes, tongues along the barrel for side views, a brake's jets, a cylinder's spit.
	 */
	AI_LOOTER_SHOOTER_API void MuzzleQuads(const FVfxMuzzleLook& Look, TArray<FVfxFlashQuad>& Out);

	/**
	 * Each layer's size for a view: from behind as built; in the player's own view the star, bloom and jets smaller and the
	 * tongues short (along the barrel is toward the crosshair), and all of it smaller and dimmer aimed down the sights
	 * (AimAlpha 0 hip .. 1 aimed).
	 */
	AI_LOOTER_SHOOTER_API FVfxMuzzleView MuzzleView(bool bFirstPerson, float AimAlpha, const FVfxTuning& Tuning);

	/**
	 * A flash quad's colour leaning toward its gun's element (Amount 0 none .. 1 the layer's whole share; Looter.Vfx.Element.MuzzleTint):
	 * the core stays near white-hot (a fifth), the star about half, the bloom, tongues and jets most (2026-10-29, "Elements matter").
	 */
	AI_LOOTER_SHOOTER_API FLinearColor MuzzleElementTint(EVfxFlashLayer Layer, const FLinearColor& Base, const FLinearColor& Element, float Amount);

	/** A layer's size in a view (its FVfxMuzzleView field). */
	AI_LOOTER_SHOOTER_API float MuzzleLayerScale(EVfxFlashLayer Layer, const FVfxMuzzleView& View);

	/**
	 * A quad's place on the flash at its layer's scale and the gun's flash scale (the Plane is 100 cm): it grows from the
	 * muzzle; a tongue's or jet's width only by the root of its scale (a short tongue stays a lick of flame, not a sliver).
	 */
	AI_LOOTER_SHOOTER_API FTransform MuzzleQuadTransform(const FVfxFlashQuad& Quad, float LayerScale, float GunScale);

	/**
	 * How far the flash reaches from the muzzle (cm at scale 1 in that view): out along the barrel (the tongues' tips) or
	 * across it (the star's, bloom's and jets' edges).
	 */
	AI_LOOTER_SHOOTER_API float MuzzleReach(const TArray<FVfxFlashQuad>& Quads, const FVfxMuzzleView& View, bool bAlongBarrel);

	/** The flash's strength Age s into its Duration: full at once, falling as (1 - share) ^ Power, nothing at the end. */
	AI_LOOTER_SHOOTER_API float MuzzleFade(float Age, float Duration, float Power);

	/**
	 * The share of its size a first-person flash keeps so it stays clear of the crosshair: the muzzle MuzzleDegrees off the
	 * view's middle, the flash's nearest edge NearestDegrees off it at full size; 1 when it already keeps ClearDegrees away,
	 * else shrunk to keep out (at the least MinScale).
	 */
	AI_LOOTER_SHOOTER_API float MuzzleClearScale(float MuzzleDegrees, float NearestDegrees, float ClearDegrees, float MinScale);

	/** ClearDegrees round the view's middle as a share of the screen's width (M_FX_Muzzle's hole), for a horizontal field of view. */
	AI_LOOTER_SHOOTER_API float MuzzleClearShare(float ClearDegrees, float HorizontalFovDegrees);

	/** The share of a streak's length one shot shows (Random 0..1): each flash's tongues a little different, never under 0.55. */
	AI_LOOTER_SHOOTER_API float MuzzleStreakShare(float Random);

	/**
	 * The share of its size a first-person flash keeps so it never looks bigger than MaxDegrees from its muzzle to its
	 * farthest edge (a revolver drawn big and close flashed over half the screen; the play-check, 2026-10-22).
	 */
	AI_LOOTER_SHOOTER_API float MuzzleSizeCap(float FlashDegrees, float MaxDegrees);

	/**
	 * A soft streak's width or length (a spark's, a tracer's: FWeaponFX's SoftStreaks) Distance cm from the eye: never bigger on
	 * screen than MaxRadians (0: as given), so a spark or tracer head right by a close target is a thin line, not a card over it.
	 */
	AI_LOOTER_SHOOTER_API float StreakWidthAt(float Width, float Distance, float MaxRadians);

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

	/**
	 * A sprite's roll Age s into its life (a puff, a fireball, a flame): turning slowly as it billows, easing off, its way and
	 * pace (0.25-0.7 rad/s) from Roll, its spawn's random roll, so a cloud of them isn't a still stack of cards. Its noise seed
	 * stays the spawn's roll (the pattern turns with it, never jumps).
	 */
	AI_LOOTER_SHOOTER_API float SpriteRoll(float Roll, float Age);

	/**
	 * How many times its size a glow Size cm across moving Speed cm/s across the view is drawn along its way (an ember, a mote,
	 * a glint: the smear a camera's shutter, GlowShutter s, shows), 1 still, at most GlowMostStretch.
	 */
	AI_LOOTER_SHOOTER_API float GlowStretch(float Speed, float Size);
	inline constexpr float GlowShutter = 0.012f;
	inline constexpr float GlowMostStretch = 3.f;
}
