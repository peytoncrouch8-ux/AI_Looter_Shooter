#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "World/FaunaTypes.h"
#include "FaunaSubsystem.generated.h"

class AFaunaActor;
class APawn;
class AWeaponBase;
struct FHitResult;

/**
 * Runs a level's ambient life (AFaunaActor: flocks, swarms, tumbleweeds, dust devils, washing) for a tenth of a millisecond
 * or so. Once a frame it reads the view (the local player's camera), the threats (players, by how boldly they move, and
 * hostile creatures near the view) and the noises (the local player's gun firing, every bullet's strike, ReportNoise),
 * then updates each fauna actor only when it's due: every frame up close on screen, a few times a second off screen or
 * far (FaunaRules::UpdateInterval). Past an actor's cull distance, outside its lighting state or with Looter.Fauna 0 it's
 * hidden and left alone. Nothing of it collides with anything.
 *
 * Looter.Fauna 0/1 turns it all off and on (to measure it by difference); Looter.Fauna.Stats prints what's out and what
 * it costs, and Looter.Fauna.Scare startles everything near the player (World/FaunaDevCommands.cpp).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UFaunaSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static UFaunaSubsystem* Get(const UObject* WorldContextObject);

	/** Played worlds, and the editor preview worlds the automated tests build. */
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	void Register(AFaunaActor* Actor);
	void Unregister(AFaunaActor* Actor);

	/** Every fauna actor in the level that has begun play. */
	const TArray<TWeakObjectPtr<AFaunaActor>>& GetActors() const { return Actors; }

	/** A loud noise at Location carrying Radius (cm): animals that hear it flee (FaunaRules::Hears). */
	void ReportNoise(const FVector& Location, float Radius, EFaunaNoise Kind);

	/** The noises of the last NoiseMemory seconds. */
	const TArray<FFaunaNoise>& GetNoises() const { return Noises; }

	/** Whether ambient life is on (Looter.Fauna). */
	static bool IsEnabled();

	/** What a frame's updates cost on the game thread, averaged (ms), and how many actors were updated last frame. */
	float GetAverageMilliseconds() const { return AverageMilliseconds; }
	int32 GetUpdatedLastFrame() const { return UpdatedLastFrame; }

	/** Builds the frame's context (view, threats, noises, lighting) as Tick does. */
	FFaunaContext GatherContext() const;

	/** A gunshot carries this far (cm) as a noise: each animal hears it within its own gunfire radius. */
	static constexpr float GunshotRadius = 8000.f;

	/** A bullet's strike carries this far (cm). */
	static constexpr float ImpactRadius = 1500.f;

	/** How long noises are kept (s): an actor updating four times a second off screen still hears every one. */
	static constexpr float NoiseMemory = 2.f;

	/** Hostile creatures count as threats only this near the view (cm). */
	static constexpr float CreatureThreatRange = 12000.f;

private:
	/** Keeps the local player's gun bound (its OnFired), whichever is in hand. */
	void BindPlayerWeapon();

	UFUNCTION()
	void HandleWeaponFired();

	void HandleBulletHit(const FHitResult& Hit, float Damage, bool bCritical);

	/** Hides or shows every actor as the toggle changed. */
	void ApplyToggle(bool bEnabled);

	TArray<TWeakObjectPtr<AFaunaActor>> Actors;
	TArray<FFaunaNoise> Noises;

	TWeakObjectPtr<AWeaponBase> BoundWeapon;
	FDelegateHandle BulletHitHandle;

	/** The toggle as the last frame saw it. */
	bool bWasEnabled = true;

	/** Hostile creatures near the view, looked for a few times a second (not every frame). */
	TArray<TWeakObjectPtr<APawn>> NearbyCreatures;
	double CreatureScanTime = -1000.0;

	float AverageMilliseconds = 0.f;
	int32 UpdatedLastFrame = 0;
};
