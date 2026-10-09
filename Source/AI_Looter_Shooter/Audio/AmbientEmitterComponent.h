#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "AmbientEmitterComponent.generated.h"

class UAudioComponent;

/**
 * A place in the world that makes sound: a loop it plays while the listener is in earshot (a creek, a waterfall, the
 * windmill's fan) and one-shots now and then round it (the windmill's head creaking, Main Street's boards and shutters,
 * the Sink's webs). Put it on an actor (AWindmill has two), or place an AAmbientEmitter (Tools/Unreal/build_area_sound.py
 * does, from the layout), or add one in play (UAmbienceSubsystem gives the chapel's bell and the warm train theirs).
 *
 * Given a Path (world points: a creek's line, a street, a pond's shore with bClosedPath), the sound comes from the point
 * on it nearest the listener, so one voice sounds like the whole line. The loop starts (fading in) as the listener comes
 * into its reach and stops (fading out) past it, so far-off places cost nothing; its reach is its cue's attenuation's
 * unless AudibleRadius says. An owner can bend the loop's volume and pitch (SetModulation: the windmill's gusts) and a
 * runtime gate can hold it silent (PlaysWhile: the train while cold). It looks around four times a second; it's silent
 * wherever the game's sounds are (tests, servers).
 */
UCLASS(ClassGroup = (Looter), meta = (BlueprintSpawnableComponent))
class AI_LOOTER_SHOOTER_API UAmbientEmitterComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UAmbientEmitterComponent();

	/** The loop it plays while in earshot (LooterSoundCues.h; none: one-shots only). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambience")
	FName LoopCue;

	/** One-shots it plays now and then (one of them each time, never the same twice running). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambience")
	TArray<FName> OneShotCues;

	/** Seconds between one-shots: a random time from X to Y. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambience")
	FVector2D OneShotGap = FVector2D(6.0, 14.0);

	/** One-shots land this far (cm) round the source in any direction on the ground: a street's shutters either side. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambience", meta = (ClampMin = "0", Units = "cm"))
	float Scatter = 400.f;

	/** The line the sound comes from (world points; empty: the component's own place). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambience")
	TArray<FVector> Path;

	/** The path closes back on its first point (a pond's shore). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambience")
	bool bClosedPath = false;

	/** How far (cm) its sound reaches. 0: its loop's (or first one-shot's) attenuation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambience", meta = (ClampMin = "0", Units = "cm"))
	float AudibleRadius = 0.f;

	/** Its loudness against its cue's mix (a small pond a little quieter than the big one). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambience", meta = (ClampMin = "0"))
	float VolumeScale = 1.f;

	/** The loop's volume and pitch bent by its owner, smoothly (the windmill's fan with the gusts). */
	void SetModulation(float InVolume, float InPitch);

	/** A gate in play: while it's set and says no, everything stays silent (the train's steam while it's cold). */
	TFunction<bool()> PlaysWhile;

	/** Where its sound comes from for a listener at ListenerLocation: the nearest point of its path, or its own place. */
	FVector SourceFor(const FVector& ListenerLocation) const;

	/** The loop is playing now. */
	bool IsLoopOn() const { return bLoopOn; }

	/** An emitter added in play on Parent (at Socket): registered with Parent's actor, ticking, for LoopCue. */
	static UAmbientEmitterComponent* AddTo(USceneComponent* Parent, FName Socket, FName InLoopCue, float InVolumeScale = 1.f);

	/** Where the local player hears from (their camera, in either view); false with no local player. */
	static bool FindListener(const UObject* WorldContext, FVector& OutLocation);

	/** How fast the loop fades in and out as the listener comes and goes (s). */
	static constexpr float FadeSeconds = 1.5f;
	/** Past its reach by this share it stops; inside it starts: a margin so the edge doesn't flicker. */
	static constexpr float Hysteresis = 1.1f;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Its reach (cm): AudibleRadius, or the cue's attenuation's. */
	float ReachOf(FName Cue) const;
	void StartLoop(const FVector& At);
	void StopLoop();
	void PlayOneShot(const FVector& Around);

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> Loop;

	bool bLoopOn = false;
	float LoopGain = 1.f;
	float ModVolume = 1.f;
	float ModPitch = 1.f;
	float AppliedVolume = -1.f;
	float AppliedPitch = -1.f;
	float NextOneShot = 0.f;
	int32 LastOneShot = INDEX_NONE;
	mutable float CachedReach = -1.f;
};
