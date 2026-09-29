#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "GraphicsSettingsSubsystem.generated.h"

UCLASS()
class AI_LOOTER_SHOOTER_API ULooterGraphicsSave : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY()
	bool bMotionBlur = true;

	/** How see-through UI panel backgrounds are: 0 = solid, 1 = fully clear. */
	UPROPERTY()
	float UITransparency = 0.f;

	/** The HUD minimap's size, relative to its standard size. */
	UPROPERTY()
	float MinimapScale = 1.f;
};

/**
 * Player display options from the settings menu, saved to the "GraphicsSettings" slot and applied to this
 * player's game viewport and UI. Motion blur uses the viewport's show flag, so scalability changes and post
 * process volumes can't switch it back on behind the player's back. UI transparency fades every UI's panel
 * backgrounds (see LooterUI::SetBackgroundOpacity), live. The HUD reads the minimap size every frame.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UGraphicsSettingsSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	bool IsMotionBlurEnabled() const;
	void SetMotionBlurEnabled(bool bEnabled);

	/** 0 = solid panels, 1 = fully see-through. */
	float GetUITransparency() const;

	/** Applies to every UI at once. bSave writes it to disk; a slider being dragged passes false and saves on release. */
	void SetUITransparency(float Transparency, bool bSave = true);

	/**
	 * The HUD minimap's size relative to its standard size, from small and out of the way to big enough to read at a
	 * glance but never taking over the screen.
	 */
	static constexpr float MinMinimapScale = 0.6f;
	static constexpr float MaxMinimapScale = 1.5f;
	float GetMinimapScale() const;

	/** The HUD follows on its next frame. bSave as for SetUITransparency. */
	void SetMinimapScale(float Scale, bool bSave = true);

	void SaveSettings() const;

	/** Pushes the saved settings to the viewport and the UI. Safe to call any time. */
	void Apply() const;

private:
	UPROPERTY(Transient)
	TObjectPtr<ULooterGraphicsSave> SaveData;
};
