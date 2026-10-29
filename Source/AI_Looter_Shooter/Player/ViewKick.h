#pragma once

#include "CoreMinimal.h"
#include "Weapons/WeaponTypes.h"

/**
 * One jolt of the view: how far it turns (degrees; pitch up, yaw right, roll clockwise), how much wider the field of view
 * gets (a share: 0.02 widens it 2%, a negative one narrows it) and how far the camera is pushed back along the view (cm),
 * at its peak, and how it moves: a damped spring that snaps to the peak in a frame or two and settles back with a small
 * overshoot. Frequency (Hz) sets how quick it is, Damping (0-1) how soon it settles, Delay (s) how long it waits to start.
 */
struct AI_LOOTER_SHOOTER_API FViewKick
{
	float Pitch = 0.f;
	float Yaw = 0.f;
	float Roll = 0.f;
	float FieldOfView = 0.f;
	/** The camera shoved back along the view (cm): the shoulder taking a heavy gun's shove (gun identity, 2026-10-28). */
	float Push = 0.f;
	float Frequency = 10.f;
	float Damping = 0.55f;
	/** Seconds before the spring starts (a launcher's heave follows its shove). */
	float Delay = 0.f;
	/**
	 * How far ahead along the view (cm) the turn pivots: 0 turns the view about the eye; a sight's settle turns it about the
	 * sight (its distance from the eye), so the sight stays on the line the shots fly while the world dips around it (a dip
	 * about the eye left the raised sight 0.03 cm off the shot's line: Looter.Weapons.Aim.CrouchAligned). Not scaled.
	 */
	float Pivot = 0.f;

	/**
	 * Sustained-shot cap (degrees of view pitch; 0 = this is no automatic's shot kick, nothing capped). A kick with one
	 * counts among the stack's shot kicks, and FViewKickStack::Add trims it so the shot kicks together stay under the cap
	 * (a lone kick bigger than the cap is never trimmed), but never below SustainFloor of its size.
	 */
	float SustainCap = 0.f;
	float SustainFloor = 0.25f;

	/** The whole kick times Scale (its springs unchanged). */
	FViewKick operator*(float Scale) const;

	/** It moves nothing (no turn, widening or push). */
	bool IsNone() const;
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

	/** How far the camera is pushed back along the view right now (cm), held within MaxPush either way. */
	float GetPush() const;

	/**
	 * How far the eye swings (world cm) so each kick with a Pivot turns the view about its point ahead rather than about the
	 * eye: for a view looking along View, its turn times Scale (the camera shake setting). Zero with no pivoted kick.
	 */
	FVector GetPivotShift(const FRotator& View, float Scale) const;

	/** The view pitch the shot kicks (those with a SustainCap) hold right now, upward swings only: what the cap measures. */
	float GetShotPitch() const;

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
	/** The most the camera is pushed back (cm), all kicks together. */
	static constexpr float MaxPush = 4.f;

private:
	struct FActive
	{
		FViewKick Kick;
		float Age = 0.f;
	};
	/** Each's spring now (its Delay waited out first). */
	static float Swing(const FActive& Each);
	TArray<FActive, TInlineAllocator<MaxKicks>> Active;
};

/**
 * How a shot's view kick comes through the sight, from its kind's identity (GunfeelKick::FKindFeel): at AimAlpha 1 it keeps
 * AimedTurn of the hip's turn and AimedFieldOfView of its widening and push. Lean (-1..1) is the side an LMG's sway is on,
 * which its roll follows.
 */
struct AI_LOOTER_SHOOTER_API FShotKickShape
{
	float AimedTurn = 0.5f;
	float AimedFieldOfView = 0.6f;
	float Lean = 0.f;

	/** Kind's shipped shape (GunfeelKick::KindFeelDefault), no lean. */
	static FShotKickShape For(EWeaponKind Kind);
};

/**
 * The game's view kicks, by what caused them. Small on purpose: they sell a shot, a hit or a kill without making anyone
 * sick, and the player's camera shake setting scales all of them (UCameraShakeModifier).
 */
namespace ViewKicks
{
	/** The Recoil stat a base Common shoots with (its stat 1 x Recoil.More 1.4): ForShot's table is its hip peaks there. */
	inline constexpr float BaseShotRecoil = 1.4f;

	/**
	 * A shot from a gun of Kind, each kind its own (gun identity, GUNS.md 2026-10-28; hip peaks at RecoilStat 1.4): the SMG a
	 * light buzz (0.2 degrees, 0.3% wider, 18 Hz), the rifle a crisp crack (0.55, 0.6%, 14 Hz), the LMG a slow chug that rolls
	 * the shoulders with its sway (0.6, 0.8%, 10 Hz), the revolver a hard snap that rolls the wrist clockwise (1.9, 1.2%,
	 * 11 Hz, well damped), the shotgun a shove through the shoulders (2.5, 3.6%, 6.5 Hz, pushed back 2 cm), the sniper a
	 * thunderclap (2.0, 1.6%, 8 Hz, 1.2 cm), the launcher a heave (1.4 up, 4.4% wider, 4.5 Hz, 3.5 cm; ForHeave follows it).
	 * Every one stays under the stack's limits at a base gun, so none clips: the heavy kinds read apart by shape. The Recoil
	 * stat scales it (as RecoilStat / 1.4, held to 0.4-2.0, so the accuracy's calm reaches both ends); through the sight the
	 * shape's shares are kept. Random sets the ways a roll or yaw may go (the revolver's never varies).
	 */
	AI_LOOTER_SHOOTER_API FViewKick ForShot(EWeaponKind Kind, float RecoilStat, float AimAlpha, FRandomStream& Random, const FShotKickShape& Shape);
	/** The same with Kind's shipped shape (FShotKickShape::For). */
	AI_LOOTER_SHOOTER_API FViewKick ForShot(EWeaponKind Kind, float RecoilStat, float AimAlpha, FRandomStream& Random);

	/**
	 * A shot's view kick as it was before gun identity (Looter.Gunfeel.Identity 0, times Default::Today's scale): the rifle
	 * a tick (0.32 degrees up, 0.5% wider), the shotgun a shove (1.15, 2.2%), the revolver a snap (0.9, 1.2%), the SMG 0.2,
	 * the LMG 0.42, the sniper 1.3 (2%), the launcher 1.4 (3%); the Recoil stat held to 0.6-1.5; through the sight half the
	 * turn and 60% of the widening for every kind.
	 */
	AI_LOOTER_SHOOTER_API FViewKick ForShotToday(EWeaponKind Kind, float RecoilStat, float AimAlpha, FRandomStream& Random);

	/**
	 * The launcher's heave after its shove: the tube's weight settling (0.5 degrees down, 0.16 s after the shot, 6 Hz),
	 * sized as ForShot is. Nothing (IsNone) for every other kind.
	 */
	AI_LOOTER_SHOOTER_API FViewKick ForHeave(EWeaponKind Kind, float RecoilStat, float AimAlpha, const FShotKickShape& Shape);

	/** The sights arriving at the eye: a small settle of the view (0.12 degrees down, 0.3% narrower, 9 Hz) times Scale. */
	AI_LOOTER_SHOOTER_API FViewKick ForAimSettle(float Scale);

	/**
	 * The player hurt for DamageShare of their most health, from Source's side: the view knocked away from the hit (rolled
	 * away from its side, tipped back from a hit ahead or forward from one behind) and squeezed a little. A hit for a
	 * quarter of the bar or more is the full jolt (about 2.5 degrees of roll); a scratch is a fifth of it. SourceSide: how
	 * far right of the view the hit came from (-1 left, 1 right); SourceAhead: how far ahead (-1 behind, 1 ahead).
	 */
	AI_LOOTER_SHOOTER_API FViewKick ForHurt(float DamageShare, float SourceSide, float SourceAhead);

	/** A kill by the player: a short punch in (1.6% narrower) with a nod; bHeavy (a crit kill, or a Gravebound and up) is 40% more. */
	AI_LOOTER_SHOOTER_API FViewKick ForKill(bool bHeavy);

	/**
	 * The shotgun's pump racked between shots (gun feel pass): a small dip of the view (0.1 degrees down, a 0.4% narrower
	 * field, settled in about 60 ms) so the rack is felt, not a second kick. Through the sight (AimAlpha 1) it halves.
	 */
	AI_LOOTER_SHOOTER_API FViewKick ForRack(float AimAlpha);

	/** The sniper's bolt slammed home after its cycle (gun identity): a nudge up (0.08 degrees, 12 Hz). Through the sight it halves. */
	AI_LOOTER_SHOOTER_API FViewKick ForBoltHome(float AimAlpha);
}
