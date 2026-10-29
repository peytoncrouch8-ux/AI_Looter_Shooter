#pragma once

#include "CoreMinimal.h"

// The hands' grip spots marked on a gun's parts (the user's pick, 2026-10-29: "grip spots on the parts";
// Docs/Work/2026-10-29-GripSpots/CONTRACT.md): empties placed in Blender on each held part become static mesh sockets, and
// the first-person hands reach to them (Player/FirstPersonHandsMarks.cpp) instead of each gun kind's hand-tuned tables.
//
// A marker sits on the part's skin under the palm's grip point. Its +X is the knuckles' way (where the fingers point), its
// +Z points out of the part's surface toward the palm (the palm looks along -Z into the part). Its Pose names a hand pose
// from the pose library (Player/FirstPersonHandPoses.h). Palm-skin offsets are the hand's size and are never in the marker.

/** What a marker is for: the resting hold, a pump's or a bolt's work, or where the support hand takes the reload's part. */
enum class EHandMarkRole : uint8
{
	Hold,
	Rack,
	Reload
};

/**
 * One marker, as UWeaponModelComponent::GetHandMarks reads it: in the gun model's own space with the model component's scale
 * divided out (as GetGrip's point is: the parts' ScaleFor is in it), the marker's own turn kept, no scale. A marker on the
 * gun's reload part is given where that part rests (its reload travel taken out: the hands put their own travel on, from
 * the same LooterReload curves that move the part).
 */
struct FHandMark
{
	bool bValid = false;
	FTransform Local = FTransform::Identity;
	FName Pose;
};

/** Every marker a gun's parts carry. Each role's two sides: index 0 the support (left) hand, 1 the trigger (right) hand (as FirstPersonHands::Index). */
struct FHandMarks
{
	FHandMark Hold[2];
	FHandMark Rack[2];
	FHandMark Reload[2];
	/** SOCKET_Trigger: the trigger's face, where the trigger finger's pad rests (location only). */
	bool bTrigger = false;
	FVector Trigger = FVector::ZeroVector;

	const FHandMark& Get(EHandMarkRole Role, int32 Side) const
	{
		const int32 Index = FMath::Clamp(Side, 0, 1);
		return Role == EHandMarkRole::Rack ? Rack[Index] : (Role == EHandMarkRole::Reload ? Reload[Index] : Hold[Index]);
	}

	FHandMark& Get(EHandMarkRole Role, int32 Side)
	{
		const int32 Index = FMath::Clamp(Side, 0, 1);
		return Role == EHandMarkRole::Rack ? Rack[Index] : (Role == EHandMarkRole::Reload ? Reload[Index] : Hold[Index]);
	}

	/** Any marker at all (a gun whose parts aren't marked yet holds by the tables). */
	bool HasAny() const
	{
		return bTrigger || Hold[0].bValid || Hold[1].bValid || Rack[0].bValid || Rack[1].bValid || Reload[0].bValid || Reload[1].bValid;
	}

	/** Every point scaled about the gun's origin (the first-person size, as the hands' sockets are: FHandsGun). */
	void ScalePoints(float Scale)
	{
		for (FHandMark* Mark : { &Hold[0], &Hold[1], &Rack[0], &Rack[1], &Reload[0], &Reload[1] })
		{
			Mark->Local.SetTranslation(Mark->Local.GetTranslation() * Scale);
		}
		Trigger *= Scale;
	}
};

namespace WeaponHandMarks
{
	/**
	 * Reads a socket's name as a hand marker: <Role>_<Side>_<Pose>, with or without the SOCKET_ prefix (Blender's empty is
	 * SOCKET_Hold_L_Cup; the export drops the prefix, so the mesh's socket is Hold_L_Cup). Role is Hold, Rack or Reload,
	 * Side L (support: 0) or R (trigger: 1), both any case; Pose one word of letters and digits starting with a letter, kept
	 * as written. Anything else (another socket, a missing or extra part, an unknown role or side) is not a marker: false.
	 */
	AI_LOOTER_SHOOTER_API bool ParseSocket(FName Socket, EHandMarkRole& OutRole, int32& OutSide, FName& OutPose);

	/** Trigger or SOCKET_Trigger (any case): the trigger's face. */
	AI_LOOTER_SHOOTER_API bool IsTriggerSocket(FName Socket);
}
