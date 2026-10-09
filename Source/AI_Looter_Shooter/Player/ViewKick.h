#pragma once

#include "CoreMinimal.h"
#include "Weapons/WeaponTypes.h"

/**
 * One jolt of the view: how far it turns (degrees; pitch up, yaw right, roll clockwise) and how much wider the field of
 * view gets (a share: 0.02 widens it 2%, a negative one narrows it), at its peak, and how it moves: a damped spring that
 * snaps to the peak in a frame or two and settles back with a small overshoot. Frequency (Hz) sets how quick it is,
 * Damping (0-1) how soon it settles.
 */
struct AI_LOOTER_SHOOTER_API FViewKick
{
	float Pitch = 0.f;
	float Yaw = 0.f;
	float Roll = 0.f;
	float FieldOfView = 0.f;
	float Frequency = 10.f;
	float Damping = 0.55f;

	/** The whole kick times Scale (its springs unchanged). */
	FViewKick operator*(float Scale) const;
};

/**
 * The jolts on a player's view right now, as plain math (no engine state, so the tests drive it): each kick plays out
 * as its spring's impulse response, peaking at the kick's numbers, and the view shows their sum, held within limits so
 * a pile of them never spins it. UCameraShakeModifier plays it on the camera; nothing here moves the aim.
 */
class AI_LOOTER_SHOOTER_API FViewKickStack
{
public:
	void Add(const FViewKick& Kick);
	void Tick(float DeltaSeconds);
	void Reset() { Active.Reset(); }

	/** The view's turn right now (degrees), the sum of every kick, held within MaxTurn. */
	FRotator GetRotation() const;

	/** The field of view's change right now, as a share (0.02 = 2% wider), held within MaxFieldOfView. */
	float GetFieldOfView() const;

	bool IsSettled() const { return Active.IsEmpty(); }
	int32 Num() const { return Active.Num(); }

	/**
	 * A spring's impulse response Time seconds in, scaled so its first peak is 1: 0 at the start, 1 a frame or two later,
	 * then a small swing past zero, settling to nothing.
	 */
	static float Response(float Time, float Frequency, float Damping);

	/** A kick is dropped once its spring's swing is under this share of its peak. */
	static constexpr float SettledShare = 0.005f;
	/** No more than this many at once (a full-auto burst keeps a handful going); the oldest gives way. */
	static constexpr int32 MaxKicks = 12;
	/** The most the view turns (pitch and roll; yaw is held to two thirds of it) and widens or narrows, all kicks together. */
	static constexpr float MaxTurn = 3.f;
	static constexpr float MaxFieldOfView = 0.05f;

private:
	struct FActive
	{
		FViewKick Kick;
		float Age = 0.f;
	};
	TArray<FActive, TInlineAllocator<MaxKicks>> Active;
};

/**
 * The game's view kicks, by what caused them. Small on purpose: they sell a shot, a hit or a kill without making anyone
 * sick, and the player's camera shake setting scales all of them (UCameraShakeModifier).
 */
namespace ViewKicks
{
	/**
	 * A shot from a gun of Kind: the rifle a crisp tick (0.32 degrees up, a 0.5% wider view, settled in about 80 ms), the
	 * shotgun a heavy shove (1.15 degrees, 2.2% wider, about 180 ms), the revolver a sharp snap (0.9 degrees, 1.2% wider,
	 * settled in about 95 ms). The gun's Recoil stat (its parts) scales it (held to
	 * 0.6-1.5), and looking through the sight (AimAlpha 1) halves the turn and takes 40% off the widening, so the sight
	 * stays readable. Its roll and yaw go a random way from Random.
	 */
	AI_LOOTER_SHOOTER_API FViewKick ForShot(EWeaponKind Kind, float RecoilStat, float AimAlpha, FRandomStream& Random);

	/**
	 * The player hurt for DamageShare of their most health, from Source's side: the view knocked away from the hit (rolled
	 * away from its side, tipped back from a hit ahead or forward from one behind) and squeezed a little. A hit for a
	 * quarter of the bar or more is the full jolt (about 2.5 degrees of roll); a scratch is a fifth of it. SourceSide: how
	 * far right of the view the hit came from (-1 left, 1 right); SourceAhead: how far ahead (-1 behind, 1 ahead).
	 */
	AI_LOOTER_SHOOTER_API FViewKick ForHurt(float DamageShare, float SourceSide, float SourceAhead);

	/** A kill by the player: a short punch in (1.6% narrower) with a nod; bHeavy (a crit kill, or a Gravebound and up) is 40% more. */
	AI_LOOTER_SHOOTER_API FViewKick ForKill(bool bHeavy);
}
