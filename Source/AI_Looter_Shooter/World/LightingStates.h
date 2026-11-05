#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Info.h"
#include "UObject/SoftObjectPtr.h"
#include "World/LightingState.h"
#include "LightingStates.generated.h"

class ADirectionalLight;
class AExponentialHeightFog;
class APostProcessVolume;
class ASkyAtmosphere;
class ASkyLight;
class UMaterialParameterCollection;

/**
 * A level's lighting states, as data: placed beside its lights by Tools/Unreal/build_area_environment.py from layout.json
 * level.environment. Day is the light the level is built in (its sun, sky light, fog and post volume are placed that
 * way); the layout adds others, such as Ransom's Rest's Dusk for the cold open and the boss. ULightingStateSubsystem
 * switches between them, and Looter.Light from the console. A level without one keeps its light as placed.
 *
 * From a build script (Python):
 *     states = actors.spawn_actor_from_class(unreal.LightingStates, unreal.Vector(0, 0, 0))
 *     states.set_editor_property('states', [unreal.LightingState(name='Day', ...), unreal.LightingState(name='Dusk', ...)])
 *     states.set_editor_property('sun', sun)    # and sky_light, atmosphere, height_fog, post_volume
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ALightingStates : public AInfo
{
	GENERATED_BODY()

public:
	ALightingStates();

	/** The state a level is built in. */
	static const FName DayState;

	/** Where the collection the states write lives (made by Tools/Unreal/build_world_materials.py). */
	static const TCHAR* DefaultCollectionPath;

	/** The level's states, Day first. */
	UPROPERTY(EditAnywhere, Category = "Lighting States")
	TArray<FLightingState> States;

	/** The state the level's lights are placed in, which the level starts in. */
	UPROPERTY(EditAnywhere, Category = "Lighting States")
	FName InitialState;

	/** The light the states turn. Left empty, it's the directional light that lights the atmosphere. */
	UPROPERTY(EditInstanceOnly, Category = "Lighting States|Lights")
	TObjectPtr<ADirectionalLight> Sun;

	/** Left empty, the level's first sky light. */
	UPROPERTY(EditInstanceOnly, Category = "Lighting States|Lights")
	TObjectPtr<ASkyLight> SkyLight;

	/** The sky whose color and ozone the states set. Left empty, the level's first sky atmosphere. */
	UPROPERTY(EditInstanceOnly, Category = "Lighting States|Lights")
	TObjectPtr<ASkyAtmosphere> Atmosphere;

	/** Left empty, the level's first height fog. */
	UPROPERTY(EditInstanceOnly, Category = "Lighting States|Lights")
	TObjectPtr<AExponentialHeightFog> HeightFog;

	/** The volume whose exposure the states set. Left empty, the level's first unbound post process volume. */
	UPROPERTY(EditInstanceOnly, Category = "Lighting States|Lights")
	TObjectPtr<APostProcessVolume> PostVolume;

	/**
	 * The material parameter collection the tints and the fog's colors go through, for the materials that read them
	 * (MPC_Lighting by default; cleared, the states write none).
	 */
	UPROPERTY(EditAnywhere, Category = "Lighting States|Lights")
	TSoftObjectPtr<UMaterialParameterCollection> Collection;

	/** The level's states actor: the first with any states, or null when the level keeps its light as placed. */
	static ALightingStates* Find(const UWorld* World);

	/** A state by name (ignoring case), or null. */
	const FLightingState* FindState(FName StateName) const;

	/** The state the level starts in: InitialState's, or the first when that names none. */
	const FLightingState* GetInitialState() const;

	/** The states' names, in order. */
	TArray<FName> GetStateNames() const;
};

/**
 * How lit a town's lamps and windows are in a lighting state, one rule for everything that lights the town for the
 * states (AHouseLights, UWindowLifeSubsystem, AmbientAirRules; ATownFire): add a new state in LightingStates::Kind and
 * nowhere else. Plain names, no world, so the tests ask it directly.
 */
enum class ELightKind : uint8
{
	/** Day, and a level without states: street lamps and porch lanterns out, a glimmer in the halls, windows at day glow. */
	Day,
	/** Smog, SmogThin (Mains 1-6 and after): the town lights its lamps against the smog, dim, the windows warm. */
	Gloom,
	/** Dusk, DuskSmog (the cold open, Mains 6-7): lamps and windows at their dusk values. */
	Dusk,
};

namespace LightingStates
{
	/** The kind of light a state (by name, ignoring case) gives the town; None and a state it doesn't know are Day. */
	AI_LOOTER_SHOOTER_API ELightKind Kind(FName State);

	/** "Day", "Gloom" or "Dusk", for logs and test messages. */
	AI_LOOTER_SHOOTER_API const TCHAR* KindName(ELightKind Kind);
}
