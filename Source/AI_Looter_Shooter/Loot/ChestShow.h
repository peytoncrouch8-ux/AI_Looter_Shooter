#pragma once

#include "CoreMinimal.h"
#include "Shields/ShieldTypes.h"
#include "Weapons/WeaponTypes.h"
#include "ChestShow.generated.h"

class UMaterialInstanceDynamic;

/**
 * A loot chest tier's show (the user's pick, 2026-10-28, the "escalating show": a bigger reveal means better loot), played
 * by AChest before and as its lid opens (ChestShow.cpp). Tiers 1-2: the two latches pop, the front light flicks, the lid
 * swings. Tier 3: the ring lock spins, its light turns from blue to green, then the lid. Tier 4: the orb pulses twice, the
 * gold trim lights in a sweep, the lid rises slowly. Tier 5: the crest unfolds off the lid, a burst of light and a short
 * rising tone, the lid, then the loot fountains out (guns and wards tossed up and out one by one, high). About 0.6 s to open
 * for tier 1, up to 2.2 s for tier 5 (each tier's ShowSeconds and LidSeconds, ChestKinds.cpp); it never holds the player.
 * Its pose is a pure function of how far the show and the lid have come (ChestShow::PoseAt), so a chest kept open by the
 * session snaps to its end with no sound or light.
 */

/** The glow slots a tier's show drives (Art/Models/Loot/ChestTiers.py's emissive slots, M_StylizedSurface: Glow, Color). */
enum class EChestGlow : uint8
{
	/** ChestTier<N>Glow: the tier's lights (tier 1's green latch light, 2's amber bar, 3's blue strips, 4's orb, 5's gold). */
	Main,
	/** ChestTier3LockGlow: the ring of tier 3's lock alone, so it can turn green. */
	LockRing,
	/** ChestTier4TrimGlow: tier 4's gold trim, dim at rest, lit in the sweep. */
	Trim,
	/** ChestTier5GemGlow: the gem on tier 5's crest. */
	Gem,
	/** ChestTier<N>InnerGlow (tiers 3-5): the strip round the inside under the rim, lit as the lid lifts. */
	Inner,
	Num
};

/** The show's moments, each played once as the show passes it (a sound, a flash): bits of ChestShow::MomentsBetween. */
namespace ChestShowMoment
{
	inline constexpr uint32 LatchLeft = 1u << 0;
	inline constexpr uint32 LatchRight = 1u << 1;
	inline constexpr uint32 LightFlare = 1u << 2;
	inline constexpr uint32 LockSpin = 1u << 3;
	inline constexpr uint32 LockOpen = 1u << 4;
	inline constexpr uint32 OrbPulse = 1u << 5;
	inline constexpr uint32 TrimSweep = 1u << 6;
	inline constexpr uint32 CrestUnfold = 1u << 7;
	inline constexpr uint32 Burst = 1u << 8;
}

/** Where a tier's show stands at a moment: its pieces' angles and its glows (times each slot's own Glow). */
struct FChestShowPose
{
	/** The latches' pitch on their lower pivots (degrees; negative pops the top out). */
	float LatchLeft = 0.f;
	float LatchRight = 0.f;
	/** The ring lock's roll (degrees), and how far its light has gone green (0 its blue, 1 green). */
	float LockRoll = 0.f;
	float LockGreen = 0.f;
	/** The crest's pitch on its top hinge (degrees; positive swings it out and up). */
	float CrestPitch = 0.f;
	/** Tier 4: the orb's halo (0 none to 1), and where the sweep's spark runs along the trim (0 left to 1 right; <0 none). */
	float OrbHalo = 0.f;
	float Sweep = -1.f;
	/** Each glow slot's strength over its own (1: as imported). */
	float Glow[static_cast<int32>(EChestGlow::Num)] = { 1.f, 1.f, 1.f, 1.f, 1.f };
};

namespace ChestShow
{
	/** The latches' pop, the crest's unfold and the lock's spin at their ends (degrees). */
	inline constexpr float LatchOpenPitch = -70.f;
	inline constexpr float CrestOpenPitch = 60.f;
	inline constexpr float LockTurnDegrees = 450.f;

	/** Tier 5's fountain: the first piece this long after the loot comes out, then one each FountainEvery (s). */
	inline constexpr float FountainFirst = 0.08f;
	inline constexpr float FountainEvery = 0.2f;

	/** Tier's show at PreShare (0 to 1: the show before the lid) and LidShare (0 shut to 1 open). Tier 0: nothing moves. */
	FChestShowPose PoseAt(int32 Tier, float PreShare, float LidShare);

	/** The moments a tier's show passes going from FromShare (exclusive; below 0 takes in the moments at 0) to ToShare. */
	uint32 MomentsBetween(int32 Tier, float FromShare, float ToShare);

	/** Every moment a tier's show has. */
	uint32 AllMoments(int32 Tier);

	/** Whether a tier's loot fountains out piece by piece (tier 5) rather than all at once. */
	bool Fountains(int32 Tier);

	/** No fountain piece flies within this many degrees of the opener's bearing from the chest. */
	inline constexpr float FountainClearOfOpener = 40.f;

	/**
	 * Which way tier 5's Index-th piece flies (degrees round the chest's forward, + to its right, Jitter added): to the sides in
	 * turn, each pair a little further round, mirrored to the other side when it would land toward the opener (OpenerBearing,
	 * the same measure; unset when nobody's there).
	 */
	float FountainTurn(int32 Index, float Jitter, TOptional<float> OpenerBearing);

	/** The glow slot's name on a tier's models ("ChestTier3LockGlow"), or none where the tier hasn't the slot. */
	FName GlowSlot(int32 Tier, EChestGlow Glow);
}

/** One piece of a tier 5's fountain, rolled with the rest of its loot and thrown when its turn comes. */
USTRUCT()
struct FChestFountainPiece
{
	GENERATED_BODY()

	UPROPERTY()
	FWeaponInstanceData Weapon;

	UPROPERTY()
	FShieldInstanceData Ward;

	UPROPERTY()
	bool bWard = false;

	UPROPERTY()
	FVector Velocity = FVector::ZeroVector;
};

/** A chest's show as it plays: its glow slots' dynamic instances, the moments played, the fountain still to throw. */
USTRUCT()
struct FChestShowState
{
	GENERATED_BODY()

	/** By EChestGlow; null where its models haven't the slot (or aren't imported). Made the first time the show needs them. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> Looks;

	/** Each slot's own Glow and Color, as imported. */
	float BaseGlow[static_cast<int32>(EChestGlow::Num)] = {};
	FLinearColor BaseColor[static_cast<int32>(EChestGlow::Num)];

	bool bLooksMade = false;

	/** The show's share at the last frame (moments between it and the next are played); below 0 before the first. */
	float LastPreShare = -1.f;

	/** How many moments have played (a snap-open plays none). */
	int32 MomentsPlayed = 0;

	/** The fountain: every piece, how many are out, the clock since the loot came out. */
	UPROPERTY(Transient)
	TArray<FChestFountainPiece> Fountain;

	int32 FountainThrown = 0;
	float FountainClock = 0.f;

	FChestShowPose Pose;
};
