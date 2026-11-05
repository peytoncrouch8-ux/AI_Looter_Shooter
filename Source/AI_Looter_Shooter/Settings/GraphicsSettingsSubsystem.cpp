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
		/** Whole numbers for the integer variables (a float sets them as its whole value). */
		float Values[4];
	};
	const FQualityVariable QualityVariables[] = {
		// Lumen lighting and reflections, Epic only. Even with its lighting off, Lumen's upkeep costs over 2 ms; on the
		// reference card Lumen holds Epic at 22-28 fps, where High without it holds 53-71 (2026-10-14), so High is the
		// middle step: Nanite and virtual shadows with screen-space reflections (2).
		{ TEXT("r.DynamicGlobalIlluminationMethod"), { 0, 0, 0, 1 } },
		{ TEXT("r.ReflectionMethod"), { 0, 0, 2, 1 } },
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
		// occlusion into the meshes instead; High gets screen-space occlusion, Epic Lumen's and the distance fields'
		// (they need the global distance field, which High doesn't otherwise keep).
		{ TEXT("r.AmbientOcclusionLevels"), { 0, 0, -1, -1 } },
		{ TEXT("r.DistanceFieldAO"), { 0, 0, 0, 1 } },
		// Texture detail (the art audit of 2026-11-03, finding 3): the engine's Medium draws every map at half its size (mip
		// bias 1) and filters it 2x, so decks, ground and walls seen at a slant smear. Medium keeps the full maps, with a
		// streaming pool to hold them (the engine's 600 MB would drop mips again), and filters 4x: 8x (the first pick, as
		// High) cost the open views 0.1-0.4 ms (2026-11-04 evening, A/B in one run: Aerial 9.3 -> 8.9, the valley views
		// -0.1..-0.25; the ground's ~20 texture reads a pixel, seen at a slant), over the 8.3 ms line, so the user took 4x.
		// Creature LOD0 up close stays the engine's Medium (LOD bias 1): 0.3 ms in a horde for a difference nobody saw at
		// 3 m. Low stays the engine's own (mip bias 16, 1x, 400 MB); High's filtering rises from the engine's 4x to 8x.
		{ TEXT("r.Streaming.MipBias"), { 16, 0, 0, 0 } },
		{ TEXT("r.MaxAnisotropy"), { 1, 4, 8, 8 } },
		{ TEXT("r.SkeletalMeshLODBias"), { 2, 1, 0, 0 } },
		{ TEXT("r.Streaming.PoolSize"), { 400, 1000, 1000, 1000 } },
		// Medium switches every static mesh to its next LOD a quarter sooner (at 80% of the distance): 0-0.25 ms in the
		// heaviest views (2026-11-04 evening: Grave and PointOverValley -0.2, the aerials -0.1), the user's yes; the
		// engine's own distances elsewhere.
		{ TEXT("r.StaticMeshLODDistanceScale"), { 1.f, 1.25f, 1.f, 1.f } },
		// The shrubs (scatter tagged ShadowHighOnly: sagebrush, rabbitbrush, bushes) cast no shadow on Low and Medium:
		// about 0.1 ms at the farm on the reference card, the user's pick 2026-11-04 (grass already casts none); High and
		// Epic keep them. ULumenScatterSubsystem applies it as play begins and whenever it changes.
		{ TEXT("Looter.Scatter.HighShadows"), { 0, 0, 1, 1 } },
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

	/** Looter.PauseMenuSize <60-100>: sets and saves the pause menu's size in percent, as the settings menu's slider does. */
	void SetPauseMenuSizeCommand(const TArray<FString>& Args, UWorld* World)
	{
		ULocalPlayer* Player = World ? World->GetFirstLocalPlayerFromController() : nullptr;
		UGraphicsSettingsSubsystem* Graphics = Player ? Player->GetSubsystem<UGraphicsSettingsSubsystem>() : nullptr;
		if (!Graphics || Args.Num() != 1 || !Args[0].IsNumeric())
		{
			UE_LOG(LogLooter, Warning, TEXT("Usage (in a game): Looter.PauseMenuSize <%d-%d>"),
				FMath::RoundToInt32(UGraphicsSettingsSubsystem::MinPauseMenuScale * 100.f), FMath::RoundToInt32(UGraphicsSettingsSubsystem::MaxPauseMenuScale * 100.f));
			return;
		}
		Graphics->SetPauseMenuScale(FCString::Atof(*Args[0]) / 100.f);
		UE_LOG(LogLooter, Display, TEXT("Pause menu size: %d%%."), FMath::RoundToInt32(Graphics->GetPauseMenuScale() * 100.f));
	}

	FAutoConsoleCommandWithWorldAndArgs SetPauseMenuSizeCommandRegistration(
		TEXT("Looter.PauseMenuSize"),
		TEXT("Sets and saves the pause menu's size in percent: Looter.PauseMenuSize 60-100"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SetPauseMenuSizeCommand));

	/** Looter.Subtitles on|off: sets and saves the subtitles switch, as the settings menu's row does. */
	void SetSubtitlesCommand(const TArray<FString>& Args, UWorld* World)
	{
		ULocalPlayer* Player = World ? World->GetFirstLocalPlayerFromController() : nullptr;
		UGraphicsSettingsSubsystem* Graphics = Player ? Player->GetSubsystem<UGraphicsSettingsSubsystem>() : nullptr;
		const bool bOn = Args.Num() == 1 && Args[0].Equals(TEXT("on"), ESearchCase::IgnoreCase);
		const bool bOff = Args.Num() == 1 && Args[0].Equals(TEXT("off"), ESearchCase::IgnoreCase);
		if (!Graphics || (!bOn && !bOff))
		{
			UE_LOG(LogLooter, Warning, TEXT("Usage (in a game): Looter.Subtitles on|off"));
			return;
		}
		Graphics->SetSubtitlesShown(bOn);
		UE_LOG(LogLooter, Display, TEXT("Subtitles %s."), bOn ? TEXT("on") : TEXT("off"));
	}

	FAutoConsoleCommandWithWorldAndArgs SetSubtitlesCommandRegistration(
		TEXT("Looter.Subtitles"),
		TEXT("Sets and saves the subtitles switch: Looter.Subtitles on|off"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SetSubtitlesCommand));

	/** Looter.CrosshairColour White|Salt|Gold|Cyan|Green: sets and saves the crosshair's colour, as the settings row does (HUD shots). */
	void SetCrosshairColourCommand(const TArray<FString>& Args, UWorld* World)
	{
		ULocalPlayer* Player = World ? World->GetFirstLocalPlayerFromController() : nullptr;
		UGraphicsSettingsSubsystem* Graphics = Player ? Player->GetSubsystem<UGraphicsSettingsSubsystem>() : nullptr;
		if (Graphics && Args.Num() == 1)
		{
			for (const ECrosshairColour Colour : AllCrosshairColours)
			{
				if (Args[0].Equals(UGraphicsSettingsSubsystem::CrosshairColourName(Colour), ESearchCase::IgnoreCase))
				{
					Graphics->SetCrosshairColour(Colour);
					UE_LOG(LogLooter, Display, TEXT("Crosshair colour: %s."), *UGraphicsSettingsSubsystem::CrosshairColourName(Colour));
					return;
				}
			}
		}
		UE_LOG(LogLooter, Warning, TEXT("Usage (in a game): Looter.CrosshairColour White|Salt|Gold|Cyan|Green"));
	}

	FAutoConsoleCommandWithWorldAndArgs SetCrosshairColourCommandRegistration(
		TEXT("Looter.CrosshairColour"),
		TEXT("Sets and saves the crosshair's colour: Looter.CrosshairColour White|Salt|Gold|Cyan|Green"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SetCrosshairColourCommand));
}

void UGraphicsSettingsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// A first run has no file yet: asking first keeps the log free of a load warning.
	SaveData = UGameplayStatics::DoesSaveGameExist(GraphicsSaveSlot, 0)
		? Cast<ULooterGraphicsSave>(UGameplayStatics::LoadGameFromSlot(GraphicsSaveSlot, 0)) : nullptr;
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

TMap<FString, float> UGraphicsSettingsSubsystem::QualitySettings(EGraphicsQuality Quality)
{
	const int32 Level = static_cast<int32>(Quality);
	TMap<FString, float> Settings;
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
	for (const TPair<FString, float>& Setting : QualitySettings(Quality))
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

float UGraphicsSettingsSubsystem::GetPauseMenuScale() const
{
	return SaveData ? FMath::Clamp(SaveData->PauseMenuScale, MinPauseMenuScale, MaxPauseMenuScale) : 1.f;
}

void UGraphicsSettingsSubsystem::SetPauseMenuScale(float Scale, bool bSave)
{
	if (!SaveData)
	{
		return;
	}
	SaveData->PauseMenuScale = FMath::Clamp(Scale, MinPauseMenuScale, MaxPauseMenuScale);
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

bool UGraphicsSettingsSubsystem::SubtitlesOf(const ULooterGraphicsSave* Save)
{
	return !Save || Save->bShowSubtitles;
}

bool UGraphicsSettingsSubsystem::AreSubtitlesShown() const
{
	return SubtitlesOf(SaveData);
}

void UGraphicsSettingsSubsystem::SetSubtitlesShown(bool bShown)
{
	if (!SaveData || SaveData->bShowSubtitles == bShown)
	{
		return;
	}
	SaveData->bShowSubtitles = bShown;
	SaveSettings();
	UE_LOG(LogLooter, Log, TEXT("Subtitles %s"), bShown ? TEXT("on") : TEXT("off"));
}

ECrosshairColour UGraphicsSettingsSubsystem::CrosshairColourOf(const ULooterGraphicsSave* Save)
{
	if (!Save)
	{
		return ECrosshairColour::White;
	}
	for (const ECrosshairColour Colour : AllCrosshairColours)
	{
		if (Save->CrosshairColour == Colour)
		{
			return Colour;
		}
	}
	return ECrosshairColour::White;
}

ECrosshairColour UGraphicsSettingsSubsystem::GetCrosshairColour() const
{
	return CrosshairColourOf(SaveData);
}

void UGraphicsSettingsSubsystem::SetCrosshairColour(ECrosshairColour Colour)
{
	if (!SaveData || SaveData->CrosshairColour == Colour)
	{
		return;
	}
	SaveData->CrosshairColour = Colour;
	SaveSettings();
}

FString UGraphicsSettingsSubsystem::CrosshairColourName(ECrosshairColour Colour)
{
	switch (Colour)
	{
	case ECrosshairColour::White: return TEXT("White");
	case ECrosshairColour::Salt:  return TEXT("Salt");
	case ECrosshairColour::Gold:  return TEXT("Gold");
	case ECrosshairColour::Cyan:  return TEXT("Cyan");
	case ECrosshairColour::Green: return TEXT("Green");
	}
	return TEXT("White");
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
