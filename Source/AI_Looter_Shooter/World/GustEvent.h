#pragma once

#include "CoreMinimal.h"
#include "GustEvent.generated.h"

/**
 * A gust of wind as UGustSubsystem broadcasts it (OnGust, once as it begins): which way it blows, how hard and for how
 * long. Anything that swings or flaps listens (USwingComponent, the hanging signs, the washing); the foliage and the
 * chimney smoke read it through MPC_Wind instead. `#include "World/GustSubsystem.h"` is all a listener needs.
 */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FGustEvent
{
	GENERATED_BODY()

	/** The way the air moves (unit, level): downwind, from the prevailing wind's quarter (the way the smoke leans). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gust")
	FVector Direction = FVector::ForwardVector;

	/** How hard it blows at its height: 0.4 (a stir) to 1 (a real blast). Its push rises and falls over Duration. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gust", meta = (ClampMin = "0", ClampMax = "1"))
	float Strength = 0.f;

	/** How long it blows (s), 2 to 5. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gust", meta = (ClampMin = "0", Units = "s"))
	float Duration = 0.f;
};

/** A gust begins: told once, with the whole gust (its push over time is FGustClock::Shape of its age). */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnGust, const FGustEvent& /*Gust*/);
