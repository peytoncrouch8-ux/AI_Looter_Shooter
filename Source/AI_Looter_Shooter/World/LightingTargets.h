#pragma once

#include "CoreMinimal.h"

class ADirectionalLight;
class AExponentialHeightFog;
class ALightingStates;
class APostProcessVolume;
class ASkyAtmosphere;
class ASkyLight;
class ULevel;
struct FLightingState;

/**
 * The lights a lighting state drives in a level: its sun, sky light, sky atmosphere, height fog and post process
 * volume, as its ALightingStates names them, or the first of each kind in the level for any it leaves empty (the sun is
 * the directional light that lights the atmosphere). Sets a state on them and reads what they show. The sky light's
 * recapture and the material parameter collection are ULightingStateSubsystem's: they belong to a running world.
 */
struct AI_LOOTER_SHOOTER_API FLightingTargets
{
	ADirectionalLight* Sun = nullptr;
	ASkyLight* SkyLight = nullptr;
	ASkyAtmosphere* Atmosphere = nullptr;
	AExponentialHeightFog* HeightFog = nullptr;
	APostProcessVolume* PostVolume = nullptr;

	/** The lights States names, the rest found by kind in Level (States' own level when Level is null). */
	static FLightingTargets Find(const ULevel* Level, const ALightingStates* States);

	/** What's missing, for a warning ("sun, height fog"), or empty when everything is there. */
	FString DescribeMissing() const;

	/**
	 * Turns the sun and sets its light and shadows, the sky light's intensity, the sky's color and ozone, the fog's
	 * density and colors, the exposure.
	 */
	void Write(const FLightingState& State) const;

	/**
	 * What the lights show now, into the fields Write sets; the rest of Out (its name, the tints) is left as it is.
	 * Reads saved levels too: the sun's turn is its own (relative) rotation, which is the world's for a light standing
	 * on its own.
	 */
	void Read(FLightingState& Out) const;
};
