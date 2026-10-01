#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Scalability.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "GraphicsSettingsSubsystem.generated.h"

/** Overall graphics quality. Medium is the minimum spec: 120 fps at 1080p on a Radeon RX 580. */
UENUM()
enum class EGraphicsQuality : uint8
{
	Low,
	Medium,
	High,
	Epic
};

UCLASS()
class AI_LOOTER_SHOOTER_API ULooterGraphicsSave : public USaveGame
{
	GENERATED_BODY()

public:
	/** Saves older than CurrentVersion get its changed defaults once (see UGraphicsSettingsSubsystem::Initialize). */
	static constexpr int32 CurrentVersion = 1;

	UPROPERTY()
	int32 Version = 0;

	UPROPERTY()
	EGraphicsQuality Quality = EGraphicsQuality::Medium;

	/** Off by default: it costs Medium about 0.4 ms. */
	UPROPERTY()
	bool bMotionBlur = false;

	/** How see-through UI panel backgrounds are: 0 = solid, 1 = fully clear. */
	UPROPERTY()
	float UITransparency = 0.f;

	/** The HUD minimap's size, relative to its standard size. */
	UPROPERTY()
	float MinimapScale = 1.f;

	/** How far the HUD minimap is zoomed in: 1 shows 35 m around the player, 2 half that. */
	UPROPERTY()
	float MinimapZoom = 1.f;

	/** The frame rate counter in the HUD's top-left corner. Saves from before it existed load with it on. */
	UPROPERTY()
	bool bShowFrameRate = true;
};

/**
 * Player display options from the settings menu, saved to the "GraphicsSettings" slot and applied to this
 * player's game viewport and UI. The quality preset sets the engine's scalability level plus the heavy features
 * the reference card can't afford below High (see QualitySettings). Motion blur uses the viewport's show flag, so
 * scalability changes and post process volumes can't switch it back on behind the player's back. UI transparency
 * fades every UI's panel backgrounds (see LooterUI::SetBackgroundOpacity), live. The HUD reads the minimap size
 * and zoom every frame, and whether to show the frame rate twice a second.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UGraphicsSettingsSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	EGraphicsQuality GetQuality() const;

	/** Applies the preset at once (switching Nanite takes a moment) and saves it. */
	void SetQuality(EGraphicsQuality Quality);

	static FString QualityName(EGraphicsQuality Quality);

	/**
	 * What a preset sets besides the engine's scalability level (0 = Low ... 3 = Epic): console variable -> value.
	 * Lumen only on High and Epic, TSR only on Epic (TAA below), and no Nanite below High.
	 */
	static TMap<FString, int32> QualitySettings(EGraphicsQuality Quality);

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

	/**
	 * How far the HUD minimap is zoomed in, from the island at a glance to the street around you (the baked map's
	 * texels start to show past 2.5).
	 */
	static constexpr float MinMinimapZoom = 0.5f;
	static constexpr float MaxMinimapZoom = 2.5f;
	float GetMinimapZoom() const;

	/** The HUD follows on its next frame. bSave as for SetUITransparency. */
	void SetMinimapZoom(float Zoom, bool bSave = true);

	/** The HUD's frame rate counter (it follows within half a second). */
	bool IsFrameRateShown() const;
	void SetFrameRateShown(bool bShown);

	void SaveSettings() const;

	/** Pushes the saved settings to the viewport and the UI. Safe to call any time. */
	void Apply() const;

private:
	void ApplyQuality();

	UPROPERTY(Transient)
	TObjectPtr<ULooterGraphicsSave> SaveData;

	/** Play-in-editor shares the editor's rendering settings: what they were before this session changed them. */
	struct FEditorRendering
	{
		Scalability::FQualityLevels Levels;
		TMap<FString, FString> Variables;
	};
	TOptional<FEditorRendering> EditorRendering;
};
