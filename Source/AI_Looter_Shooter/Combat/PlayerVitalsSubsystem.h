#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "PlayerVitalsSubsystem.generated.h"

class APlayerController;

/**
 * Player-side consequences of taking damage: a red flash when hurt, and on death a fade-out, then a respawn with full
 * health at the open respawn grave nearest where they fell (ARespawnMarker), or else at the level's own start, never at
 * a trip's landing. Works for any player character with a HealthComponent.
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

private:
	struct FPlayerVitals
	{
		float LastHealth = -1.f;
		bool bDying = false;
		float DeathTime = 0.f;
		/** Where they fell: they wake at the open grave nearest it. */
		FVector DeathLocation = FVector::ZeroVector;
	};

	void Respawn(APlayerController* PC, FPlayerVitals& Vitals);

	TMap<TWeakObjectPtr<APlayerController>, FPlayerVitals> Players;
};
