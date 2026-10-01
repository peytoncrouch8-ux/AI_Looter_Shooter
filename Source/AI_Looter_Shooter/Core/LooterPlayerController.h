#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LooterPlayerController.generated.h"

class UInputMappingContext;

/** The local player's controller: turns on the always-on controls and sets how far the view can look up and down. */
UCLASS()
class AI_LOOTER_SHOOTER_API ALooterPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ALooterPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void SpawnPlayerCameraManager() override;

	/** Walking and jumping (IMC_Default) and mouse look (IMC_MouseLook). Weapon and character contexts come from their components. */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TArray<TObjectPtr<UInputMappingContext>> DefaultContexts;

	/** Lowest the view can look, in degrees. */
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float ViewPitchMin = -70.f;

	/** Highest the view can look, in degrees. */
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float ViewPitchMax = 80.f;
};
