#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "LooterCharacter.generated.h"

class UInputAction;
class UInteractionComponent;
struct FInputActionValue;

/**
 * The player's character: walking, looking and jumping. Everything else lives in components (view, locomotion,
 * health, weapons, interaction). The Blueprint child, BP_LooterCharacter, holds only data: meshes, animation, camera
 * placement and the component settings. The whole character is the full-size mannequin scaled down
 * (Player/PlayerSize.h), and walks at the full-size speed scaled the same.
 */
UCLASS(Abstract)
class AI_LOOTER_SHOOTER_API ALooterCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ALooterCharacter();

	/** The standing half height at the player's size (the engine's version misses the capsule's scale). */
	virtual float GetDefaultHalfHeight() const override;

protected:
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	/** The Interact key and everything it uses: loot, doors, the bell, lantern posts (made here, not in the Blueprint). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInteractionComponent> Interaction;

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
	/** Jumps, or stands a crouched or sliding player up (UPlayerLocomotionComponent::HandleJumpPressed). */
	void JumpPressed();
};
