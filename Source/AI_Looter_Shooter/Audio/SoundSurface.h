#pragma once

#include "CoreMinimal.h"

struct FHitResult;

/** What a foot or a body comes down on, for its sound. */
enum class ESoundSurface : uint8
{
	Dirt,
	Grass,
	Wood,
	Stone,
	Metal,
	Water,
};

/**
 * Telling a surface from what was hit. The project has no physical surface types, so the names say it: a
 * "Surface.<Name>" tag on the component or its actor first (Surface.Wood on a plank deck whose material says otherwise),
 * then the words in the physical material's, the material's and its parents' names (MI_WoodPlanks is wood,
 * MI_RockGranite_Ransom stone). Terrain (an actor tagged Ground, or a macro/terrain material) is grass on the flat and
 * dirt on a slope, since its grass and soil are painted from one macro map. Anything else is dirt.
 */
namespace SoundSurface
{
	/** The surface a trace or a sweep landed on. */
	AI_LOOTER_SHOOTER_API ESoundSurface Of(const FHitResult& Hit);

	/** The surface a name's words say (case doesn't matter), or false when none of them does. */
	AI_LOOTER_SHOOTER_API bool FromName(const FString& Name, ESoundSurface& OutSurface);

	/** The surface a "Surface.<Name>" tag names, or false for any other tag. */
	AI_LOOTER_SHOOTER_API bool FromTag(FName Tag, ESoundSurface& OutSurface);

	/** Terrain is grass up to this slope (its normal's Z: about 30 degrees) and dirt past it. */
	inline constexpr float GrassFloorZ = 0.866f;

	/** The player's footstep on a surface: metal rings like stone, and water until it has a splash of its own is dirt. */
	AI_LOOTER_SHOOTER_API FName FootstepCue(ESoundSurface Surface);
}
