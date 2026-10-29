#pragma once

#include "CoreMinimal.h"
#include "Player/FirstPersonHandsTypes.h"
#include "Weapons/WeaponHandMarks.h"

/**
 * The hands on the grip spots a gun's parts mark (the user's pick, 2026-10-29: grip spots on the parts, instead of tuning
 * each gun's hands by hand; Docs/Work/2026-10-29-GripSpots/CONTRACT.md). A marker gives the hand's place and turn on the
 * part, its pose (Player/FirstPersonHandPoses.h) the fingers and the handle they close on. FirstPersonHandsGrips' OnGrip,
 * OnForegrip and OnCup, NextPumpShare and the reloads' magazine and rocket ask here first and keep the kind's tables for
 * a gun (or a role) not marked yet.
 *
 * The frame (camera space, through FHandsGun::ToCamera): the knuckles along the marker's +X, the palm facing its -Z (into
 * the part); the grip point FirstPersonHands::GripOffPalmSkin x HandScale off the marker along the palm's facing (the
 * marker is the palm's skin; hand-sized, never the gun's). The handle the fingers close on is a rounded box with the face
 * under the palm at the marker, endless along its axis, sized by the pose's HandleHalfAcross / HalfThrough / Corner as the
 * tables' FHandleSpec reads them (cm of the modeled gun, drawn to scale with it): a palm on a handle's side (it faces across
 * the gun: a pistol grip, a magazine) lies on its width, the handle across the palm (the marker's Y), front to back along the
 * knuckles; a palm under or over it (a fore-end, a pump, a cup) lies on its height, the handle along the gun.
 */
namespace FirstPersonHandsMarks
{
	/** Looter.Hands.Marks (1): the hands go to the parts' marks where a gun has them. 0 forces the kinds' tables (before/after). */
	AI_LOOTER_SHOOTER_API bool UseMarks();

	/** The gun's mark for Role on Hand's side (null when it has none, or Looter.Hands.Marks is 0). */
	AI_LOOTER_SHOOTER_API const FHandMark* Find(const FHandsGun& Gun, EHandMarkRole Role, EFirstPersonHand Hand);

	/**
	 * Hand on Mark (camera space): turned from the marker, the grip point off it by the palm's skin, the fingers from its
	 * pose (today's grip curls for a pose the library doesn't have), the forearm left to the framing (zero) unless the pose
	 * keeps it straight back behind the knuckles (the cup's rule), the contact closing on the pose's handle by its touch
	 * share. The trigger hand on a pistol hold: its index finger on the trigger (SOCKET_Trigger, else the kind's table) as
	 * far as Gun.TriggerReady, else laid along the frame.
	 */
	AI_LOOTER_SHOOTER_API FHandPlacement Placement(const FHandsGun& Gun, const FHandMark& Mark, EFirstPersonHand Hand);

	/**
	 * Find and Placement together: false (Out untouched) when the gun has no such mark. Slide (cm of the modeled gun, the
	 * gun's own space) moves the mark with its part's travel: a racked pump back along -X, a magazine down along -Z.
	 */
	AI_LOOTER_SHOOTER_API bool Place(const FHandsGun& Gun, EHandMarkRole Role, EFirstPersonHand Hand, FHandPlacement& Out,
		const FVector& Slide = FVector::ZeroVector);

	/** The support hand's rest hold off Hold_L (false: none, or a cup while Looter.Hands.Cup is 0: the tables' fore-end then). */
	AI_LOOTER_SHOOTER_API bool SupportRest(const FHandsGun& Gun, FHandPlacement& Out);

	/**
	 * The support hand at rest cups, keeping its own forearm (UFirstPersonHandsComponent's cupped test): Hold_L's pose when
	 * marked (a cup, with Looter.Hands.Cup on), else the kind's table (FGunHands::bCup) with Looter.Hands.Cup on.
	 */
	AI_LOOTER_SHOOTER_API bool SupportCups(const FHandsGun& Gun);

	/** A pose that cups (its forearm straight back behind the knuckles): the library's say, else the name "Cup". */
	AI_LOOTER_SHOOTER_API bool PoseCups(FName Pose);

	/** Where the trigger finger goes (the modeled gun's space, as FGunHands::TriggerAt): SOCKET_Trigger when marked, else TableAt. */
	AI_LOOTER_SHOOTER_API FVector TriggerAt(const FHandsGun& Gun, const FVector& TableAt);
}
