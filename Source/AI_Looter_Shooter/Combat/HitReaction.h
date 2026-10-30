#pragma once

#include "CoreMinimal.h"
#include "Combat/GunfeelReactTuning.h"
#include "Combat/HitFeelTuning.h"
#include "Creatures/CreatureRank.h"
#include "Elements/LooterElement.h"
#include "Weapons/WeaponTypes.h"

/**
 * Where on its body a hit landed (HITS layer 2; CreatureHitZones::ZoneOf maps each kind's bones): its kind's pose reacts
 * there. Weak is a weak spot that isn't the main crit (a Spitter's sac, a miner's lamp, the Bell-Ringer's bell); Core the
 * crit at a body's heart (the Unpaid's coal on a crit, the slime's core). No bone known (a power, a status, a test level's
 * damage): Body.
 */
enum class ECreatureHitZone : uint8 { Body, Head, Abdomen, Leg, Arm, Core, Shroud, Weak };

/**
 * How hard a hit lands (HITS layer 1; FHitReaction::WeightOf): Body (a burst under the heavy share), Heavy (a burst of
 * StaggerHeavyShare or a crit), Big (a single hit of BigShare, a sniper's or a rocket's, a close shotgun blast of its
 * power's share), Kill, Overkill (a kill OverkillShare past dead, or a Big kill). Each is heavier than the one before.
 */
enum class EHitWeight : uint8 { Body, Heavy, Big, Kill, Overkill };

/** Which side of it the shot came from, by its facing: Front within 60 degrees of it, Back past 120, else Left or Right. */
enum class EHitSide : uint8 { Front, Left, Right, Back };

/**
 * The hit a creature took last (ACreatureBase::GetLastHit; stored before its kind's OnHurt): how hard, by what gun, how,
 * which way, where. Its kind's flinch is sized by it (FlinchStrength); its death goes by it (UCreatureHitReactionComponent).
 */
struct AI_LOOTER_SHOOTER_API FCreatureHit
{
	/** This hit's damage over its max health. */
	float Share = 0.f;
	/** Its burst's so far (a shotgun blast's pellets together; a lone hit's own share): what its flinch is sized by. */
	float BurstShare = 0.f;
	/** The gun that fired it (None: not a gun, or not known). */
	EWeaponKind Kind = EWeaponKind::None;
	/**
	 * The element whose status dealt it (a burn's piece, a smite, the salt), None for anything else. Such a hit has no gun's
	 * Kind (a burn is not a shotgun blast): a kill by it is the element's death (CreatureKillLook, ElementDeathLook), with
	 * neither the gun's corpse knock nor the kind's own death burst.
	 */
	ELooterElement Element = ELooterElement::None;
	bool bCritical = false;
	/** The shot's way (from the shooter through the wound, unit), and where it landed. */
	FVector Direction = FVector::ZeroVector;
	FVector Location = FVector::ZeroVector;
	/** A boss (its moves never interrupted: a smaller flinch). */
	bool bBoss = false;
	/** The killing hit: its death motion (throe, corpse knock) carries it, so its flinch stays today's size. */
	bool bKilled = false;
	/**
	 * Set while the hit's OnHurt runs (ACreatureBase::HandleDamaged): a flinch played any other way (a boss's own scripted
	 * jolt calling OnHurt) isn't this hit's, so it keeps its own size.
	 */
	bool bFresh = false;

	// --- HITS (2026-10-28): what the kinds' new poses go by (UCreatureHitReactionComponent::TakeHit fills them) ---
	/** The bone the shot hit (None: not known: a power, a status, a test level's damage). */
	FName Bone;
	/** Where on the body (Body with Looter.Hits.Zones 0). */
	ECreatureHitZone Zone = ECreatureHitZone::Body;
	/** How hard it landed. */
	EHitWeight Weight = EHitWeight::Body;
	/** Which side the shot came from (Front with Looter.Hits.Zones 0). */
	EHitSide Side = EHitSide::Front;
	/** A killing hit's damage past what health it had left, over its max health (0 for any other hit). */
	float Overkill = 0.f;
	/**
	 * How far the wound is off the body's upright axis across the shot (cm, world): + on the creature's right as it faces
	 * the shot (a front shot's right shoulder). Whatever side the shot came from, + twists it toward + yaw (the torque of
	 * the shot on that wound) and - the other way. 0 with Looter.Hits.Zones 0.
	 */
	float LateralOffset = 0.f;
	/** The shooter's distance (cm; below 0: not known). */
	float Distance = -1.f;
	/** The far boost on its flinch (HitFeel::FarBoostAt: 1 within 10 m, up to 1.5 at 25 m), set by TakeHit. */
	float FarBoost = 1.f;

	/**
	 * How hard its kind flinches, times today's flinch: Scale x min(clamp(Floor + DamageGain x BurstShare, Floor, Max) x
	 * FarBoost, Max), a crit x CritScale, a boss x BossScale. 1 when not fresh. The strongest hit of a burst stands (each
	 * kind keeps the bigger kick), so a blast's pellets don't add up; the last of them carries the blast's whole share.
	 */
	float FlinchStrength(const FGunfeelReactTuning& Tuning) const;
};

/**
 * One creature's reactions to the hits it takes, as plain rules over the world's clock (no engine state, so the tests
 * drive it): a hit-stop (its own time all but stopped for an instant, never the world's), a stagger (its brain paused and
 * a shove back) and a knock-back. UCreatureHitReactionComponent plays them on the creature. The numbers are Tuning's
 * (Looter.Gunfeel.* cvars) and Feel's (Looter.Hits.*; the component sets the live ones each hit, tests keep the defaults).
 *
 *  - Hit-stop: a crit (HitStopCrit; a revolver's RevolverCritHitStop), a Big hit (HitStopBig) or a heavy body hit
 *    (HitStopHeavy) at most every HitStopCooldown together (a rifle on a head would otherwise freeze it half the time); the
 *    kill (HitStopKill, a critical one HitStopCritKill, an overkill HitStopOverkill), always.
 *  - Stagger: on a heavy hit (a crit, or a burst of damage worth StaggerHeavyShare of its health inside BurstWindow: a
 *    shotgun's pellets at close range land together). Basic StaggerSeconds, the tougher ranks shorter in the same
 *    proportions as ever (Restless 0.8, Gravebound 0.63...); then StaggerCooldown before the next, so nothing is
 *    stun-locked, and Gravebound or tougher never sooner than StaggerGap after the last began. Soulfed monsters and bosses
 *    never stagger (their fights are made around their moves), and bosses get no hit-stop either: a boss's own code runs
 *    its big moments.
 *  - Poise (HITS): each hit's share of health adds up, draining PoiseDrain a second; at PoiseBreak x its rank's scale it
 *    staggers (EJob::Poise) and starts again from none. Poise only builds while a stagger could begin (not during one, its
 *    cooldown or an elite's gap), so steady fire staggers it at most once a stagger-and-cooldown plus a build-up.
 *  - The rifle's pressure: RiflePressureHits rifle hits within RiflePressureWindow stagger it too (the same cooldown).
 *  - The shotgun's power: once a blast, a burst worth ShotgunKnockShare within ShotgunKnockRange knocks a creature of
 *    ShotgunKnockSize or smaller back (bKnockBack: the component throws it), a bigger one is staggered; a stagger that
 *    blast began gives way to it, and it skips the cooldown (each power hit has taken a third of its health, so it can't
 *    chain long), but not an elite's gap. Never on a Soulfed monster or a boss.
 *  - A melee strike (UPlayerMeleeComponent) is the panic button against a lunge, so it always lands heavy: a hit-stop of
 *    MeleeHitStop with no cooldown, and a stagger MeleeStaggerScale times the rank's whatever its share of health, its
 *    cooldown skipped (the strike's own 0.6 s keeps it from stun-locking much). Not while a stagger still runs, and the
 *    stagger cooldown starts after it as usual, so guns can't chain one straight on.
 */
struct AI_LOOTER_SHOOTER_API FHitReaction
{
	/** Why a stagger (or knock) came. */
	enum class EJob : uint8 { None, Heavy, Melee, Pressure, Power, Poise };

	/** What one hit starts: seconds of hit-stop and of stagger (0: none), and a knock-back. */
	struct FResponse
	{
		float HitStopSeconds = 0.f;
		float StaggerSeconds = 0.f;
		/** The share of its health the hit's burst has taken so far (a shotgun blast's pellets together): the shove's strength. */
		float BurstShare = 0.f;
		/** The shotgun's power: thrown back (the component's UCreatureKnockComponent::Knock), no stagger of its own. */
		bool bKnockBack = false;
		/** What brought the stagger or the knock on (None: neither). */
		EJob Job = EJob::None;
		/** How hard the hit landed (WeightOf). */
		EHitWeight Weight = EHitWeight::Body;
	};

	/** One hit as the rules see it. */
	struct FHit
	{
		float Damage = 0.f;
		float MaxHealth = 1.f;
		bool bCritical = false;
		/** This hit killed it. */
		bool bKilled = false;
		ECreatureRank Rank = ECreatureRank::Basic;
		/** A melee strike's (so is any hit while an FMeleeScope is open). */
		bool bMelee = false;
		/** The gun that fired it (None: not a gun). */
		EWeaponKind Kind = EWeaponKind::None;
		/** The shooter's distance (cm; below 0: not known, so no shotgun power). */
		float Distance = -1.f;
		/** Its size (ACreatureBase::GetSizeScale). */
		float Size = 1.f;
		/** A killing hit's damage past the health it had left, over its max health (0: none, or not a kill). */
		float Overkill = 0.f;
	};

	/** The creature took Hit at Now. */
	FResponse OnHit(double Now, const FHit& Hit);

	/**
	 * The creature took Damage (of MaxHealth) at Now; bKilled: this hit killed it. bMelee: a melee strike's (so is any hit
	 * while an FMeleeScope is open). Not a gun's.
	 */
	FResponse OnHit(double Now, float Damage, float MaxHealth, bool bCritical, bool bKilled, ECreatureRank Rank, bool bMelee = false);

	/**
	 * Marks the damage dealt while it lives as a melee strike's. Whoever hears the hit without being told what dealt it
	 * (UCreatureHitReactionComponent hears it through ACreatureBase::HandleDamaged, from the health's OnDamaged, which
	 * carries no damage type) gets it this way: the strike opens one around its damage, which the engine and the health
	 * hand on at once, on the game thread.
	 */
	struct FMeleeScope
	{
		FMeleeScope();
		~FMeleeScope();
		FMeleeScope(const FMeleeScope&) = delete;
		FMeleeScope& operator=(const FMeleeScope&) = delete;

		/** A melee strike's damage is being dealt right now. */
		static bool IsOpen();
	};

	/**
	 * A Thunder jolt burst on it at Now: a stagger of BasicSeconds at its rank (StaggerSecondsFor: a Soulfed's none) when one
	 * may come (CanStagger: none running, its cooldown over). True if it began.
	 */
	bool OnJolt(double Now, float BasicSeconds, ECreatureRank Rank);

	/**
	 * Its ward broke at Now (the typed wards, 2026-10-29 night: a break always staggers): a stagger of its rank's length
	 * (StaggerSecondsFor; never a Soulfed or a boss) whatever the cooldown or an elite's gap, one running started over. Job
	 * Power. True if it began.
	 */
	bool OnWardBreak(double Now, ECreatureRank Rank);

	bool IsHitStopped(double Now) const { return Now < HitStopEnd; }
	bool IsStaggered(double Now) const { return Now < StaggerEnd; }

	/**
	 * How much of a stagger pose shows at Now (0 to 1, FStaggerCurve over the stagger running; 0 when none runs, or with
	 * Looter.Hits.StaggerPose 0 as of the hit that began it). The kinds pose on it (HITS layer 3).
	 */
	float GetStaggerAlpha(double Now) const;

	/** Its poise at Now (each hit's share added up, drained since: a stagger at PoiseBreak x PoiseScaleFor(rank)). */
	float GetPoise(double Now) const;

	/** How much more poise a rank takes to stagger, of a Basic one's (Restless 1.15 ... Ethereal 1.9; 0: never staggers). */
	static float PoiseScaleFor(ECreatureRank Rank);

	/**
	 * How hard Hit lands, its burst so far BurstShare (EHitWeight): Big also takes a sniper's or a rocket's, and a shotgun
	 * blast of its power's share within its power's range; a Big kill is an overkill. With Looter.Hits.KillStyles 0 a kill is
	 * never more than a Kill.
	 */
	static EHitWeight WeightOf(const FHit& Hit, float BurstShare, const FGunfeelReactTuning& InTuning, const FHitFeelTuning& InFeel);

	/** When the latest hit-stop ends (the world's seconds). */
	double GetHitStopEnd() const { return HitStopEnd; }

	/** The numbers it goes by (the component sets the live cvars' before each hit; a new one has the defaults). */
	void SetTuning(const FGunfeelReactTuning& InTuning) { Tuning = InTuning; }
	const FGunfeelReactTuning& GetTuning() const { return Tuning; }
	/** HITS' numbers and switches (Looter.Hits.*), set by the component before each hit as SetTuning; a new one has the defaults. */
	void SetFeel(const FHitFeelTuning& InFeel) { Feel = InFeel; }
	const FHitFeelTuning& GetFeel() const { return Feel; }

	/** Forgets everything (a creature coming back), its tunings kept. */
	void Reset()
	{
		const FGunfeelReactTuning Kept = Tuning;
		const FHitFeelTuning KeptFeel = Feel;
		*this = FHitReaction();
		Tuning = Kept;
		Feel = KeptFeel;
	}

	/** How far its own time runs during a hit-stop: all but stopped (not quite 0, which some motion code divides by). */
	static constexpr float HitStopDilation = 0.02f;
	/** A crit's (or heavy hit's) hit-stop comes at most this often. */
	static constexpr float HitStopCooldown = 0.25f;

	/** Damage landing within this long of the burst's first hit counts together (a shotgun blast's pellets). */
	static constexpr float BurstWindow = 0.08f;

	/** A melee strike's hit-stop: a touch longer than a crit's, so the blow's weight is felt; a kill's own wins if longer. */
	static constexpr float MeleeHitStop = 0.06f;
	/** A melee strike staggers this many times as long as a heavy hit does (Basic 0.6 s): time to step back and shoot. */
	static constexpr float MeleeStaggerScale = 1.5f;

	/** How long a heavy hit staggers a creature of Rank (0: never), Basic's being BasicSeconds. */
	static float StaggerSecondsFor(ECreatureRank Rank, float BasicSeconds);
	/** ...at the default Basic seconds (FGunfeelReactTuning's). */
	static float StaggerSecondsFor(ECreatureRank Rank);
	/** Whether a creature of Rank gets hit-stops at all. */
	static bool HitStopsFor(ECreatureRank Rank);

private:
	/** A stagger begins now for Seconds, for Job (its poise spent). */
	void BeginStagger(double Now, float Seconds, FResponse& Response, EJob Job);
	/** Whether a stagger may begin at Now for a creature of Rank: none running, the cooldown over, an elite's gap past. */
	bool CanStagger(double Now, ECreatureRank Rank) const;
	/** Poise added up for this hit (drained to Now first) and, if it breaks, a stagger. */
	void TakePoise(double Now, const FHit& Hit, float StaggerFor, FResponse& Response);

	FGunfeelReactTuning Tuning;
	FHitFeelTuning Feel;

	double HitStopEnd = -1.0;
	/** A crit's (or heavy hit's) hit-stop can come again from here. */
	double HitStopReady = -1.0;
	double StaggerStart = -1000.0;
	double StaggerEnd = -1.0;
	/** A stagger can come again from here. */
	double StaggerReady = -1.0;
	double BurstStart = -1000.0;
	float BurstDamage = 0.f;
	/** This burst already had its shotgun power (one a blast). */
	bool bBurstPowered = false;
	/** The latest rifle hits' times (the pressure's count; oldest first). */
	TArray<double, TInlineAllocator<8>> RifleHits;
	/** Poise as of PoiseAt (it drains from then). */
	float Poise = 0.f;
	double PoiseAt = 0.0;
	/** The stagger running shows its pose (Feel.StaggerPose as of the hit that began it). */
	bool bStaggerPose = true;
};

/**
 * A stagger pose's strength over the stagger (HITS): up over its first Rise seconds (eased out: a jolt), held to HoldShare
 * of it, then eased back to none at its end. Seconds is the whole stagger's; Time since it began.
 */
struct AI_LOOTER_SHOOTER_API FStaggerCurve
{
	static float Alpha(float Time, float Seconds);

	/** The rise (s) and where the hold ends (share of the stagger). */
	static constexpr float Rise = 0.08f;
	static constexpr float HoldShare = 0.55f;
};

/** HITS' plain rules for a hit's side, offset, far boost and overkill (the component's TakeHit uses them; the tests drive them). */
namespace HitFeel
{
	/** The side Direction (the shot's way, from the shooter through the wound) comes from, for a creature facing Facing. */
	AI_LOOTER_SHOOTER_API EHitSide SideOf(const FVector& Facing, const FVector& Direction);

	/**
	 * The wound at Location off the upright axis through Center, across the shot's Direction (cm): + on the creature's right
	 * as it faces the shot (FCreatureHit::LateralOffset).
	 */
	AI_LOOTER_SHOOTER_API float LateralOffsetOf(const FVector& Center, const FVector& Direction, const FVector& Location);

	/** The flinch's far boost for a shooter Distance cm away (below 0: not known, x1): x1 within FarNear, FarBoost from FarFar. */
	AI_LOOTER_SHOOTER_API float FarBoostAt(float Distance, const FHitFeelTuning& Feel);

	/** A hit of Damage on HealthBefore (of MaxHealth): how far past dead it went, over MaxHealth (0 if it didn't kill). */
	AI_LOOTER_SHOOTER_API float OverkillOf(float Damage, float HealthBefore, float MaxHealth);

	/** Short names for the log and Looter.Hits.Show. */
	AI_LOOTER_SHOOTER_API const TCHAR* ZoneName(ECreatureHitZone Zone);
	AI_LOOTER_SHOOTER_API const TCHAR* WeightName(EHitWeight Weight);
	AI_LOOTER_SHOOTER_API const TCHAR* SideName(EHitSide Side);
}
