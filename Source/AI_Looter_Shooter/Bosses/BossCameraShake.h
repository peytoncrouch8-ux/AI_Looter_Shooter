#pragma once

#include "CoreMinimal.h"
#include "Player/CameraShakeModifier.h"

/**
 * A camera shake for a boss fight's big moments (its start, a phase, a slam, a stagger, its death): a shake on the one
 * camera modifier that carries every shake and kick of the player's view (UCameraShakeModifier::AddShake: smooth noise,
 * about 1.6 degrees and 4 cm at full strength, dying away over its seconds). The aim doesn't move, only the view. The
 * player's camera shake setting scales it with everything else (Looter.CameraShake sets it for a run).
 */
namespace BossCameraShake
{
	/**
	 * Shakes the view of every local player near Source: Strength (0-1) at full within a fifth of Radius, falling off to
	 * nothing at Radius (cm), dying away over Seconds. Nothing happens outside a game world (a test level).
	 */
	AI_LOOTER_SHOOTER_API void Kick(const UObject* WorldContext, const FVector& Source, float Strength, float Seconds = 0.6f,
		float Radius = 4500.f);

	/** A kick of Strength as felt Distance away (cm) from it: full within a fifth of Radius, nothing at or past it. */
	AI_LOOTER_SHOOTER_API float Falloff(float Strength, float Distance, float Radius);
}
