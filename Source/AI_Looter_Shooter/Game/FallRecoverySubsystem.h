#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "FallRecoverySubsystem.generated.h"

/**
 * The world is a set of floating islands, so walking off an edge is easy. Instead of killing the player,
 * this remembers where each player last stood on solid ground and, after a long fall, fades them back there.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UFallRecoverySubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

	/** How far below the last safe spot a player may fall before being returned. */
	UPROPERTY(EditAnywhere, Category = "Fall Recovery")
	float RecoverDropHeight = 3000.f;

private:
	struct FSafeSpot
	{
		FVector Location = FVector::ZeroVector;
		FRotator Rotation = FRotator::ZeroRotator;
		bool bValid = false;
		float SampleTimer = 0.f;
	};

	TMap<TWeakObjectPtr<APawn>, FSafeSpot> SafeSpots;
};
