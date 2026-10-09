#include "Settings/GraphicsSettingsSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "UI/Style/LooterUIStyle.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	const TCHAR* GraphicsSaveSlot = TEXT("GraphicsSettings");

	/**
	 * The presets' own settings, by level (Low, Medium, High, Epic), on top of the engine's scalability level of the
	 * same name. Medium has to hold 120 fps at 1080p on the reference card (RX 580); Docs/Performance.md has the history.
	 */
	struct FQualityVariable
	{
		const TCHAR* Name;
		int32 Values[4];
	};
	const FQualityVariable QualityVariables[] = {
		// Lumen lighting and reflections. Even with its lighting off, Lumen's upkeep costs over 2 ms.
		{ TEXT("r.DynamicGlobalIlluminationMethod"), { 0, 0, 1, 1 } },
		{ TEXT("r.ReflectionMethod"), { 0, 0, 1, 1 } },
		// TSR costs about 5 ms at 1080p; TAA about half a millisecond.
		{ TEXT("r.AntiAliasingMethod"), { 2, 2, 2, 4 } },
		// Nanite costs about 2.5 ms; without it every mesh draws its fallback.
		{ TEXT("r.Nanite"), { 0, 0, 1, 1 } },
		// Virtual shadow maps are only cheap with Nanite: without it they cost Medium 2.5 ms, where two cascades (the
		// engine's Medium has one of 1024, too coarse to show a person's shadow) cost 0.35 on open ground. Under trees
		// every leaf card is drawn into them: 1536 instead of 2048 saves the forest 0.5 ms.
		{ TEXT("r.Shadow.Virtual.Enable"), { 0, 0, 1, 1 } },
		{ TEXT("r.Shadow.CSM.MaxCascades"), { 1, 2, 4, 10 } },
		{ TEXT("r.Shadow.MaxCSMResolution"), { 512, 1536, 2048, 2048 } },
		// Swaying foliage (world position offset) writing motion vectors costs the forest 0.4 ms on Medium; TAA copes
		// without them. 2 is the engine's default.
		{ TEXT("r.Velocity.EnableVertexDeformation"), { 0, 0, 2, 2 } },
		// Screen-space and distance field ambient occlusion cost Medium 1.7 and 0.7 ms. The textured art bakes its
		// occlusion into the meshes instead; High and Epic get Lumen's.
		{ TEXT("r.AmbientOcclusionLevels"), { 0, 0, -1, -1 } },
		{ TEXT("r.DistanceFieldAO"), { 0, 0, 1, 1 } },
	};

	/** Looter.Quality Low|Medium|High|Epic: sets and saves the preset, as the settings menu does (handy for perf runs). */
	void SetQualityCommand(const TArray<FString>& Args, UWorld* World)
	{
		ULocalPlayer* Player = World ? World->GetFirstLocalPlayerFromController() : nullptr;
		UGraphicsSettingsSubsystem* Graphics = Player ? Player->GetSubsystem<UGraphicsSettingsSubsystem>() : nullptr;
		if (!Graphics || Args.Num() != 1)
		{
			UE_LOG(LogLooter, Warning, TEXT("Usage (in a game): Looter.Quality Low|Medium|High|Epic"));
			return;
		}
		for (const EGraphicsQuality Quality : { EGraphicsQuality::Low, EGraphicsQuality::Medium, EGraphicsQuality::High, EGraphicsQuality::Epic })
		{
			if (Args[0].Equals(UGraphicsSettingsSubsystem::QualityName(Quality), ESearchCase::IgnoreCase))
			{
				Graphics->SetQuality(Quality);
				return;
			}
		}
		UE_LOG(LogLooter, Warning, TEXT("Unknown quality '%s'. Use Low, Medium, High or Epic."), *Args[0]);
	}

	FAutoConsoleCommandWithWorldAndArgs SetQualityCommandRegistration(
		TEXT("Looter.Quality"),
		TEXT("Sets and saves the graphics quality preset: Looter.Quality Low|Medium|High|Epic"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SetQualityCommand));

	/**
	 * Looter.FieldOfView <degrees>: sets and saves the first-person field of view, as the settings menu's slider does (a
	 * wider view draws more, so perf runs can measure it).
	 */
	void SetFieldOfViewCommand(const TArray<FString>& Args, UWorld* World)
	{
		ULocalPlayer* Player = World ? World->GetFirstLocalPlayerFromController() : nullptr;
		UGraphicsSettingsSubsystem* Graphics = Player ? Player->GetSubsystem<UGraphicsSettingsSubsystem>() : nullptr;
		if (!Graphics || Args.Num() != 1 || !Args[0].IsNumeric())
		{
			UE_LOG(LogLooter, Warning, TEXT("Usage (in a game): Looter.FieldOfView <%d-%d>"),
				FMath::RoundToInt32(UGraphicsSettingsSubsystem::MinFieldOfView), FMath::RoundToInt32(UGraphicsSettingsSubsystem::MaxFieldOfView));
			return;
		}
		Graphics->SetFirstPersonFieldOfView(FCString::Atof(*Args[0]));
		UE_LOG(LogLooter, Display, TEXT("First-person field of view: %d degrees."), FMath::RoundToInt32(Graphics->GetFirstPersonFieldOfView()));
	}

	FAutoConsoleCommandWithWorldAndArgs SetFieldOfViewCommandRegistration(
		TEXT("Looter.FieldOfView"),
		TEXT("Sets and saves the first-person field of view in degrees: Looter.FieldOfView 70-110"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SetFieldOfViewCommand));
}

void UGraphicsSettingsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	SaveData = Cast<ULooterGraphicsSave>(UGameplayStatics::LoadGameFromSlot(GraphicsSaveSlot, 0));
	if (!SaveData)
	{
		SaveData = NewObject<ULooterGraphicsSave>(this);
	}
	if (SaveData->Version < ULooterGraphicsSave::CurrentVersion)
	{
		// Version 1 turned motion blur off by default, for the 120 fps budget; older saves still hold the old default.
		SaveData->bMotionBlur = false;
		SaveData->Version = ULooterGraphicsSave::CurrentVersion;
		SaveSettings();
	}

	// The local player gets its viewport before its subsystems initialize, so this takes effect from the first frame.
	Apply();
	ApplyQuality();
}

void UGraphicsSettingsSubsystem::Deinitialize()
{
	// Play-in-editor shares the editor's rendering settings: give the editor back its own.
	if (EditorRendering.IsSet())
	{
		Scalability::SetQualityLevels(EditorRendering->Levels);
		for (const TPair<FString, FString>& Variable : EditorRendering->Variables)
		{
			if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(*Variable.Key))
			{
				CVar->Set(*Variable.Value, ECVF_SetByGameOverride);
			}
		}
		EditorRendering.Reset();
	}
	Super::Deinitialize();
}

EGraphicsQuality UGraphicsSettingsSubsystem::GetQuality() const
{
	return SaveData ? SaveData->Quality : EGraphicsQuality::Medium;
}

void UGraphicsSettingsSubsystem::SetQuality(EGraphicsQuality Quality)
{
	if (!SaveData || SaveData->Quality == Quality)
	{
		return;
	}
	SaveData->Quality = Quality;
	ApplyQuality();
	SaveSettings();
	UE_LOG(LogLooter, Log, TEXT("Graphics quality %s"), *QualityName(Quality));
}

FString UGraphicsSettingsSubsystem::QualityName(EGraphicsQuality Quality)
{
	switch (Quality)
	{
	case EGraphicsQuality::Low:    return TEXT("Low");
	case EGraphicsQuality::Medium: return TEXT("Medium");
	case EGraphicsQuality::High:   return TEXT("High");
	case EGraphicsQuality::Epic:   return TEXT("Epic");
	}
	return TEXT("Medium");
}

TMap<FString, int32> UGraphicsSettingsSubsystem::QualitySettings(EGraphicsQuality Quality)
{
	const int32 Level = static_cast<int32>(Quality);
	TMap<FString, int32> Settings;
	for (const FQualityVariable& Variable : QualityVariables)
	{
		Settings.Add(Variable.Name, Variable.Values[Level]);
	}
	return Settings;
}

void UGraphicsSettingsSubsystem::ApplyQuality()
{
	if (GIsEditor && !EditorRendering.IsSet())
	{
		FEditorRendering Saved;
		Saved.Levels = Scalability::GetQualityLevels();
		for (const FQualityVariable& Variable : QualityVariables)
		{
			if (const IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(Variable.Name))
			{
				Saved.Variables.Add(Variable.Name, CVar->GetString());
			}
		}
		EditorRendering = MoveTemp(Saved);
	}

	const EGraphicsQuality Quality = GetQuality();
	Scalability::FQualityLevels Levels = Scalability::GetQualityLevels();
	Levels.SetFromSingleQualityLevel(static_cast<int32>(Quality));
	// Always full resolution (the engine's own levels render Medium at 71%): Medium still has time to spare at 1080p.
	Levels.ResolutionQuality = 100.f;
	Scalability::SetQualityLevels(Levels);
	for (const TPair<FString, int32>& Setting : QualitySettings(Quality))
	{
		if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(*Setting.Key))
		{
			// Above the project settings, which set some of these (the command line and console still win, for testing).
			CVar->Set(Setting.Value, ECVF_SetByGameOverride);
		}
	}
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

float UGraphicsSettingsSubsystem::ClampFieldOfView(float Degrees)
{
	// Whole degrees, so the menu's label and the saved value always agree.
	return FMath::Clamp(FMath::RoundToFloat(Degrees), MinFieldOfView, MaxFieldOfView);
}

float UGraphicsSettingsSubsystem::GetFirstPersonFieldOfView() const
{
	// Clamped on the way out too, so a hand-edited or damaged save can't turn the view inside out.
	return SaveData ? ClampFieldOfView(SaveData->FirstPersonFieldOfView) : DefaultFieldOfView;
}

void UGraphicsSettingsSubsystem::SetFirstPersonFieldOfView(float Degrees, bool bSave)
{
	if (!SaveData)
	{
		return;
	}
	SaveData->FirstPersonFieldOfView = ClampFieldOfView(Degrees);
	if (bSave)
	{
		SaveSettings();
	}
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

float UGraphicsSettingsSubsystem::GetMinimapZoom() const
{
	return SaveData ? FMath::Clamp(SaveData->MinimapZoom, MinMinimapZoom, MaxMinimapZoom) : 1.f;
}

void UGraphicsSettingsSubsystem::SetMinimapZoom(float Zoom, bool bSave)
{
	if (!SaveData)
	{
		return;
	}
	SaveData->MinimapZoom = FMath::Clamp(Zoom, MinMinimapZoom, MaxMinimapZoom);
	if (bSave)
	{
		SaveSettings();
	}
}

bool UGraphicsSettingsSubsystem::IsMinimapShown() const
{
	return !SaveData || SaveData->bShowMinimap;
}

void UGraphicsSettingsSubsystem::SetMinimapShown(bool bShown)
{
	if (!SaveData || SaveData->bShowMinimap == bShown)
	{
		return;
	}
	SaveData->bShowMinimap = bShown;
	SaveSettings();
}

bool UGraphicsSettingsSubsystem::IsFrameRateShown() const
{
	return !SaveData || SaveData->bShowFrameRate;
}

void UGraphicsSettingsSubsystem::SetFrameRateShown(bool bShown)
{
	if (!SaveData || SaveData->bShowFrameRate == bShown)
	{
		return;
	}
	SaveData->bShowFrameRate = bShown;
	SaveSettings();
}

bool UGraphicsSettingsSubsystem::AreControlHintsShown() const
{
	return !SaveData || SaveData->bShowControlHints;
}

void UGraphicsSettingsSubsystem::SetControlHintsShown(bool bShown)
{
	if (!SaveData || SaveData->bShowControlHints == bShown)
	{
		return;
	}
	SaveData->bShowControlHints = bShown;
	SaveSettings();
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
