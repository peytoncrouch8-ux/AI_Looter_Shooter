#pragma once

#include "CoreMinimal.h"

/**
 * The Dredgers' numbers (the Looter.Dredger.* cvars, live in PIE; field values are the defaults, so the rules' tests and a
 * fresh game agree). The contract's starting numbers (Docs/Work/2026-10-31-Wardens/CONTRACT.md, DREDGER): rifleman 140 HP,
 * 7 a bullet, a burst of 3 every 2.2 s, 70% accurate at 15 m and falling off; hose-man 220 HP, latch at 8 m, break at 14 m,
 * about 5 HP/s at level 1, the tank 30% of his health. Health and damage live on the classes (the bestiary reads them);
 * everything about how they fight is here.
 */
struct AI_LOOTER_SHOOTER_API FDredgerTuning
{
	// --- The rifleman's gun (Looter.Dredger.Rifle.*) ---
	/** Each bullet's damage against his AttackDamage (which has his level and rank in it), before Looter.Fight.DamageScale. */
	// 0.65 after the 2026-11-01 Landfall play-through: at 1.0 a deck of riflemen hit for 11-14 a round at 88% up close and
	// dropped a fresh level-1 Warden in seconds.
	float BulletScale = 0.65f;
	/** Rounds in a burst, seconds between them, and from one burst's start to the next. */
	int32 BurstCount = 3;
	float BurstGap = 0.13f;
	float BurstPeriod = 2.6f;
	/** The tell before a burst (s): he rises out of cover, shoulders the rifle, his lenses flare and the muzzle glints. */
	float RifleWindup = 0.6f;
	/** Rounds a magazine (three bursts) and a reload's length (s). */
	int32 Magazine = 9;
	float ReloadSeconds = 2.4f;
	/** The share of bullets aimed to hit: at NearRange or nearer, at 15 m, and at FarRange or farther (straight lines between). */
	float AccuracyNear = 0.6f;
	float Accuracy = 0.5f;
	float AccuracyFar = 0.25f;
	float NearRange = 800.f;
	float FarRange = 3000.f;
	/** A target crossing his line at sprint speed (600 cm/s) takes this share off his accuracy (less for slower). */
	float MovingPenalty = 0.3f;
	/** How fast his bullets fly (cm/s): quick, but slow enough to see the tracer come. */
	float BulletSpeed = 7500.f;

	// --- The rifleman's distance and cover (Looter.Dredger.Rifle.*) ---
	/** His band round his target (cm): he backs away inside Near, comes in from past Far, and likes Preferred. */
	float BandNear = 1000.f;
	float BandFar = 2500.f;
	float Preferred = 1600.f;
	/** He looks at his cover again this often (s), and moves on after this many bursts from one spot (0: never). */
	float CoverRecheck = 2.5f;
	int32 RelocateAfter = 3;
	/** Riflemen on one target spread round it this far apart (degrees): the second flanks one way, the third the other. */
	float FlankAngle = 40.f;
	/** Closer than this (cm) he clubs with the rifle's butt instead of firing. */
	float ButtReach = 230.f;

	// --- The hose-man's soul-hose (Looter.Dredger.Hose.*) ---
	/** He throws the line within this (cm), it breaks past this, and it flies this fast (cm/s). */
	float LatchRange = 800.f;
	float BreakRange = 1400.f;
	float LineSpeed = 3200.f;
	/** Health drained a second at level 1 (it grows with his level and rank, as his AttackDamage does). */
	float DrainPerSecond = 5.f;
	/** How hard the line pulls the victim toward him: a share of their own walking input (they walk away slower). */
	float PullShare = 0.35f;
	/** The tell before the throw (s): he plants, raises the nozzle, its mouth swells cyan, the tank's gauge flares. */
	float LatchWindup = 0.85f;
	/** He lets go after this long (s), waits this long before the next throw (s), and the line breaks once the world has cut it this long (s). */
	float LatchMaxSeconds = 8.f;
	float LatchRest = 2.5f;
	float BlockedBreak = 0.4f;
	/** 1: the drain keeps the rattle's frost on the screen's edges (until the HUD has its own cyan-black edge: lanes/DREDGER.md). */
	int32 FrostEdge = 0;

	// --- The hose-man's tank (Looter.Dredger.Tank.*) ---
	/** Bursts once tank hits have taken this share of his health; the burst staggers him this long (s at Regular). */
	float TankShare = 0.3f;
	float TankStagger = 1.6f;
	/** The burst's reach (cm), the share of their health it takes from other creatures in it, and from him. */
	float TankRadius = 320.f;
	float TankSplashShare = 0.15f;
	float TankSelfShare = 0.15f;

	// --- The body (Looter.Dredger.Body.*) ---
	/** The speeds (cm/s, full size) the mannequin's walk and jog clips were made at: the cycle plays at the speed moved. */
	float WalkClipSpeed = 165.f;
	float JogClipSpeed = 375.f;
	/** 1: a killed Dredger goes limp (ragdoll); 0: the code fall. */
	int32 Ragdoll = 1;
	/** 1: the stand-in kit (mask, lenses, cap) shows; 0 hides it: the default since the Tripo Dredgers (their own masks). */
	int32 Kit = 0;
	/** Two Dredger barks near each other at least this far apart (s). */
	float BarkGap = 1.6f;

	static FDredgerTuning Live();
};
