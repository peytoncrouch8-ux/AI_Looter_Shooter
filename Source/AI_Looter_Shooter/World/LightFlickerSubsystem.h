#pragma once

#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "Math/RandomStream.h"
#include "Subsystems/WorldSubsystem.h"
#include "World/LightFlickerRules.h"
#include "LightFlickerSubsystem.generated.h"

class ULevel;
class ULightComponent;
class UMaterialInstanceDynamic;

/**
 * Lamplight that breathes (LightFlickerRules has the flame): every light tagged Flicker (the light component's tag, or
 * its actor's for all its lights) and every light added by code (AHouseLights' lamps, with their windows as glass) burns
 * like a flame while it's within LightFlickerRules::Radius of the view: a soft breath of a few percent, a rare gutter,
 * and a house lamp carried about the house now and then. Its glass (a material instance with a glow parameter) follows.
 *
 * Cheap, and nothing runs for lights far off: twice a second a timer looks for lights near the view (plain distances),
 * and only while one is near does the subsystem tick, writing those lights 30 times a second; a light going far is set
 * back to its own intensity. A level with no such lights has no timer at all. Whatever else sets a light (the lighting
 * states through AHouseLights, the story) wins: a light found at an intensity it wasn't given is taken to have that one
 * as its own from then on, and a hidden light is left alone (the Foreman's dark).
 *
 * Looter.Flicker 0/1 stills and restarts it (to measure by difference). Played worlds, and the editor preview worlds the
 * automated tests build (they call Look and Step, as a test world never ticks).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ULightFlickerSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static ULightFlickerSubsystem* Get(const UObject* WorldContextObject);

	/** The tag a light flickers by: on the light component, or on its actor (then every light of it). */
	static const FName FlickerTag;

	// --- UWorldSubsystem ---
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	// --- FTickableGameObject ---
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;

	/**
	 * Light flickers from now on. Glass (optional): an instance whose GlassParameter (a scalar: its glow) follows the
	 * flame, such as a house's windows. bCarried: a house lamp, carried off and back now and then. Adding a light already
	 * in gives it the glass and carry named now.
	 */
	void AddLight(ULightComponent* Light, UMaterialInstanceDynamic* Glass = nullptr, FName GlassParameter = NAME_None,
		bool bCarried = false);

	/** Light stops flickering and is set back to its own intensity (its glass to its own glow). */
	void RemoveLight(ULightComponent* Light);

	/** Every light tagged Flicker in Level joins (null: in every level of the world). Lights already in stay as they are. */
	void GatherTagged(const ULevel* Level = nullptr);

	/** One look from Eyes (the view's place): lights within Radius flicker, the rest rest (the timer's; tests call it). */
	void Look(const FVector& Eyes);

	/** Moves the near lights' flames on by DeltaSeconds and writes them when a step is due (the tick's; tests call it). */
	void Step(float DeltaSeconds);

	int32 NumLights() const;
	int32 NumNear() const { return NearCount; }
	bool IsFlickering(const ULightComponent* Light) const;

	/** The intensity Light burns about (its own), or -1 when it isn't in. */
	float GetOwnIntensity(const ULightComponent* Light) const;

	/** The glow its glass burns about, or -1 without glass. */
	float GetOwnGlow(const ULightComponent* Light) const;

	/** The flame of Light (null when it isn't in), for the tests. */
	const FFlameFlicker* GetFlame(const ULightComponent* Light) const;

	/** The dice, fixed for a test. */
	void SetRandomSeed(int32 Seed) { Random.Initialize(Seed); }

	/** Whether lamps flicker at all (Looter.Flicker). */
	static bool IsEnabled();

private:
	struct FFlickerLight
	{
		TWeakObjectPtr<ULightComponent> Light;
		TWeakObjectPtr<UMaterialInstanceDynamic> Glass;
		FName GlassParameter;
		/** Its own intensity and glow, and what was written last (anything else found there is somebody else's). */
		float Own = 0.f;
		float Written = 0.f;
		float OwnGlow = 0.f;
		float GlowWritten = 0.f;
		bool bNear = false;
		FFlameFlicker Flame;
	};

	FFlickerLight* Find(const ULightComponent* Light);
	const FFlickerLight* Find(const ULightComponent* Light) const;
	void UpdateTimer();
	void LookFromView();
	/** Reads what the light and its glass show now: what this didn't write is their own from now on. */
	void Adopt(FFlickerLight& Each) const;
	void Write(FFlickerLight& Each);
	/** Back to its own intensity and glow. */
	void Rest(FFlickerLight& Each);
	void HandleLevelAdded(ULevel* Level, UWorld* World);

	TArray<FFlickerLight> Lights;
	int32 NearCount = 0;
	float StepClock = 0.f;
	FRandomStream Random = FRandomStream(20261014);
	FTimerHandle LookTimer;
	FDelegateHandle LevelAddedHandle;
};
