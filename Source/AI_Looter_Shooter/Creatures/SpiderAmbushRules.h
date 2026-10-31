#pragma once

#include "CoreMinimal.h"
#include "Creatures/SpiderPerch.h"

class AAmbushSpawner;
class APawn;

/**
 * The spiders' ambush as plain rules (the user's pick, 2026-10-31: "spiders drop from walls/ledges"): where a lurker may hold
 * on, how it hangs there, when it springs and how (a leap onto the player, or a scramble down its wall), the leap's arc and
 * whether its landing bites. ASpiderCreature plays them (SpiderCreatureLurk.cpp, SpiderCreatureCling.cpp); the tests check
 * them without a world (Looter.Creatures.Spider.Ambush.*).
 */

/** The ambush's cues (Art/Sounds/recipes/spider_ambush.py; a cue with no sound yet plays nothing). */
namespace SpiderAmbushCue
{
	/** A lurker's faint chitter as the player comes near it (the player who listens hears it before it springs). */
	inline constexpr const TCHAR* Lurk = TEXT("Creature.Spider.Lurk");
	/** Springing: a shriek of stridulation and legs tearing off the rock (the tell: it comes 0.35 s before it leaves). */
	inline constexpr const TCHAR* Leap = TEXT("Creature.Spider.Leap");
	/** Landing from a leap: a heavy thump, eight feet striking, grit thrown. */
	inline constexpr const TCHAR* Land = TEXT("Creature.Spider.Land");
	/** Scrambling down its wall: claws scrabbling on rock. */
	inline constexpr const TCHAR* Climb = TEXT("Creature.Spider.Climb");
}

/** The ambush's numbers (cvars Looter.Spider.Lurk.*; Live() reads them, a default-made one has the defaults). Cm at size 1. */
struct AI_LOOTER_SHOOTER_API FSpiderLurkTuning
{
	/** Looter.Spider.Lurk: 1 lurkers wait on their perches; 0 every lurker springs as its ambush is sprung. */
	int32 Enabled = 1;
	/** Under it: the player within this (flat, from the ground under its perch) and seen springs it. */
	float SpringRadius = 650.f;
	/** Within this, a player who has passed it and walks away, who stares at it, or who lingers, springs it. */
	float PassRadius = 1000.f;
	/** Stared at this long (s), with the player's view on it within LookCone (a cosine). */
	float SpottedSeconds = 0.8f;
	float LookCone = 0.966f;
	/** Standing about within PassRadius this long (s). */
	float LingerSeconds = 2.5f;
	/** A leap reaches this far at most (3D, from its body to the player), at a player at most LeapHighest above its perch. */
	float LeapReach = 1500.f;
	float LeapHighest = 180.f;
	/** Its crouch before it leaps (s): its shriek is the tell. */
	float CoilSeconds = 0.35f;
	/** It aims this far ahead (s) of a moving player, and lands this short of them along its way (cm). */
	float LeadSeconds = 0.3f;
	float LandShort = 80.f;
	/** A landing within this of the player bites (cm), for this share of its bite. */
	float LeapBiteReach = 170.f;
	float LeapBiteShare = 0.8f;
	/** A mate's call: it springs this long after (s, by its dice), if the player is within MateReach of it. */
	float MateDelay[2] = { 0.35f, 1.2f };
	float MateReach = 2200.f;
	/** Down its wall at this (cm/s), hopping off once its body is this low over the ground. */
	float ClimbSpeed = 260.f;
	float ClimbHopHeight = 170.f;
	/** Its body's middle this far off the surface (cm): flattened against it. */
	float ClingRide = 34.f;
	/** Its landing's crouch before it fights (s); a leap that hasn't landed in LeapTimeout gives up the ambush. */
	float LandRecover = 0.35f;
	float LeapTimeout = 3.f;
	/** Within this it turns its head along the surface to follow the player (at most WatchMost degrees, WatchRate a second). */
	float WatchRadius = 1800.f;
	float WatchMost = 70.f;
	float WatchRate = 60.f;
	/** Its chitter's hint: within this, at most once every LurkCueGap (s). */
	float LurkCueRadius = 1400.f;
	float LurkCueGap = 6.f;

	/** The live numbers (the Looter.Spider.Lurk.* cvars). */
	static FSpiderLurkTuning Live();
};

/** What a surface's normal makes of it for a lurker. */
enum class ESpiderSurface : uint8
{
	/** Nothing it can hold. */
	None,
	/** Steep: a wall, a cliff, a trunk. */
	Wall,
	/** A top it can crouch on. */
	Ledge,
	/** Leaning out over the ground (the underside of a lip). */
	Overhang,
};

/** How a lurker springs. */
enum class ESpiderSpring : uint8
{
	None,
	/** A leap at the player (seen, in reach). */
	Leap,
	/** Down its wall at a scramble, then hunting (out of a leap's reach, or out of sight and shot at). */
	Climb,
};

/** Where a lurker is in its ambush (ASpiderCreature::GetLurk). */
enum class ESpiderLurkPhase : uint8
{
	/** Not in an ambush: it walks and fights. */
	None,
	/** On its perch, waiting. */
	Perched,
	/** Its crouch and shriek before a leap. */
	Coiling,
	/** In the air. */
	Leaping,
	/** Scrambling down its wall. */
	Climbing,
	/** Down from a leap, gathering itself. */
	Landing,
};

/** What a lurker knows of the player at one look (SpiderCreatureLurk.cpp). Distances in cm. */
struct FSpiderLurkSense
{
	/** Flat, from the ground under its perch. */
	float FlatDistance = TNumericLimits<float>::Max();
	/** From its body to the player's middle. */
	float Distance = TNumericLimits<float>::Max();
	/** The player's feet over the ground under its perch (+ above). */
	float Rise = 0.f;
	/** A clear line from its body to the player. */
	bool bSees = false;
	/** The player walking and heading away from it (they've passed it). */
	bool bMovingAway = false;
	/** How long the player's view has been on it, and how long they've stood about within its PassRadius (s). */
	float LookedAtSeconds = 0.f;
	float StillSeconds = 0.f;
	/** Shot on its perch; a mate's call come due; its pack's call (it has a target). */
	bool bHurt = false;
	bool bCalled = false;
	bool bHunting = false;
	/** Its own spring radius (0: the tuning's). */
	float SpringRadius = 0.f;
};

/** A lurker's state (ASpiderCreature keeps one: SpiderCreatureLurk.cpp moves it on). */
struct FSpiderLurkState
{
	ESpiderLurkPhase Phase = ESpiderLurkPhase::None;
	FSpiderPerchSpot Perch;
	ESpiderSurface Surface = ESpiderSurface::None;
	/** On the surface (world), its normal, the way its head hangs along it, and the way its head is turned now (watching). */
	FVector Point = FVector::ZeroVector;
	FVector Normal = FVector::UpVector;
	FVector Forward = FVector::ForwardVector;
	FVector Watch = FVector::ForwardVector;
	/** How far its head is lifted off the surface (degrees, eased): up as it watches someone near, higher in its coil. */
	float Raise = 0.f;
	/** The ground under (or before) its perch (world): its distances are measured from here. */
	FVector Foot = FVector::ZeroVector;
	/** Seconds in its phase, until its next look, and its looks' counts. */
	float Clock = 0.f;
	float SenseIn = 0.f;
	float LookedAt = 0.f;
	float Still = 0.f;
	FVector LastPreySpot = FVector::ZeroVector;
	/** Shot on its perch; a mate's call (s until it springs; below 0: none); its last chitter (s ago). */
	bool bHurt = false;
	float CallIn = -1.f;
	float SinceCue = 100.f;
	TWeakObjectPtr<APawn> Prey;
	TWeakObjectPtr<AAmbushSpawner> Owner;
	/** Its leap: from, to (the capsule's middle), and the body's turn as it left the surface. */
	FVector LeapFrom = FVector::ZeroVector;
	FVector LeapTo = FVector::ZeroVector;
	FQuat LeftFrom = FQuat::Identity;
	FVector LeftAt = FVector::ZeroVector;
	/** Climbing: its body over the ground below (cm, looked at now and then), and when it looks next. */
	float ClimbOver = 1000.f;
	float ClimbLookIn = 0.f;
	float ClimbCueIn = 0.f;
};

namespace SpiderAmbushRules
{
	/** What a surface with this normal is to a lurker. */
	AI_LOOTER_SHOOTER_API ESpiderSurface SurfaceOf(const FVector& Normal);
	/** The flat way FaceYaw points (degrees: 0 is +X). */
	AI_LOOTER_SHOOTER_API FVector FaceWay(float FaceYaw);
	/**
	 * The way its head hangs along a surface: down a wall or trunk (head down, the hunting spider's way); on a ledge, out
	 * along FaceYaw over the edge.
	 */
	AI_LOOTER_SHOOTER_API FVector ClingForward(const FVector& Normal, ESpiderPerchKind Kind, float FaceYaw);
	/** Its body's turn clinging: X along Forward, Z out of the surface. */
	AI_LOOTER_SHOOTER_API FQuat ClingRotation(const FVector& Normal, const FVector& Forward);
	/** Forward turned toward Toward along the surface (Normal), at most MostDegrees from Hang, by at most StepDegrees. */
	AI_LOOTER_SHOOTER_API FVector TurnWatch(const FVector& Current, const FVector& Hang, const FVector& Toward, const FVector& Normal,
		float MostDegrees, float StepDegrees);

	/** Whether the player as Sense sees them springs it now. */
	AI_LOOTER_SHOOTER_API bool WantsToSpring(const FSpiderLurkSense& Sense, const FSpiderLurkTuning& Tuning);
	/** How it springs (once it does): a leap when it sees them within a leap's reach, else down its wall. A ledge never climbs. */
	AI_LOOTER_SHOOTER_API ESpiderSpring ChooseSpring(const FSpiderLurkSense& Sense, ESpiderPerchKind Kind, float Scale,
		const FSpiderLurkTuning& Tuning);
	/** Whether a leap reaches a player Distance off (3D) and Rise above its perch's foot, at its size. */
	AI_LOOTER_SHOOTER_API bool CanLeap(float Distance, float Rise, bool bSees, float Scale, const FSpiderLurkTuning& Tuning);

	/** Where a leap from From lands (the feet: on the player's ground), aimed a little ahead of a moving player and short of them. */
	AI_LOOTER_SHOOTER_API FVector LeapLanding(const FVector& From, const FVector& PreyFeet, const FVector& PreyVelocity, float Scale,
		const FSpiderLurkTuning& Tuning);
	/** How long a leap from From to To flies (s): longer for a longer leap. */
	AI_LOOTER_SHOOTER_API float LeapSeconds(const FVector& From, const FVector& To);
	/** The launch that carries a body from From to To in Seconds under GravityZ (cm/s/s, below 0). */
	AI_LOOTER_SHOOTER_API FVector LeapVelocity(const FVector& From, const FVector& To, float Seconds, float GravityZ);
	/** Where that launch has it Time into its flight. */
	AI_LOOTER_SHOOTER_API FVector LeapPositionAt(const FVector& From, const FVector& Velocity, float Time, float GravityZ);
	/** Whether a landing at Landed (feet) bites a player at PreyFeet. */
	AI_LOOTER_SHOOTER_API bool LandingBites(const FVector& Landed, const FVector& PreyFeet, float Scale, const FSpiderLurkTuning& Tuning);
	/** A mate's call's delay (s) for a dice roll Dice01. */
	AI_LOOTER_SHOOTER_API float MateDelay(float Dice01, const FSpiderLurkTuning& Tuning);
	/** Climbing down: low enough over the ground (BodyOver, cm) to hop off. */
	AI_LOOTER_SHOOTER_API bool ClimbDone(float BodyOver, float Scale, const FSpiderLurkTuning& Tuning);
}
