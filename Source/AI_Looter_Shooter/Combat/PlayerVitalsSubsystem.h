#pragma once

#include "CoreMinimal.h"
#include "Combat/RecoverySettings.h"
#include "Combat/WoundsClose.h"
#include "Subsystems/WorldSubsystem.h"
#include "PlayerVitalsSubsystem.generated.h"

class APlayerController;

/**
 * Player-side consequences of taking damage: a red flash when hurt, and on death a fade-out, then a respawn with full
 * health at the open respawn grave nearest where they fell (ARespawnMarker), or else at the level's own start, never at
 * a trip's landing. It also gives health back: the revenant's wounds close on their own after a while without being hurt
 * (FWoundsClose), by the numbers in Recovery. Works for any player character with a HealthComponent.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UPlayerVitalsSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

	/** Seconds from death to respawn. */
	UPROPERTY(EditAnywhere, Category = "Player Vitals")
	float RespawnDelay = 2.5f;

	/** Every number of how health comes back: the wounds that close and the soul-motes. */
	UPROPERTY(EditAnywhere, Category = "Player Vitals")
	FRecoverySettings Recovery;

	/**
	 * The recovery numbers in force in WorldContext's world: the subsystem's, or the defaults where there is none (a test
	 * level, the editor). Soul-motes and the drop roll read them here.
	 */
	static const FRecoverySettings& SettingsFor(const UObject* WorldContext);

	/** Whether PC's wounds are closing right now (past the wait, below full health, not held): the HUD shows it as it rises. */
	bool IsRegenerating(const APlayerController* PC) const;

private:
	struct FPlayerVitals
	{
		float LastHealth = -1.f;
		bool bDying = false;
		float DeathTime = 0.f;
		/** Where they fell: they wake at the open grave nearest it. */
		FVector DeathLocation = FVector::ZeroVector;
		/** The wait since the last hurt and the run of healing after it. */
		FWoundsClose Wounds;
	};

	void Respawn(APlayerController* PC, FPlayerVitals& Vitals);

	TMap<TWeakObjectPtr<APlayerController>, FPlayerVitals> Players;
};
