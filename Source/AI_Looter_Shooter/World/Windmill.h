#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Windmill.generated.h"

class UStaticMeshComponent;

/**
 * A water-pump windmill whose fan turns in the wind. The tower and the fan are two Blender models: the fan's pivot is
 * its hub, and it hangs from the tower's Fan socket, facing along the tower's forward. The fan turns about its own
 * forward axis at a speed that swells and eases with gusts, and only while someone can see it.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AWindmill : public AActor
{
	GENERATED_BODY()

public:
	AWindmill();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Windmill")
	TObjectPtr<UStaticMeshComponent> Tower;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Windmill")
	TObjectPtr<UStaticMeshComponent> Fan;

	/** The tower socket the fan's hub sits on. */
	UPROPERTY(EditAnywhere, Category = "Windmill")
	FName FanSocket = TEXT("Fan");

	/** Average speed of the fan. */
	UPROPERTY(EditAnywhere, Category = "Windmill", meta = (ClampMin = "0"))
	float TurnsPerMinute = 18.f;

	/** How much gusts speed the fan up and slow it down, as a share of the average speed. */
	UPROPERTY(EditAnywhere, Category = "Windmill", meta = (ClampMin = "0", ClampMax = "1"))
	float Gustiness = 0.4f;

	/** About how long one gust lasts, in seconds. */
	UPROPERTY(EditAnywhere, Category = "Windmill", meta = (ClampMin = "0.1"))
	float GustSeconds = 6.f;

private:
	float Angle = 0.f;
	float WindTime = 0.f;
};
