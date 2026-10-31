#pragma once

#include "CoreMinimal.h"
#include "Creatures/GravemotherFX.h"

class UObject;

/**
 * The Stone Sentinel's dust kept cheap (Main's PIE check 2026-10-31: a boulder bursting on the player dropped 80 fps to 12-14
 * for a second: thirty-odd big translucent dust cards filling the screen at the lens). Every puff its effects throw (the
 * sentinel, its boulder, its shockwave) goes through Dust: one card at a time, none whose grown size and drift would bring it
 * within LensClear of a local player's camera, and no more than a small budget a second across all of them. Smoke from
 * FWeaponFX (dirt, wave dust, ember puffs) and UGravemotherFX's jets are asked IsClearOfLens first. Opaque pieces (chips,
 * stones) cost next to nothing and stay, fewer and smaller.
 */
namespace StoneSentinelFX
{
	/** Nothing translucent spawns with its grown edge nearer a camera than this (cm). */
	inline constexpr float LensClear = 300.f;
	/** The dust budget across every sentinel effect: this many puffs a second, at most Burst at once. */
	inline constexpr float PuffsPerSecond = 24.f;
	inline constexpr float PuffBurst = 10.f;

	/**
	 * Whether a translucent piece LensDistance (cm) from the nearest camera, growing to Size across and travelling up to
	 * Travel (cm) over its life, would come within LensClear of it (plain rule; the tests check it).
	 */
	AI_LOOTER_SHOOTER_API bool CrowdsLens(float LensDistance, float Size, float Travel);

	/** The local players' cameras now (none outside a game world). */
	AI_LOOTER_SHOOTER_API void GatherLenses(const UObject* WorldContext, TArray<FVector, TInlineAllocator<2>>& OutLenses);

	/** How far At is from the nearest camera (cm; a huge number with none). */
	AI_LOOTER_SHOOTER_API float LensDistance(const UObject* WorldContext, const FVector& At);

	/** Whether a piece at At growing to Size across, travelling up to Travel, stays clear of every camera. */
	AI_LOOTER_SHOOTER_API bool IsClearOfLens(const UObject* WorldContext, const FVector& At, float Size, float Travel = 0.f);

	/**
	 * Burst's puffs round At (UGravemotherFX::Puffs), one card each, placed within its Spread and SpreadUp, each skipped if it
	 * would crowd a camera, all within the budget. Returns how many were spawned.
	 */
	AI_LOOTER_SHOOTER_API int32 Dust(const UObject* WorldContext, const FVector& At, const FGraveDustBurst& Burst);
}
