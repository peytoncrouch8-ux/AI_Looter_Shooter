#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LooterMenuPlayerController.generated.h"

class ACameraActor;

/**
 * The main menu's player: no character, and a view that slowly circles the level's island (the actors tagged Ground,
 * as the minimap finds it) from out in the sky, the island set right of center to leave the menu room on the left.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ALooterMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ALooterMenuPlayerController();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** How fast the view circles the island: a full turn takes 360 / this seconds. */
	UPROPERTY(EditDefaultsOnly, Category = "Menu Camera")
	float DegreesPerSecond = 1.5f;

	/** Where the circle starts, in degrees around the island (0 north, 90 east): the waterfall's side. */
	UPROPERTY(EditDefaultsOnly, Category = "Menu Camera")
	float StartAngle = 50.f;

	/** How far out it circles, in island radii from its center. */
	UPROPERTY(EditDefaultsOnly, Category = "Menu Camera")
	float Distance = 2.2f;

	/** How high over the island's ground it flies, in island radii. */
	UPROPERTY(EditDefaultsOnly, Category = "Menu Camera")
	float Height = 0.3f;

	/** How far under the ground's top the view aims, in island radii, so its cliffs and underside show. */
	UPROPERTY(EditDefaultsOnly, Category = "Menu Camera")
	float AimDrop = 0.2f;

	/** Degrees the view turns left of the island, which then sits right of the menu. */
	UPROPERTY(EditDefaultsOnly, Category = "Menu Camera")
	float AimOffset = 14.f;

	UPROPERTY(EditDefaultsOnly, Category = "Menu Camera")
	float FieldOfView = 60.f;

private:
	/** Puts the camera at Angle on its circle, looking at the island. */
	void PlaceCamera();

	UPROPERTY(Transient)
	TObjectPtr<ACameraActor> OrbitCamera;

	FVector Center = FVector::ZeroVector;
	float Radius = 10000.f;
	float Top = 0.f;
	float Angle = 0.f;
};
