#include "Dev/ViewTour.h"
#include "AI_Looter_Shooter.h"
#include "Settings/GraphicsSettingsSubsystem.h"
#include "World/WorldQueries.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Dom/JsonObject.h"
#include "DynamicRHI.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/HUD.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/App.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "ProfilingDebugging/CsvProfiler.h"
#include "RenderTimer.h"
#include "RHIStats.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"

namespace
{
	const TCHAR* DefaultViews = TEXT("Art/Levels/TutorialIsland/views.json");
	// Streaming, LODs and exposure settle before measuring; the first stop also waits for the level's first frames.
	constexpr float FirstSettleSeconds = 6.f;
	constexpr float SettleSeconds = 3.f;
	constexpr float MeasureSeconds = 3.f;
	// The screenshot is written at the end of a frame; give it a moment before the view moves.
	constexpr float ShotSeconds = 1.f;

	FString QualityName(const UWorld* World)
	{
		const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
		const ULocalPlayer* Player = Controller ? Controller->GetLocalPlayer() : nullptr;
		const UGraphicsSettingsSubsystem* Graphics = Player ? Player->GetSubsystem<UGraphicsSettingsSubsystem>() : nullptr;
		return Graphics ? UGraphicsSettingsSubsystem::QualityName(Graphics->GetQuality()) : TEXT("Unknown");
	}
}

void UViewTourSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!Stops.IsValidIndex(Current))
	{
		return;
	}

	// Real time, not game time: the tour must behave the same under slow motion.
	const float Delta = static_cast<float>(FApp::GetDeltaTime());
	Clock += Delta;
	const float Settle = Current == 0 ? FirstSettleSeconds : SettleSeconds;
	if (Clock > Settle && Clock <= Settle + MeasureSeconds)
	{
		if (Frames == 0)
		{
			// Marks the measured frames in a CSV profiler capture (Tools/perf.ps1 -Tour splits a capture by these).
			CSV_EVENT_GLOBAL(TEXT("Tour %s"), *Stops[Current].Name);
		}
		const double Frame = Delta * 1000.0;
		++Frames;
		FrameMs += Frame;
		WorstFrameMs = FMath::Max(WorstFrameMs, Frame);
		GameMs += FPlatformTime::ToMilliseconds(GGameThreadTime);
		RenderMs += FPlatformTime::ToMilliseconds(GRenderThreadTime);
		GpuMs += FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles(0));
		DrawCalls += GNumDrawCallsRHI[0];
		Primitives += GNumPrimitivesDrawnRHI[0];
	}
	else if (Clock > Settle + MeasureSeconds && !bShotRequested)
	{
		CSV_EVENT_GLOBAL(TEXT("Tour end"));
		Report();
		if (bShots)
		{
			const FString Shot = FPaths::ProjectSavedDir() / TEXT("Screenshots/Tour") / FString::Printf(TEXT("%s_%s.png"),
				*Stops[Current].Name, *QualityName(GetWorld()));
			FScreenshotRequest::RequestScreenshot(Shot, /*bInShowUI*/ false, /*bAddFilenameSuffix*/ false);
		}
		bShotRequested = true;
	}
	else if (bShotRequested && Clock > Settle + MeasureSeconds + ShotSeconds)
	{
		Visit(Current + 1);
	}
}

TStatId UViewTourSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UViewTourSubsystem, STATGROUP_Tickables);
}

bool UViewTourSubsystem::IsTickable() const
{
	return Stops.IsValidIndex(Current);
}

bool UViewTourSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

bool UViewTourSubsystem::Start(const FString& ViewsFile, bool bInShots, bool bInQuit)
{
	FString Text;
	const FString Path = FPaths::ProjectDir() / ViewsFile;
	TSharedPtr<FJsonObject> Json;
	const TArray<TSharedPtr<FJsonValue>>* Views = nullptr;
	if (!FFileHelper::LoadFileToString(Text, *Path) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Json)
		|| !Json.IsValid() || !Json->TryGetArrayField(TEXT("views"), Views))
	{
		UE_LOG(LogLooter, Warning, TEXT("Looter.Tour: can't read the viewpoints in %s."), *Path);
		return false;
	}

	UWorld* World = GetWorld();
	Stops.Reset();
	for (const TSharedPtr<FJsonValue>& Value : *Views)
	{
		const TSharedPtr<FJsonObject> View = Value->AsObject();
		if (!View.IsValid())
		{
			continue;
		}
		FStop Stop;
		Stop.Name = View->GetStringField(TEXT("name"));
		const double X = View->GetNumberField(TEXT("x"));
		const double Y = View->GetNumberField(TEXT("y"));
		// The height is above the ground there: find the ground with a trace that skips volumes.
		FHitResult Hit;
		const bool bGround = World->LineTraceSingleByChannel(Hit, FVector(X, Y, 100000.0), FVector(X, Y, -100000.0), ECC_Visibility,
			LooterWorld::StaticGeometryParams(World, TEXT("ViewTour")));
		Stop.Location = FVector(X, Y, (bGround ? Hit.ImpactPoint.Z : 0.0) + View->GetNumberField(TEXT("height")));
		Stop.Rotation = FRotator(View->GetNumberField(TEXT("pitch")), View->GetNumberField(TEXT("yaw")), 0.0);
		double FieldOfView = 80.0;
		View->TryGetNumberField(TEXT("fov"), FieldOfView);
		Stop.FieldOfView = static_cast<float>(FieldOfView);
		View->TryGetStringArrayField(TEXT("exec"), Stop.Exec);
		View->TryGetStringArrayField(TEXT("after"), Stop.After);
		Stops.Add(Stop);
	}
	if (Stops.IsEmpty())
	{
		UE_LOG(LogLooter, Warning, TEXT("Looter.Tour: %s has no viewpoints."), *Path);
		return false;
	}

	bShots = bInShots;
	bQuit = bInQuit;
	Rows.Reset();
	if (!Camera)
	{
		FActorSpawnParameters Params;
		Params.ObjectFlags |= RF_Transient;
		Camera = World->SpawnActor<ACameraActor>(Params);
	}
	UE_LOG(LogLooter, Display, TEXT("Looter.Tour: %d viewpoints from %s, quality %s."), Stops.Num(), *ViewsFile, *QualityName(World));
	Visit(0);
	return true;
}

void UViewTourSubsystem::Visit(int32 Index)
{
	APlayerController* Controller = GetWorld()->GetFirstPlayerController();
	// What the last view switched for its own measuring goes back first.
	if (Stops.IsValidIndex(Current))
	{
		RunCommands(Stops[Current].After);
	}
	Current = Index;
	Clock = 0.f;
	bShotRequested = false;
	Frames = 0;
	FrameMs = GameMs = RenderMs = GpuMs = WorstFrameMs = 0.0;
	DrawCalls = Primitives = 0;

	if (Stops.IsValidIndex(Index))
	{
		const FStop& Stop = Stops[Index];
		Camera->SetActorLocationAndRotation(Stop.Location, Stop.Rotation);
		Camera->GetCameraComponent()->SetFieldOfView(Stop.FieldOfView);
		RunCommands(Stop.Exec);
		if (Controller)
		{
			Controller->SetViewTarget(Camera);
			if (Controller->MyHUD)
			{
				Controller->MyHUD->bShowHUD = false;
			}
		}
		return;
	}

	// Done: back to the player, the results saved.
	Current = INDEX_NONE;
	if (Controller)
	{
		Controller->SetViewTarget(Controller->GetPawn());
		if (Controller->MyHUD)
		{
			Controller->MyHUD->bShowHUD = true;
		}
	}
	const FString Csv = FPaths::ProjectSavedDir() / TEXT("Tour") / GetWorld()->GetMapName() + TEXT(".csv");
	FString Existing;
	const bool bNew = !FFileHelper::LoadFileToString(Existing, *Csv);
	TArray<FString> Lines;
	if (bNew)
	{
		Lines.Add(TEXT("When,View,Quality,Frame ms,fps,Worst ms,Game ms,Render ms,GPU ms,Draws,Primitives"));
	}
	Lines.Append(Rows);
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(Csv), true);
	FFileHelper::SaveStringArrayToFile(Lines, *Csv, FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), FILEWRITE_Append);
	UE_LOG(LogLooter, Display, TEXT("Looter.Tour: done, results in %s."), *Csv);
	if (bQuit)
	{
		FPlatformMisc::RequestExit(false, TEXT("Looter.Tour"));
	}
}

void UViewTourSubsystem::RunCommands(const TArray<FString>& Commands) const
{
	APlayerController* Controller = GetWorld()->GetFirstPlayerController();
	for (const FString& Command : Commands)
	{
		UE_LOG(LogLooter, Display, TEXT("Looter.Tour: %s"), *Command);
		if (Controller)
		{
			Controller->ConsoleCommand(Command);
		}
		else if (GEngine)
		{
			GEngine->Exec(GetWorld(), *Command);
		}
	}
}

void UViewTourSubsystem::Report()
{
	const double N = FMath::Max(Frames, 1);
	const double Frame = FrameMs / N;
	const FString Quality = QualityName(GetWorld());
	UE_LOG(LogLooter, Display, TEXT("Looter.Tour: %s (%s): frame %.1f ms (%.0f fps), worst %.1f, game %.1f, render %.1f, GPU %.1f, %.0f draws, %.0f primitives."),
		*Stops[Current].Name, *Quality, Frame, 1000.0 / FMath::Max(Frame, 0.01), WorstFrameMs, GameMs / N, RenderMs / N, GpuMs / N,
		DrawCalls / N, Primitives / N);
	Rows.Add(FString::Printf(TEXT("%s,%s,%s,%.2f,%.0f,%.2f,%.2f,%.2f,%.2f,%.0f,%.0f"), *FDateTime::Now().ToString(TEXT("%Y-%m-%d %H:%M")),
		*Stops[Current].Name, *Quality, Frame, 1000.0 / FMath::Max(Frame, 0.01), WorstFrameMs, GameMs / N, RenderMs / N, GpuMs / N,
		DrawCalls / N, Primitives / N));
}

#if !UE_BUILD_SHIPPING
namespace
{
	/** Looter.Tour [views file] [noshots] [quit] */
	void TourCommand(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* GameWorld = World && World->IsGameWorld() ? World : nullptr;
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			GameWorld = GameWorld ? GameWorld : (Context.World() && Context.World()->IsGameWorld() ? Context.World() : nullptr);
		}
		UViewTourSubsystem* Tour = GameWorld ? GameWorld->GetSubsystem<UViewTourSubsystem>() : nullptr;
		if (!Tour)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Tour: start the game first."));
			return;
		}
		FString File = DefaultViews;
		bool bShots = true;
		bool bQuit = false;
		for (const FString& Arg : Args)
		{
			if (Arg.Equals(TEXT("noshots"), ESearchCase::IgnoreCase))
			{
				bShots = false;
			}
			else if (Arg.Equals(TEXT("quit"), ESearchCase::IgnoreCase))
			{
				bQuit = true;
			}
			else
			{
				File = Arg;
			}
		}
		if (!Tour->Start(File, bShots, bQuit) && bQuit)
		{
			FPlatformMisc::RequestExit(false, TEXT("Looter.Tour"));
		}
	}

	FAutoConsoleCommandWithWorldAndArgs TourCommandRegistration(
		TEXT("Looter.Tour"),
		TEXT("Looks from each viewpoint of the level (default Art/Levels/TutorialIsland/views.json), measures frame times there and takes a screenshot: Looter.Tour [views file] [noshots] [quit]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TourCommand));
}
#endif
