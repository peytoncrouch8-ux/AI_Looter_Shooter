#pragma once

#include "CoreMinimal.h"
#include "Combat/HitReaction.h"

class ACreatureBase;
class UPrimitiveComponent;

/**
 * Which part of a creature a shot's bone is (HITS layer 2: ECreatureHitZone), by its kind:
 *  - Spiders (the Brown Spider, the Spitter, the Gravemother): head and fangs Head, body Body, abdomen Abdomen, the sac
 *    Weak, femur/tibia/foot Leg (SpiderLegOf says which leg).
 *  - The Unpaid and its kin (the dead miner, the Bell-Ringer, Abel, the Foreman): head, neck, jaw and hat Head; the waist,
 *    chest and coal Body (Core on a crit: the coal shot); pelvis Body; the arms down to the fingers Arm; the shroud and the
 *    strips Shroud.
 *  - The slime: Core on a crit (a shot through the gel onto its core) or on the core bone, else Body.
 *  - Anything else: a "head" bone Head, the rest Body. No bone: Body.
 */
namespace CreatureHitZones
{
	AI_LOOTER_SHOOTER_API ECreatureHitZone ZoneOf(const ACreatureBase& Creature, FName Bone, bool bCritical);

	/**
	 * As ZoneOf, with the part the shot hit: a hit zone of the creature's own besides its body and capsule (a miner's lamp, the
	 * Bell-Ringer's bell) is Weak; anything else goes by Bone.
	 */
	AI_LOOTER_SHOOTER_API ECreatureHitZone ZoneOfHit(const ACreatureBase& Creature, const UPrimitiveComponent* Component, FName Bone,
		bool bCritical);

	/**
	 * The bone a shot counts on: its own when it has one; a shot that met the creature's collision capsule (or its mesh with
	 * no bone) the mesh's bone nearest Location, so every real shot gets a zone (the 2026-10-29 play-check: "body hit (bone
	 * None)" for shots on an Unpaid's capsule). Left None for a hit with no part (a status's tick, a blast), for a part of its
	 * own (a lamp, a bell: the part is the zone) and for the slime (one gel; a bone near the core is no shot onto it).
	 */
	AI_LOOTER_SHOOTER_API FName BoneOfHit(const ACreatureBase& Creature, const UPrimitiveComponent* Component, FName Bone,
		const FVector& Location);

	/**
	 * Which spider leg Bone belongs to (femur_<pair>_<side>, tibia_..., foot_...): pair x 2 + (1 on the right), the same order
	 * as ASpiderCreature's Legs (front left 0, front right 1 ... back right 7); -1 for any other bone.
	 */
	AI_LOOTER_SHOOTER_API int32 SpiderLegOf(FName Bone);
}
