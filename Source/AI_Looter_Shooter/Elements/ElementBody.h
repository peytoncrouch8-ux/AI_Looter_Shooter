#pragma once

#include "CoreMinimal.h"
#include "ElementBody.generated.h"

/**
 * What a creature is made of, as the elements see it (the user's bodies, 2026-10-29 night; ElementRules' table): each kind
 * sets its own in its constructor (ACreatureBase::ElementBody), Blueprint children keep their parent's. Weak / resist /
 * immune:
 *   Flesh   Thunder, Rot, Cinder / - / -                    the living (Gravepickers, Bone Hounds, Coffin Rats, Knockers)
 *   Chitin  Cinder, Rot / Wardbreak / -                     spiders, the Spitter, the Gravemother
 *   Gel     Cinder, Thunder / Hallow, Rot / -               slimes
 *   Bone    Hallow / Thunder / -                            dead miners, the Foreman, the Bell-Ringer, Abel
 *   Coal    Hallow (Wardbreak on the coal, a crit) / Cinder, Thunder / -    the Unpaid
 *   Spirit  Wardbreak, Hallow / - / Cinder, Rot, Thunder    (wisps later; the Ethereal tier adds it on top of any body)
 *   Stone   Wardbreak, Thunder / Cinder / Rot               the Stone Sentinel
 *   Iron    Thunder, Rot / Cinder / Wardbreak               (the Tin Man, later)
 * Saved with nothing (a creature's body is its class's), but keep the order: Blueprints store it by number.
 */
UENUM(BlueprintType)
enum class EElementBody : uint8
{
	/** The living (was Other: anything the old table didn't name). */
	Flesh,
	Chitin,
	Gel,
	/** The risen dead but the Unpaid (was Undead). */
	Bone,
	/** The Unpaid: a coal burning in a shroud of the dead (was Unpaid). */
	Coal,
	Spirit,
	Stone,
	Iron
};
