#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "World/LightingState.h"
#include "LightingStateSubsystem.generated.h"

class ALightingStates;
class UMaterialParameterCollection;
struct FLightingTargets;

/** How a switch to another lighting state looks. */
enum class ELightingSwitch : uint8
{
	/** Behind the camera's fade: out to black, the new light and the sky light's recapture, back in. */
	Fade,
	/** At once, for a caller that has covered the screen already (a scene's cut, the cloud's whiteout). */
	Instant,
	/** Eased over seconds while the player watches, the sky light recaptured every so often on the way. */
	Blend,
};

/** What OnChanged tells: the state the level left and the one now in place, and how it came. */
struct FLightingStateChange
{
	FName From;
	FName To;
	ELightingSwitch How = ELightingSwitch::Fade;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnLightingStateChanged, const FLightingStateChange& /*Change*/);

/**
 * Switches the level between its lighting states (ALightingStates): the sun's turn, light and shadows, the sky light, the
 * fog and the exposure on the level's lights (FLightingTargets), and the backdrop's and clouds' tints and the fog's
 * colors in the material parameter collection MPC_Lighting. Dusk is the same sun turned, so a switch costs nothing but
 * the sky light's recapture, which happens behind the camera's fade (or the caller's cover) so it's never seen. Nothing
 * runs between switches: it ticks only while one is underway.
 *
 * The level starts in its initial state (Day) as placed; only the collection is written then, so the materials reading
 * it match the level from the first frame. Looter.Light switches from the console; the cold open and the boss (Main 6)
 * will set Dusk, and anything that cares hears it through OnChanged.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ULightingStateSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static ULightingStateSubsystem* Get(const UObject* WorldContextObject);

	/** The collection's parameters the states write (MPC_Lighting; Tools/Unreal/lighting_collection.py makes them). */
	static const FName BackdropTintParameter;
	static const FName CloudTintParameter;
	static const FName FogInscatteringParameter;
	static const FName FogDirectionalParameter;

	/** Played worlds, and the editor preview worlds the automated tests build. */
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;

	/**
	 * Switches the level to a state by name (ignoring case). False, and nothing changes, when the level has no such state
	 * (or no states at all). Asking for the state already in place does nothing. A switch asked for during another takes
	 * over from wherever that one got to. A fade with nobody to fade for (no local player) switches at once.
	 */
	bool SetState(FName StateName, ELightingSwitch How = ELightingSwitch::Fade, float BlendSeconds = 0.f);

	/** The state in place: the level's initial one until a switch lands. None when the level has no states. */
	FName GetState() const;

	/** The state a switch underway is heading for, or None. */
	FName GetPendingState() const;

	bool IsSwitching() const { return Phase != EPhase::Idle; }

	/** The level's states' names, in order (empty when it has none). */
	TArray<FName> GetStateNames() const;

	/** Moves a switch on by DeltaSeconds: the tick calls it, and so can tests. */
	void Update(float DeltaSeconds);

	/** Fires as each new state is in place: behind the fade, at once, or at the end of a blend. */
	FOnLightingStateChanged OnChanged;

	/** How long the screen takes to fade to black before a switch, and back in after (seconds). */
	UPROPERTY(EditAnywhere, Category = "Lighting States")
	float FadeOutSeconds = 0.35f;

	UPROPERTY(EditAnywhere, Category = "Lighting States")
	float FadeInSeconds = 0.6f;

	/** How often a blend recaptures the sky light on its way (seconds): each is a small hitch, and the light's steps small. */
	UPROPERTY(EditAnywhere, Category = "Lighting States")
	float BlendRecaptureSeconds = 1.5f;

private:
	enum class EPhase : uint8
	{
		Idle,
		/** The screen going black; the new state lands when it is. */
		FadingOut,
		/** Black, with the new state in place and the sky light recaptured, a frame or two before fading back in. */
		Holding,
		Blending,
	};

	/** The level's states actor (found again if it went away), and what's in place, from the level's initial state. */
	ALightingStates* FindLevelStates();

	/** The lights the level's states drive. */
	FLightingTargets FindTargets();

	/** Sets a state on the lights and the collection; with bRecapture, the sky light captures the new sky this frame. */
	void Apply(const FLightingState& State, bool bRecapture);

	/** Writes the state's tints and fog colors to this world's instance of the collection. */
	void WriteCollection(const FLightingState& State);

	/** The new state is in place: logs what it set and tells OnChanged. */
	void Land();

	/** Starts every local player's screen fading to black; false when there's nobody to fade for. */
	bool FadeOut();

	/** Holds every local player's screen black (the new light lands on a covered screen whatever else faded it). */
	void HoldBlack();

	/** Fades every local player's screen back in. */
	void FadeIn();

	EPhase Phase = EPhase::Idle;
	ELightingSwitch SwitchHow = ELightingSwitch::Fade;

	/** The state in place (or None), and the values the lights have now: the start of a blend. */
	FName Current;
	FLightingState Applied;
	bool bKnowsLevel = false;

	/** Where a switch underway goes, and from where a blend set out. */
	FLightingState Target;
	FLightingState BlendFrom;
	float BlendDuration = 0.f;
	float Clock = 0.f;
	float SinceRecapture = 0.f;
	int32 HeldFrames = 0;

	/** The collection, loaded once (held here: the world's instance of it holds it only weakly). */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialParameterCollection> ParameterCollection;
	bool bCollectionLooked = false;
	bool bCollectionWarned = false;

	TWeakObjectPtr<ALightingStates> LevelStates;
};
