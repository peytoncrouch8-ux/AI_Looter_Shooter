#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "ControlSettingsSubsystem.generated.h"

class AController;

UCLASS()
class AI_LOOTER_SHOOTER_API ULooterControlsSave : public USaveGame
{
	GENERATED_BODY()

public:
	/**
	 * How far the view turns for a mouse move or stick push, as a multiple of the turn the game was made with. Saves
	 * from before it existed load with 1 (UControlSettingsSubsystem::DefaultLookSensitivity).
	 */
	UPROPERTY()
	float LookSensitivity = 1.f;

	/**
	 * How strongly the view kicks and shakes (shots, hits, kills, a boss's slam), 0 (still) to 1 (as designed). Saves from
	 * before it existed load with 1 (UControlSettingsSubsystem::DefaultCameraShake).
	 */
	UPROPERTY()
	float CameraShake = 1.f;
};

/**
 * The player's control options from the settings menu's Controls section, saved to the "ControlSettings" slot (the
 * keys keep their own, UKeyBindingSubsystem). Look sensitivity scales every turn of the view by mouse or stick: the
 * character's look (ALooterCharacter::Look, where a zoomed sight slows it further on top) and a scene's free look
 * (USceneSubsystem). Both read it as each input arrives, so a change applies from the next move. Camera shake scales
 * every shake and kick of the view, the boss fights' too: the one camera modifier that carries them all
 * (UCameraShakeModifier) reads it each frame, and the Looter.CameraShake command sets it for a run.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UControlSettingsSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/**
	 * Look sensitivity, from a tenth of the game's own turn (a long sweep of the mouse for a small turn) to three times
	 * it. 1 is the feel the game was made with. The settings slider moves in steps of LookSensitivityStep.
	 */
	static constexpr float MinLookSensitivity = 0.1f;
	static constexpr float MaxLookSensitivity = 3.f;
	static constexpr float DefaultLookSensitivity = 1.f;
	static constexpr float LookSensitivityStep = 0.05f;

	/** Rounded to the slider's steps and kept within Min..Max; anything that isn't a number gives the default. */
	static float ClampLookSensitivity(float Sensitivity);

	/** Slot's saved controls, or a fresh set at the defaults if it holds none. */
	static ULooterControlsSave* LoadControls(const FString& Slot, UObject* Outer);

	/** A save's look sensitivity, clamped (a hand-edited or damaged save can't spin the view); the default without one. */
	static float LookSensitivityOf(const ULooterControlsSave* Controls);

	/** Look input (mouse or stick, after its mapping's own modifiers) turned by Sensitivity. */
	static FVector2D ScaleLookInput(const FVector2D& Input, float Sensitivity);

	/** Look input scaled by the look sensitivity of the player Controller is; unchanged for anything without a local player. */
	static FVector2D ScaleLookInputFor(const AController* Controller, const FVector2D& Input);

	float GetLookSensitivity() const;

	/** Applies from the next look input. bSave writes it to disk; a slider being dragged passes false and saves on release. */
	void SetLookSensitivity(float Sensitivity, bool bSave = true);

	/** Camera shake, from none (0) to as designed (1, the default), in the slider's steps of CameraShakeStep. */
	static constexpr float MinCameraShake = 0.f;
	static constexpr float MaxCameraShake = 1.f;
	static constexpr float DefaultCameraShake = 1.f;
	static constexpr float CameraShakeStep = 0.05f;

	/** Rounded to the slider's steps and kept within Min..Max; anything that isn't a number gives the default. */
	static float ClampCameraShake(float Shake);

	/** A save's camera shake, clamped; the default without one. */
	static float CameraShakeOf(const ULooterControlsSave* Controls);

	float GetCameraShake() const;

	/** Applies from the next frame, to every shake and kick. bSave as for the sensitivity. */
	void SetCameraShake(float Shake, bool bSave = true);

	void SaveSettings() const;

private:
	UPROPERTY(Transient)
	TObjectPtr<ULooterControlsSave> SaveData;
};
