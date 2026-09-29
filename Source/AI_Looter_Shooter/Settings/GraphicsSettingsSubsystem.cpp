#include "Settings/GraphicsSettingsSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "UI/LooterUIStyle.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	const TCHAR* GraphicsSaveSlot = TEXT("GraphicsSettings");
}

void UGraphicsSettingsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	SaveData = Cast<ULooterGraphicsSave>(UGameplayStatics::LoadGameFromSlot(GraphicsSaveSlot, 0));
	if (!SaveData)
	{
		SaveData = NewObject<ULooterGraphicsSave>(this);
	}

	// The local player gets its viewport before its subsystems initialize, so this takes effect from the first frame.
	Apply();
}

bool UGraphicsSettingsSubsystem::IsMotionBlurEnabled() const
{
	return SaveData && SaveData->bMotionBlur;
}

void UGraphicsSettingsSubsystem::SetMotionBlurEnabled(bool bEnabled)
{
	if (!SaveData || SaveData->bMotionBlur == bEnabled)
	{
		return;
	}

	SaveData->bMotionBlur = bEnabled;
	Apply();
	SaveSettings();
	UE_LOG(LogLooter, Log, TEXT("Motion blur %s"), bEnabled ? TEXT("on") : TEXT("off"));
}

float UGraphicsSettingsSubsystem::GetUITransparency() const
{
	return SaveData ? SaveData->UITransparency : 0.f;
}

void UGraphicsSettingsSubsystem::SetUITransparency(float Transparency, bool bSave)
{
	if (!SaveData)
	{
		return;
	}
	SaveData->UITransparency = FMath::Clamp(Transparency, 0.f, 1.f);
	LooterUI::SetBackgroundOpacity(1.f - SaveData->UITransparency);
	if (bSave)
	{
		SaveSettings();
	}
}

float UGraphicsSettingsSubsystem::GetMinimapScale() const
{
	return SaveData ? FMath::Clamp(SaveData->MinimapScale, MinMinimapScale, MaxMinimapScale) : 1.f;
}

void UGraphicsSettingsSubsystem::SetMinimapScale(float Scale, bool bSave)
{
	if (!SaveData)
	{
		return;
	}
	SaveData->MinimapScale = FMath::Clamp(Scale, MinMinimapScale, MaxMinimapScale);
	if (bSave)
	{
		SaveSettings();
	}
}

void UGraphicsSettingsSubsystem::SaveSettings() const
{
	if (SaveData)
	{
		UGameplayStatics::SaveGameToSlot(SaveData, GraphicsSaveSlot, 0);
	}
}

void UGraphicsSettingsSubsystem::Apply() const
{
	const ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (UGameViewportClient* Viewport = LocalPlayer ? LocalPlayer->ViewportClient.Get() : nullptr)
	{
		Viewport->EngineShowFlags.SetMotionBlur(IsMotionBlurEnabled());
	}
	LooterUI::SetBackgroundOpacity(1.f - GetUITransparency());
}
