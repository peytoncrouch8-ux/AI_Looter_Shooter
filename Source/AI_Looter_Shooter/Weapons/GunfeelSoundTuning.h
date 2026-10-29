#pragma once

#include "CoreMinimal.h"
#include "Weapons/CreatureBlood.h"
#include "Weapons/GunshotSound.h"

/**
 * Gunfeel, the sound side (Docs/Plan.md 12.1 and the gun feel pass, Docs/Work/2026-10-22-Gunfeel/PLAN.md): which of a shot's
 * layers play (GunshotSound::PlayShot) and how loud, what the shooter's surroundings make the land answer with, how an
 * automatic's tails are spaced, how many rounds click at the end of a magazine, and how loud and heavy impacts and creature
 * hits sound by gun.
 *
 * Every number is a Looter.Gunfeel.Sound.* console variable read through an accessor here each time it is asked, so the
 * player tunes it live in play. The rules are plain functions over numbers (Default holds the shipped ones) so tests drive
 * them without a world or a variable the player may have set.
 */
namespace GunfeelSound
{
	/**
	 * How a kind balances its own layers (gun identity, 2026-10-28): the shooter's action (Mech), chest thump (Low) and the
	 * land's answer (Tail) times these, on top of the maker's flavour and the global Mech/Low/Tail. 1/1/1 is the plain kind.
	 */
	struct FKindMix
	{
		float Mech = 1.f;
		float Low = 1.f;
		float Tail = 1.f;
	};

	/** The numbers a variable starts at. */
	namespace Default
	{
		inline constexpr float Tail = 1.f;
		inline constexpr float Mech = 1.f;
		inline constexpr float Low = 1.f;
		inline constexpr float DistantFrom = 3000.f;
		inline constexpr float ProbeRange = 800.f;

		// --- Automatics: the tail every Nth shot of a burst, and one at its end ---
		inline constexpr int32 AutoTailRifle = 2;
		inline constexpr int32 AutoTailSMG = 3;
		inline constexpr int32 AutoTailLMG = 2;
		/** The burst-end tail's volume (the tail layer again, once, when the burst skipped the last shot's). 0 = none. */
		inline constexpr float BurstEndTail = 1.f;
		/** Shots further apart than this (s) are not one burst. */
		inline constexpr float BurstGap = 0.35f;

		// --- The end of a magazine ---
		/** Rounds left that click, by kind (clamped to half a magazine); the launcher's 1-3 round magazine never clicks. */
		inline constexpr int32 LowAmmoRifle = 6;
		inline constexpr int32 LowAmmoSMG = 10;
		inline constexpr int32 LowAmmoLMG = 15;
		inline constexpr int32 LowAmmoShotgun = 2;
		inline constexpr int32 LowAmmoRevolver = 2;
		inline constexpr int32 LowAmmoSniper = 1;
		inline constexpr int32 LowAmmoLauncher = 0;
		/** The click's top pitch at the last round: Looter.Gunfeel.LowAmmo.PitchTop's own default (the variable wins). */
		inline constexpr float LowAmmoPitchTop = 1.35f;
		/** The last-round sound's volume (0 = none) and how long after the shot it comes (s), so the shot doesn't bury it. */
		inline constexpr float LastRound = 1.f;
		inline constexpr float LastRoundDelay = 0.07f;

		// --- The LMG's hot barrel, the shotgun's rack ---
		/** Barrel heat (0..1) above which a burst's end hisses. */
		inline constexpr float HissFrom = 0.6f;
		inline constexpr float Hiss = 1.f;
		inline constexpr float Rack = 1.f;

		// --- Impacts: the surface cue's loudness by the gun's mark scale, the heavy and scatter layers ---
		inline constexpr float ImpactBase = 0.75f;
		inline constexpr float ImpactSlope = 0.25f;
		inline constexpr float ImpactMin = 0.8f;
		inline constexpr float ImpactMax = 1.2f;
		/** A blast's pellets merge into one surface cue (UBulletSubsystem), so that one is louder by this. */
		inline constexpr float ScatterLouder = 1.2f;
		inline constexpr float ImpactHeavyFrom = 1.2f;
		inline constexpr float ImpactHeavy = 1.f;
		inline constexpr float ImpactScatter = 1.f;

		// --- Creature hits ---
		/** Mark scale (FBulletMarks::GunScale) from which the body hit has the heavy thump under it. */
		inline constexpr float CreatureHeavyFrom = 1.2f;
		inline constexpr float CreatureHeavy = 1.f;
		/** A blast's pellets landing within CreatureBlastWindow (s) that together take this share of the creature's health thump too. */
		inline constexpr float CreatureHeavyShare = 0.35f;
		inline constexpr float CreatureBlastWindow = 0.08f;
		inline constexpr float CreatureCrit = 1.f;

		// --- Gun identity (GUNS.md): each kind's layer balance and its body punch ---
		/** The body punch under a shot (Weapon.<Kind>.Punch), all kinds, times this. 0 = none. */
		inline constexpr float Punch = 1.f;
		/** The rifle's punch sits at -4 dB (a share of 10^(-4/20)), so every third shot's thump is felt, not heard over the crack. */
		inline constexpr float PunchRifleGain = 0.63f;

		/** The shipped Mech/Low/Tail multipliers of a kind (Looter.Gunfeel.Sound.<Kind>.Mech/.Low/.Tail start at these). */
		constexpr FKindMix KindMix(EWeaponKind Kind)
		{
			switch (Kind)
			{
			case EWeaponKind::SMG: return { 1.2f, 0.75f, 0.85f };
			case EWeaponKind::LMG: return { 1.1f, 1.2f, 1.0f };
			case EWeaponKind::Revolver: return { 0.9f, 1.1f, 1.2f };
			case EWeaponKind::Shotgun: return { 1.0f, 1.25f, 1.1f };
			case EWeaponKind::Sniper: return { 0.9f, 1.2f, 1.35f };
			case EWeaponKind::Launcher: return { 1.0f, 1.2f, 1.0f };
			default: return { 1.0f, 1.0f, 1.0f };
			}
		}
		/** The punch plays on every Nth shot of a burst by kind (0 = never): rifle 3rd, SMG none, LMG 2nd, the single shots every one. */
		constexpr int32 PunchEvery(EWeaponKind Kind)
		{
			return Kind == EWeaponKind::Rifle ? 3 : Kind == EWeaponKind::SMG ? 0 : Kind == EWeaponKind::LMG ? 2 : 1;
		}
		constexpr float PunchGain(EWeaponKind Kind)
		{
			return Kind == EWeaponKind::Rifle ? PunchRifleGain : 1.f;
		}

		/** The shipped number of tails/low rounds for a kind (the variables start at these). */
		constexpr int32 AutoTailEvery(EWeaponKind Kind)
		{
			return Kind == EWeaponKind::Rifle ? AutoTailRifle : Kind == EWeaponKind::SMG ? AutoTailSMG : Kind == EWeaponKind::LMG ? AutoTailLMG : 1;
		}
		constexpr int32 LowAmmoRounds(EWeaponKind Kind)
		{
			switch (Kind)
			{
			case EWeaponKind::Rifle: return LowAmmoRifle;
			case EWeaponKind::SMG: return LowAmmoSMG;
			case EWeaponKind::LMG: return LowAmmoLMG;
			case EWeaponKind::Shotgun: return LowAmmoShotgun;
			case EWeaponKind::Revolver: return LowAmmoRevolver;
			case EWeaponKind::Sniper: return LowAmmoSniper;
			default: return LowAmmoLauncher;
			}
		}
	}

	/** How long a gun's look at its surroundings holds (s): a burst fires many shots from the same spot. */
	inline constexpr double ProbeSeconds = 0.25;

	// --- The live numbers (the console variables' current values) ---

	/** The tail's volume (the land answering), times this. */
	AI_LOOTER_SHOOTER_API float TailVolume();
	/** The shooter's own action (Mech) close to the ear, times this. */
	AI_LOOTER_SHOOTER_API float MechVolume();
	/** The shooter's own chest thump (Low), times this. */
	AI_LOOTER_SHOOTER_API float LowVolume();
	/** Someone else's shot farther than this from the listener (cm) is heard as the far one (Distant). */
	AI_LOOTER_SHOOTER_API float DistantFrom();
	/** How far the surroundings probe looks for a roof and walls (cm). */
	AI_LOOTER_SHOOTER_API float ProbeRange();

	/** An automatic's tail on every Nth shot of a burst (Looter.Gunfeel.Sound.AutoTailEvery.<Rifle|SMG|LMG>; 1 for the rest, never under 1). */
	AI_LOOTER_SHOOTER_API int32 AutoTailEvery(EWeaponKind Kind);
	/** The volume of the tail at a burst's end (0 = none), the gap that ends a burst (s). */
	AI_LOOTER_SHOOTER_API float BurstEndTailVolume();
	AI_LOOTER_SHOOTER_API float BurstGap();
	/** Rounds left that click for Kind (Looter.Gunfeel.Sound.LowAmmo.<Kind>), and the click's top pitch (Looter.Gunfeel.LowAmmo.PitchTop). */
	AI_LOOTER_SHOOTER_API int32 LowAmmoRounds(EWeaponKind Kind);
	AI_LOOTER_SHOOTER_API float LowAmmoPitchTop();
	/** The last-round sound's volume (0 = none) and its delay after the shot (s). */
	AI_LOOTER_SHOOTER_API float LastRoundVolume();
	AI_LOOTER_SHOOTER_API float LastRoundDelay();
	/** The hot barrel's hiss: the heat it starts above, and its volume. */
	AI_LOOTER_SHOOTER_API float HissFrom();
	AI_LOOTER_SHOOTER_API float HissVolume();
	/** The shotgun's rack volume. */
	AI_LOOTER_SHOOTER_API float RackVolume();

	// --- Gun identity (GUNS.md; all of it plain with Looter.Gunfeel.Identity 0) ---

	/** The kind's layer balance (Looter.Gunfeel.Sound.<Kind>.Mech/.Low/.Tail; never negative); 1/1/1 with Identity 0. */
	AI_LOOTER_SHOOTER_API FKindMix KindMix(EWeaponKind Kind);
	/** The body punch's volume for Kind: Looter.Gunfeel.Sound.Punch x the kind's gain (Looter.Gunfeel.Sound.PunchGain.<Kind>); 0 with Identity 0. */
	AI_LOOTER_SHOOTER_API float PunchVolume(EWeaponKind Kind);
	/** The punch on every Nth shot of a burst (Looter.Gunfeel.Sound.PunchEvery.<Kind>; 0 = never, never negative). */
	AI_LOOTER_SHOOTER_API int32 PunchEvery(EWeaponKind Kind);
	/** The surface cue's volume for a bullet of MarkScale (bBlast: one of a shotgun's pellets, whose cue stands for the blast). */
	AI_LOOTER_SHOOTER_API float ImpactVolume(float MarkScale, bool bBlast);
	/** Mark scale from which an impact has the heavy layer (0 = never), its volume, and the scatter pepper's volume. */
	AI_LOOTER_SHOOTER_API float ImpactHeavyFrom();
	AI_LOOTER_SHOOTER_API float ImpactHeavyVolume();
	AI_LOOTER_SHOOTER_API float ImpactScatterVolume();
	/** Mark scale from which a body hit has the heavy thump (0 = never), its volume, the blast's health share, the window pellets add up in (s), the crit layer's volume. */
	AI_LOOTER_SHOOTER_API float CreatureHeavyFrom();
	AI_LOOTER_SHOOTER_API float CreatureHeavyVolume();
	AI_LOOTER_SHOOTER_API float CreatureHeavyShare();
	AI_LOOTER_SHOOTER_API float CreatureBlastWindow();
	AI_LOOTER_SHOOTER_API float CreatureCritVolume();

	// --- The rules ---

	/** What's round the shooter, for the tail: open land, walls close by (a street, a yard), or a room or a tunnel. */
	enum class ESurroundings : uint8
	{
		Open,
		Town,
		Inside,
	};

	/**
	 * The surroundings from the probe's hits: a roof over the shooter with walls on 2 or more sides is Inside; walls on 2
	 * or more sides, or a roof alone (a porch, an overhang, a bridge), is Town; anything less is Open. A roof alone isn't
	 * a room: a warehouse's tail under a porch roof or a ledge would be wrong.
	 */
	AI_LOOTER_SHOOTER_API ESurroundings Classify(bool bRoof, int32 WallSides);

	/** The tail layer for the surroundings. */
	AI_LOOTER_SHOOTER_API EGunshotLayer TailFor(ESurroundings Surroundings);

	/** Which of a shot's layers play, and the tail's. */
	struct FShotLayers
	{
		/** The punch (Weapon.<Kind>.Core): everyone hears it within DistantFrom. */
		bool bCore = false;
		/** The action and the chest thump: the shooter's own gun only. */
		bool bMech = false;
		bool bLow = false;
		/** The land's answer by the surroundings (Tail). */
		bool bTail = false;
		EGunshotLayer Tail = EGunshotLayer::TailOpen;
		/** The far shot, in place of the punch and the tail. */
		bool bDistant = false;
	};

	/**
	 * The layers of a shot: the shooter's own gun (bOwnGun: a locally controlled pawn's) plays the punch, the action, the
	 * chest thump and the tail; anyone else's plays the punch and the tail, or past DistantFromCm from the listener the far
	 * shot alone (its take carries its own field).
	 */
	AI_LOOTER_SHOOTER_API FShotLayers Choose(bool bOwnGun, float DistanceToListener, float DistantFromCm,
		ESurroundings Surroundings);

	/** Whether a gun's last look at its surroundings (at LastProbe, s; negative: never) is too old at Now. */
	AI_LOOTER_SHOOTER_API bool ProbeStale(double Now, double LastProbe, double MaxAge = ProbeSeconds);

	// --- Bursts: a tail every Nth shot, one at the end ---

	/** Whether the shot at Now starts a new burst: never fired, further than Gap after the last, or the clock went back. */
	AI_LOOTER_SHOOTER_API bool NewBurst(double Now, double LastShot, double Gap);

	/** Whether shot number ShotInBurst (0 the first) plays its tail when a tail comes every Every shots: the first always does. */
	AI_LOOTER_SHOOTER_API bool TailPlays(int32 ShotInBurst, int32 Every);

	/**
	 * Whether a burst's end plays the long tail: the burst had BurstShots (an automatic's run of 2 or more), its last shots
	 * skipped their tails (ShotsSinceTail above 0), and Volume is above 0.
	 */
	AI_LOOTER_SHOOTER_API bool BurstEndTailPlays(int32 BurstShots, int32 ShotsSinceTail, float Volume);

	// --- The end of a magazine ---

	/**
	 * How many of the last rounds click: Rounds, never more than half the magazine (a short magazine doesn't click through
	 * itself), none for a magazine under 4 or Rounds of 0.
	 */
	AI_LOOTER_SHOOTER_API int32 LowAmmoWindow(int32 Rounds, int32 Magazine);

	/** The click's pitch for a shot that left RoundsLeft: 1 at the window's first round up to Top at the last before empty; 0 = no click. */
	AI_LOOTER_SHOOTER_API float LowAmmoPitch(int32 RoundsLeft, int32 Window, float Top);

	/** Whether the shot that left RoundsLeft of Magazine plays the last-round sound: Kind has one (not the launcher), it emptied it, Volume above 0. */
	AI_LOOTER_SHOOTER_API bool LastRoundPlays(EWeaponKind Kind, int32 RoundsLeft, int32 Magazine, float Volume);

	/** Whether a burst's end hisses: the LMG's Heat above From after a burst of 2 or more, Volume above 0. */
	AI_LOOTER_SHOOTER_API bool HeatHissPlays(EWeaponKind Kind, float Heat, float From, int32 BurstShots, float Volume);

	/** The hiss's loudness: half at From, full at a glowing barrel (Heat 1). */
	AI_LOOTER_SHOOTER_API float HeatHissLoudness(float Heat, float From);

	/**
	 * Whether shot number ShotInBurst (0 the first) plays the body punch when it comes on every Every shots: the first of a
	 * burst always does; Every of 0 or less = never (the SMG).
	 */
	AI_LOOTER_SHOOTER_API bool PunchPlays(int32 ShotInBurst, int32 Every);

	/** Whether the punch is heard: the shooter's own gun only (it is a thump in the chest, 2D), at a volume above 0. */
	AI_LOOTER_SHOOTER_API bool PunchAudible(bool bOwnGun, float Volume);

	/** Whether the rack is heard: the shooter's own always, someone else's within DistantFrom of the listener. */
	AI_LOOTER_SHOOTER_API bool RackAudible(bool bOwnGun, float DistanceToListener, float DistantFromCm);

	// --- Impacts and creature hits ---

	/** The surface cue's loudness for a mark scale: Base + Slope x scale, held between Min and Max. */
	AI_LOOTER_SHOOTER_API float ImpactLoudness(float MarkScale, float Base, float Slope, float Min, float Max);

	/** Whether MarkScale reaches From (the heavy layers' threshold; From of 0 or less = never). */
	AI_LOOTER_SHOOTER_API bool IsHeavy(float MarkScale, float From);

	/** Pellets of one blast adding up on one creature: the window's start (s; negative: none), the share of its health taken, and whether the thump already played. */
	struct FBlastTally
	{
		double Start = -1.0;
		float Share = 0.f;
		bool bPlayed = false;
	};

	/**
	 * A hit taking Share of the creature's health at Now: true once, when the hits within Window of the first add up to
	 * From (a shotgun blast's pellets, each small, together a heavy blow). From of 0 or less = never.
	 */
	AI_LOOTER_SHOOTER_API bool BlastIsHeavy(FBlastTally& Tally, double Now, float Share, float Window, float From);

	/** The crit crack for what a body spills: shell (the spiders, the Gravemother), gel (slimes, clay), coal (the Unpaid and miners); none for no body. */
	AI_LOOTER_SHOOTER_API FName CritCueFor(ECreatureBlood Blood);

	/** The hit confirm the player hears: the ward's tick when a ward took it, else the crit ping, else the plain tick. */
	AI_LOOTER_SHOOTER_API FName HitConfirmCue(bool bCritical, bool bWardHit);

	/** The kill confirm: the crit kill's when the killing blow was a critical and the bank has it, else the plain one. */
	AI_LOOTER_SHOOTER_API FName KillConfirmCue(bool bCriticalKill, bool bBankHasCritKill);
}
