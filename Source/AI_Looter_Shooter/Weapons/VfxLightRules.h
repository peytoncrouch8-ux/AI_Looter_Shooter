#pragma once

#include "CoreMinimal.h"
#include "Weapons/ImpactSurface.h"
#include "Weapons/WeaponTypes.h"

struct FVfxTuning;

/**
 * The gun lights the hands and the ground (Phase 3, lane MUZZLE, 2026-11-03; the core-loop audit's change 8). The audit found
 * the muzzle light 24 cd / 3.2 m (invisible in daylight) and no light on a hit at all, so the flash read as a sticker. Two
 * lights, both unshadowed (a shadowed light costs a shadow pass; an unshadowed one is nearly free): the shot's own, riding the
 * muzzle flash (AWeaponBase, WeaponBaseMuzzle.cpp), and a flash on hard surfaces a round hits (UImpactFlashSubsystem). This
 * file is their plain rules and the pool's bookkeeping, apart from any world so the tests (Looter.Vfx.Light.*) read them
 * with the cvars' defaults (a default-made FVfxTuning). Numbers: Looter.Vfx.Muzzle.Light* and Looter.Vfx.Impact.Light*.
 */

/** A point light's strength at its peak (candelas) and reach (cm); no candelas, no light. */
struct FVfxLightShape
{
	float Candelas = 0.f;
	float RadiusCm = 0.f;
	FLinearColor Color = FLinearColor::White;
};

namespace VfxRules
{
	// --- The shot's light ---

	/** A gun kind's share of the shot light (Looter.Vfx.Muzzle.Light.<Kind>); no gun is a rifle's. */
	AI_LOOTER_SHOOTER_API float MuzzleLightKindShare(EWeaponKind Kind, const FVfxTuning& Tuning);

	/**
	 * The shot light at its peak for a kind: Looter.Vfx.Muzzle.Light candelas times the kind's share (and the first-person
	 * share in the player's own view), reaching Looter.Vfx.Muzzle.LightRadius times the root of the share. Candelas 0 when the
	 * light, the kind's share or the radius is 0.
	 */
	AI_LOOTER_SHOOTER_API FVfxLightShape MuzzleLightShape(EWeaponKind Kind, bool bFirstPerson, const FVfxTuning& Tuning);

	/**
	 * How long the shot light shows: Looter.Vfx.Muzzle.LightSeconds, never longer than its flash (FlashSeconds): the gun ticks only
	 * while the flash shows, so a light outliving it would hang lit. A short flash (the SMG's 30 ms) cuts it short.
	 */
	AI_LOOTER_SHOOTER_API float MuzzleLightLife(float FlashSeconds, const FVfxTuning& Tuning);

	/**
	 * Where the shot light hangs: where the muzzle shows, PushCm out along the barrel, so it never sits inside the gun's mesh
	 * (a light inside a mesh lights nothing but the mesh's inside). A barrel with no direction leaves it at the muzzle.
	 */
	AI_LOOTER_SHOOTER_API FVector MuzzleLightPlace(const FVector& Muzzle, const FVector& Barrel, float PushCm);

	// --- The flash on the rock ---

	/** How much of the impact light a surface takes: stone and metal 1, sheet metal 0.8, wood 0.55; soft ground, water, cloth, hay, glass, bone, gravel and the unknown 0. */
	AI_LOOTER_SHOOTER_API float ImpactLightSurface(EImpactMaterial Material);

	/** The flash's colour on a surface: hot gold off stone and metal sparks, deeper orange on wood. */
	AI_LOOTER_SHOOTER_API FLinearColor ImpactLightColor(EImpactMaterial Material);

	/**
	 * How much of its light a flash EyeDistance cm from the eye keeps: 0 at Looter.Vfx.Impact.LightEye (1 m) and nearer ("never nearer
	 * than 1 m": a wall hit at arm's length flashes the whole screen), easing to 1 at twice it. A negative distance (no eye known yet)
	 * and an eye rule of 0 keep it all.
	 */
	AI_LOOTER_SHOOTER_API float ImpactLightEyeShare(float EyeDistance, const FVfxTuning& Tuning);

	/**
	 * The flash a round throws on Material: the impact light times the surface's share, the round's heft (Weight, the hit look's weight:
	 * a rifle round's 1.1 is 1, a sniper's and a revolver's more up to 1.5, an SMG's and a pellet's less down to 0.5) and the eye's share
	 * (EyeDistance to where the light hangs). Nothing for a soft surface, a hit past Looter.Vfx.Impact.LightFar, or the light off.
	 */
	AI_LOOTER_SHOOTER_API FVfxLightShape ImpactLightShape(EImpactMaterial Material, float Weight, float EyeDistance, const FVfxTuning& Tuning);

	/** The weight (FWeaponFX's hit weight) that counts as a full rifle round, and the least and most the heft goes to. */
	inline constexpr float ImpactLightWeightRef = 1.1f;
	inline constexpr float ImpactLightHeftLeast = 0.5f;
	inline constexpr float ImpactLightHeftMost = 1.5f;
	/** A flash fades as (1 - age share) ^ this: full at once, gone fast (the muzzle flash's own curve). */
	inline constexpr float ImpactLightFadePower = 2.f;
}

/** One impact flash light's bookkeeping. */
struct FImpactFlashSlot
{
	bool bLive = false;
	FVector Place = FVector::ZeroVector;
	FLinearColor Color = FLinearColor::White;
	float Candelas = 0.f;
	float RadiusCm = 0.f;
	float Age = 0.f;
	float Life = 0.f;
	/** The frame it started on: not aged that frame, so even a slow frame shows it once at full (the muzzle flash's rule). */
	uint64 StartFrame = 0;
};

/**
 * The impact flash lights as plain data: four slots (no spawn per hit; UImpactFlashSubsystem owns the four lights they drive).
 * A hit takes a free slot; beside a live flash it refreshes that one (a stream into one wall, a blast's pellets: one light
 * each, not four on one spot); with all lit, a brighter hit takes over the weakest and a dimmer one is dropped. Never more
 * than MaxSlots lit, whatever the fire.
 */
struct AI_LOOTER_SHOOTER_API FImpactFlashPool
{
	static constexpr int32 MaxSlots = 4;

	FImpactFlashSlot Slots[MaxSlots];

	/**
	 * Lights a flash at Place: the slot it took, or INDEX_NONE when none was (all MaxLit lit and brighter, a brighter live flash
	 * within MergeReach cm, the light off). Life s long; Frame is the frame it starts on.
	 */
	int32 Start(const FVector& Place, const FVfxLightShape& Shape, float LifeSeconds, int32 MaxLit, float MergeReach, uint64 Frame);

	/** Ages the live flashes (not those started this Frame) and frees the ended: a mask of the slots that just ended, bit per slot. */
	uint32 Advance(float DeltaSeconds, uint64 Frame);

	/** A slot's candelas now: its peak times (1 - age share) ^ ImpactLightFadePower; 0 when not live. */
	float StrengthOf(int32 Index) const;

	int32 NumLive() const;
};
