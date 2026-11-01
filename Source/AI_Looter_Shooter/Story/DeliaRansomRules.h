#pragma once

#include "CoreMinimal.h"

/** A moment in Delia's fight that draws a bark (DA_Lines_DeliaFight). */
enum class EDeliaBark : uint8
{
	/** She first sees a fight coming. */
	Opening,
	/** She breaks her gun open. */
	Reload,
	/** A Dredger hose-man comes into her reach. */
	HoseMan,
	/** The one she shot at goes down. */
	Down,
	/** Something hits her. */
	Hurt,
	/** She's knocked one down to the player's finish. */
	Finish,
	/** The porch is clear. */
	Clear,
};

/** One creature Delia might shoot at, as she sees it this moment. */
struct FDeliaTarget
{
	/** Its middle (where she aims), its health and its most. */
	FVector Where = FVector::ZeroVector;
	float Health = 0.f;
	float MaxHealth = 1.f;
	/** Nothing in the way from her gun to it. */
	bool bSeen = false;
};

/**
 * How Delia Ransom fights on her porch (ADeliaRansom; the user's call, 2026-10-31: she fights beside the player, strong
 * but leaving the work to them), as plain rules the tests check. A blast every ShotInterval at the nearest creature within
 * FightRadius of her porch, about BlastShare of its whole health if her pellets land (a Dredger's 25%), two shells then a
 * reload; never a critical; and never below LeaveShare of a creature's health: she knocks them down to where the player
 * finishes them (the kill and its experience are the player's), and holds her fire on one that's there, calling on the
 * player to finish it. She holds fire too while the player is in her line.
 */
namespace DeliaFightRules
{
	/** She fights creatures within this of her porch (cm). */
	inline constexpr float FightRadius = 1800.f;
	/** Seconds between blasts, and how much either way each wait varies. */
	inline constexpr float ShotInterval = 1.8f;
	inline constexpr float ShotJitter = 0.2f;
	/** Shells in her gun: two, then a reload of this long (s). */
	inline constexpr int32 ShellsPerLoad = 2;
	inline constexpr float ReloadSeconds = 2.4f;
	/** A blast's whole damage if every pellet lands, as a share of its target's most health. */
	inline constexpr float BlastShare = 0.25f;
	/** She never takes a creature below this share of its most health. */
	inline constexpr float LeaveShare = 0.1f;
	/** A creature only this far over the floor isn't worth a shell: hers to leave to the player. */
	inline constexpr float WorthMargin = 0.05f;
	/** Pellets in a blast, and the share of them that land on a body at her ranges (the spread fits it). */
	inline constexpr int32 Pellets = 9;
	inline constexpr float LandingShare = 0.8f;
	/** The pattern's spread: about this wide (cm, radius) at the target, within these (degrees). */
	inline constexpr float PatternRadius = 35.f;
	inline constexpr float MinSpread = 1.5f;
	inline constexpr float MaxSpread = 5.f;
	/** A shot only once the gun is on its target within this (degrees) and fully up. */
	inline constexpr float AimTolerance = 10.f;
	/** She stays ready this long after the last creature near her goes (s). */
	inline constexpr float StayReadySeconds = 2.5f;
	/** The player this near her line of fire (cm), between her and her target, holds her fire. */
	inline constexpr float LineClearance = 70.f;

	/** Where lies within Radius of Porch (flat: a creature on the steps or the yard below counts the same). */
	bool InReach(const FVector& Porch, const FVector& Where, float Radius = FightRadius);

	/** A creature with this much health is worth a blast (above the floor she leaves for the player, by a margin). */
	bool WorthABlast(float Health, float MaxHealth, float Leave = LeaveShare);

	/**
	 * Which of Targets she turns on (INDEX_NONE: none): the nearest seen one worth a blast; failing that the nearest seen
	 * at all, to cover (bOutWorth false: she aims but holds her fire).
	 */
	int32 Choose(const FVector& From, const TArray<FDeliaTarget>& Targets, bool& bOutWorth, float Leave = LeaveShare);

	/** Each pellet's damage against a target with MaxHealth (before the game's roll). */
	float PelletDamage(float MaxHealth, float Share = BlastShare, int32 Count = Pellets);

	/**
	 * What a pellet's damage is scaled by as it lands on something with Health of MaxHealth (UBulletSubsystem's landing
	 * scale): whole while its best roll leaves the floor, less as the floor nears, 0 at it.
	 */
	float LandingScale(float Health, float MaxHealth, float Pellet, float Leave = LeaveShare);

	/** The pattern's spread for a target Distance away (degrees, half the cone). */
	float SpreadFor(float Distance);

	/** The player at Player stands in the line from Muzzle to Target (between them, within LineClearance of it). */
	bool PlayerInLine(const FVector& Muzzle, const FVector& Target, const FVector& Player, float Clearance = LineClearance);

	/**
	 * A reload's moves at Share of the way through it (0-1): the gun broken open, and the left hand to the apron pocket
	 * for the shells and back.
	 */
	void ReloadPose(float Share, float& OutBreak, float& OutToPocket);

	/**
	 * Which of DA_Lines_DeliaFight's lines suit Moment, by their place in STORY's set (create_story_lines.py, 8 lines):
	 * 0 "Off my porch!" opens a fight; 3 "Reloading! Cover me" a reload; 4 "Hose-man coming!" a hose-man; 2 "That's for my
	 * son!" (or 1) one going down; 6, 5 a hit; 7, 1, 5 the finish left to the player; none yet for the all-clear. A set
	 * shorter than an index skips it. Reorder the set and this table goes with it.
	 */
	const TArray<int32>& BarkLines(EDeliaBark Moment);
}
