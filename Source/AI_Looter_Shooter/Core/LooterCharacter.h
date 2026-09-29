#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "LooterCharacter.generated.h"

class UInputAction;
struct FInputActionValue;

/**
 * The player's character: walking, looking and jumping. Everything else lives in components (view, locomotion,
 * health, weapons). The Blueprint child, BP_LooterCharacter, holds only data: meshes, animation, camera placement and
 * the component settings.
 */
UCLASS(Abstract)
class AI_LOOTER_SHOOTER_API ALooterCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ALooterCharacter();

protected:
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	/** Gamepad look (right stick). */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MouseLookAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> JumpAction;

private:
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
};
