#pragma once

#include "CoreMinimal.h"
#include "Creatures/PlaceholderCreatureDefinition.h"

/** The clay body's pose this frame, against its rest (mesh space at size 1, cm and degrees). */
struct FPlaceholderPose
{
	/** Height scale (1 at rest); the sides follow to keep its volume (PlaceholderRules::SquashScale). */
	float Squash = 1.f;
	/** Up off its feet (a flyer's hover, a notice hop) or down into the ground (rising in, sinking dead). */
	float Lift = 0.f;
	/** Forward (a strike's lunge) or back (a shot's recoil). */
	float Lunge = 0.f;
	/** Leaning back (negative) or forward, and side to side. */
	float LeanPitch = 0.f;
	float LeanRoll = 0.f;
};

/** What its pose is worked out from (APlaceholderCreature fills it every frame it's posed). */
struct FPlaceholderPoseInput
{
	EPlaceholderRole Role = EPlaceholderRole::Melee;
	/** Its body's size (cm at size 1) and a flyer's hover. */
	float Length = 90.f;
	float Height = 80.f;
	float Hover = 0.f;
	/** Its own clock (s), and how fast it moves against its chase (0 to 1). */
	float Clock = 0.f;
	float Speed01 = 0.f;
	/** In an attack: its time in it, its wind-up and recovery, and whether it's a shot (a ranged kind's, not its bite). */
	bool bAttacking = false;
	float AttackTime = 0.f;
	float Windup = 0.6f;
	float Recovery = 0.5f;
	bool bShot = false;
	/** Its notice beat's strength now (0 to 1), and how much of its entrance is left (1 just begun, 0 out). */
	float Notice = 0.f;
	float EntranceLeft = 0.f;
	/** Dead: seconds since, and how long it lies before it sinks. */
	bool bDead = false;
	float DeadTime = 0.f;
	float CorpseTime = 6.f;
};

/** What a ranged placeholder does about its distance inside its brain's hold (APlaceholderCreatureMoves.cpp). */
enum class EPlaceholderKeep : uint8
{
	/** Outside its hold, or not hunting: its brain moves it. */
	None,
	/** Too close: backing away, facing its target. */
	BackOff,
	/** In its band with a fight spot to walk round to. */
	ToSpot,
	/** In its band, at its spot (or with none): standing, facing its target. */
	Hold,
};

/**
 * The placeholder creature's rules as plain functions (Looter.Creatures.Placeholder.Rules drives them): its name on the
 * tag, the clay blob stretched to a kind's size, its capsule and head, its pose by role, a shot's aim, its distance keeping.
 * The blob is SK_Slime's dome (Art/Models/Creatures/Slime.py: about 156 cm across its foot, 60 cm tall, origin on the
 * ground under its middle), stretched per axis.
 */
namespace PlaceholderRules
{
	/** SK_Slime's dome at rest: across its foot and its height (cm). */
	constexpr float BlobFoot = 156.f;
	constexpr float BlobHeight = 60.f;

	/** "Coffin Rat (rat)"; the name alone with no type, the type alone with no name. */
	AI_LOOTER_SHOOTER_API FText TagName(const FText& Name, const FText& PlainType);

	/** The mesh's scale that stretches the dome to Length x Width x Height (cm). */
	AI_LOOTER_SHOOTER_API FVector BodyStretch(float Length, float Width, float Height);

	/** Its movement capsule for that body: a radius under its narrower side, and half its height (at least the radius). */
	AI_LOOTER_SHOOTER_API void CapsuleFor(float Length, float Width, float Height, float& OutRadius, float& OutHalfHeight);

	/** Its head's middle from the ground under its middle (cm, at size 1): HeadHeight of its height up, HeadForward of its half-length on. */
	AI_LOOTER_SHOOTER_API FVector HeadSpot(float Length, float Height, float HeadHeight, float HeadForward);

	/** The body's scale for a height scale of Z, keeping its volume (as the slime's squash). */
	AI_LOOTER_SHOOTER_API FVector SquashScale(float Z);

	/**
	 * Its pose now. Idle it breathes; moving it bobs and sways; its wind-up is the tell (a melee kind squashes down and
	 * leans back, a shot rears up, a flyer drops toward the ground), its strike stretches and lunges (a shot recoils), and
	 * it eases back over its recovery. Dead it slumps flat and, after its corpse time, sinks. Rising in, it comes up out of
	 * the ground.
	 */
	AI_LOOTER_SHOOTER_API FPlaceholderPose PoseAt(const FPlaceholderPoseInput& In);

	/** Where a shot from From aims at a target at TargetAt moving at TargetVelocity: led by LeadShare of its flight. */
	AI_LOOTER_SHOOTER_API FVector LeadAim(const FVector& From, const FVector& TargetAt, const FVector& TargetVelocity, float ShotSpeed,
		float LeadShare);

	/** A ranged kind's keep at Distance from its target (its hold is where its brain stops closing in). */
	AI_LOOTER_SHOOTER_API EPlaceholderKeep ChooseKeep(float Distance, float BackOff, float Hold, bool bHasSpot, float SpotGap, float Arrive);

	/** A ranged kind's attack range: its brain's hold (three quarters of it) lands on CloseInTo, as the Venom Spitter's. */
	inline float RangedAttackRange(float CloseInTo) { return CloseInTo / 0.75f; }

	/** A ranged kind bites what comes this near (cm, at size 1). */
	constexpr float BiteReach = 230.f;
}
