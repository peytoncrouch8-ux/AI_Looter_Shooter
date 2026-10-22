#pragma once

#include "CoreMinimal.h"
#include "Elements/LooterElement.h"

class FWeaponFX;

/**
 * A creature killed by an element's status (a burn's piece, a smite, the salt) dies the element's way, not the gun's:
 *  - Ember: the body collapses in embers, a fountain of them and a curl of dark smoke from its foot.
 *  - Soul-light: it goes up in a burst of light, rays flung out and motes lifting away.
 *  - Grave-salt: it crystallises and crumbles, salt crystals falling off the whole body and flung out, a breath of frost.
 * CreatureKillLook::Play lays it over the kind's kill layers when the killing hit was a status's (FCreatureHit::Element);
 * UCreatureHitReactionComponent leaves such a corpse without the gun's knock and the kind's own burst. Everything through
 * the bullets' pooled FWeaponFX (the element looks it already has: SpawnElementAura, SpawnElementBurst), no new draws.
 */
namespace ElementDeathLook
{
	/** The body's crystals, embers or motes the death spawns on a body of this size (BodyScale: radius over 40 cm, 0.5-3): grows with the square root. */
	AI_LOOTER_SHOOTER_API int32 AuraCount(ELooterElement Element, float BodyScale);

	/** Past this many particles in the pool the death spawns half as many (the shots' own come first). */
	inline constexpr int32 ParticleBudget = 1400;

	/** Plays the element's death over a body (Center, Radius, HalfHeight: its cylinder) seen from Camera (zero: unknown). Nothing for None. */
	AI_LOOTER_SHOOTER_API void Play(FWeaponFX& Effects, ELooterElement Element, const FVector& Center, float Radius, float HalfHeight,
		const FVector& Camera);
}
