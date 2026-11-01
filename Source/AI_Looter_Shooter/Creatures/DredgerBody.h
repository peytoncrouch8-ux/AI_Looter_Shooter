#pragma once

#include "CoreMinimal.h"
#include "Player/Animation/LooterStanceInput.h"

struct FCompactPose;

/**
 * A Dredger's body for one frame, as the game thread hands it to its animation (ADredgerCreature::GetBodyInput ->
 * UDredgerBodyAnimInstance): how it moves (the mannequin's walk and jog clips by speed and direction), the rifle hold over
 * them (MF_Rifle_Idle_ADS on the upper body, MM_Rifle_Reload over it, then the player's own stance layer: crouch, the aim's
 * pitch, the left hand on the foregrip, the kick), and the code layer on top (a hit's flinch, a stagger, a peek's lean, the
 * hose-man's brace against the line, breath, the fall when no ragdoll takes it). Plain data: copied once a frame.
 */
struct FDredgerBodyInput
{
	/** Ground speed at full size (cm/s) and the way it moves from its facing (degrees, 0 ahead, 90 to its right). */
	float Speed = 0.f;
	float MoveAngle = 0.f;
	/** Far off (it updates less often): one clip a gait instead of a blend. */
	bool bCheap = false;

	/** The player's stance layer's input, filled for a Dredger (LooterStancePose::Apply runs on it; its ReloadWeight is the reload's). */
	FLooterStanceInput Stance;
	/** How far into the reload (0-1): the reload clip plays at that share of its length (the anim instance knows the clip). */
	float ReloadShare = 0.f;

	/** A hit's flinch (degrees: X forward lean, Y lean to its right, Z twist to its right) and its head's whip (X down, Y right). */
	FVector Flinch = FVector::ZeroVector;
	FVector2D HeadWhip = FVector2D::ZeroVector;
	/** How far into a stagger's pose (0-1), and its wound's hunch (0, 1 under half health, 2 under a quarter). */
	float Stagger = 0.f;
	float Wound = 0.f;
	/** A peek's lean out of cover (-1 its left .. 1 its right) and the hose-man's brace against the line (0-1) with its tug (degrees). */
	float Lean = 0.f;
	float Brace = 0.f;
	float Tug = 0.f;
	/** Its notice beat (0-1): it straightens, head up, as it sees its prey. */
	float Notice = 0.f;
	/** Its breath's clock (s), its own so a squad doesn't breathe in step. */
	float BreathTime = 0.f;
	/** The code fall (0 standing .. 1 lying), when no ragdoll takes the death; its way (1 backward, -1 forward). */
	float Fall = 0.f;
	float FallWay = 1.f;
};

/** The body's clips and the code layer (DredgerBody.cpp; the anim instance in DredgerBodyAnimInstance.cpp). */
namespace DredgerBody
{
	/** The mannequin's clips the body plays (the project's own: /Game/Characters/Mannequins/Anims). */
	inline constexpr const TCHAR* IdleClip = TEXT("/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle.MM_Idle");
	inline constexpr const TCHAR* UpperBodyClip = TEXT("/Game/Characters/Mannequins/Anims/Rifle/MF_Rifle_Idle_ADS.MF_Rifle_Idle_ADS");
	inline constexpr const TCHAR* ReloadClip = TEXT("/Game/Characters/Mannequins/Anims/Rifle/MM_Rifle_Reload.MM_Rifle_Reload");

	/** The eight ways round (0 ahead, then a step of 45 degrees clockwise: ahead-right, right, ...). */
	inline constexpr int32 Ways = 8;
	/** The walk and jog clip for way Index (0-7), as an object path. */
	FString WalkClip(int32 Index);
	FString JogClip(int32 Index);

	/** The two ways round a move's angle (degrees, any) and the second's share: 100 degrees is way 2 (right) and 22% of way 3. */
	AI_LOOTER_SHOOTER_API void WayBlend(float Angle, int32& OutA, int32& OutB, float& OutBShare);

	/** How a speed (cm/s, full size) splits between standing, walking and jogging (each 0-1, summing to 1). */
	AI_LOOTER_SHOOTER_API void GaitShares(float Speed, float WalkClipSpeed, float JogClipSpeed, float& OutIdle, float& OutWalk, float& OutJog);

	/** How fast the step cycle turns (cycles a second) at Speed, given each gait's clip length (s) and the jog's share. */
	AI_LOOTER_SHOOTER_API float CycleRate(float Speed, float WalkLength, float JogLength, float JogShare, float WalkClipSpeed, float JogClipSpeed);

	/** The code layer over the pose (component space work, written back to local): flinch, stagger, lean, brace, breath, the fall. */
	void ApplyCodeLayer(FCompactPose& Pose, const FDredgerBodyInput& In);
}
