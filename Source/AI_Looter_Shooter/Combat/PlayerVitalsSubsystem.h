#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "PlayerVitalsSubsystem.generated.h"

class APlayerController;

/**
 * Player-side consequences of taking damage: a red flash when hurt, and on death a fade-out, then a
 * respawn at the player start with full health. Works for any player character with a HealthComponent.
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
	};

	void Respawn(APlayerController* PC, FPlayerVitals& Vitals);

	TMap<TWeakObjectPtr<APlayerController>, FPlayerVitals> Players;
};
