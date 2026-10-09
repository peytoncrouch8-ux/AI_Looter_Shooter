#pragma once

#include "CoreMinimal.h"
#include "Audio/AmbienceRules.h"
#include "Subsystems/WorldSubsystem.h"
#include "AmbienceSubsystem.generated.h"

class UAudioComponent;
struct FLightingStateChange;

/**
 * The level's air (Docs/Polish/BorderlandsComparison.md, item 1): the area's bed for the light it's in, crossfading as
 * the lighting state changes (ULightingStateSubsystem::OnChanged), and the sweeteners, single calls placed round the
 * listener at random distances and times, never the same twice running and each with its cooldown (AmbienceRules).
 * As the level begins it also gives the world's sound-making things that aren't placed as emitters their voices: the
 * chapel's bell its hum, the train its steam while it's warm (UAmbientEmitterComponent::AddTo).
 *
 * The bed is two 2D loops of different lengths (the air and the life) in the Ambience class, which follows the Effects
 * slider; they and the sweeteners pause with the game, as the world does. It runs only where the game's sounds play (a
 * game or play-in-editor world with audio), and ticks for the sweeteners' clock alone.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UAmbienceSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static UAmbienceSubsystem* Get(const UObject* WorldContext);

	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;

	ELooterAudioArea GetArea() const { return Area; }
	FName GetLight() const { return Light; }
	const FAmbienceBed& GetBed() const { return Bed; }

	/** Plays the next sweetener now, whatever the clock says (Looter.Ambience.Sweetener). */
	void PlaySweetenerNow();

	/** Lines describing what it's doing, for the console (Looter.Ambience). */
	FString Describe() const;

private:
	void HandleLightingChanged(const FLightingStateChange& Change);
	void SetLight(FName NewLight, float CrossfadeSeconds);
	void StartBed(const FAmbienceBed& NewBed, float FadeSeconds);
	UAudioComponent* StartLayer(FName Cue, float FadeSeconds);
	void PlaySweetener();
	void HookWorldEmitters(UWorld& InWorld);

	/** Lets a layer fade away and go (it destroys itself once quiet). */
	static void FadeAway(UAudioComponent* Layer, float Seconds);

	ELooterAudioArea Area = ELooterAudioArea::Unknown;
	FName Light;
	FAmbienceBed Bed;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> AirLayer;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> LifeLayer;

	bool bStarted = false;
	/** Seconds of play since the level began (it stops while the game is paused, as the sweeteners do). */
	double Clock = 0.0;
	double NextSweetener = 0.0;
	int32 LastSweetener = INDEX_NONE;
	/** When each of the light's sweeteners may play again (their cooldowns). */
	TArray<double> SweetenerReady;
	float LightCheck = 0.f;
	FDelegateHandle LightingHandle;
};
