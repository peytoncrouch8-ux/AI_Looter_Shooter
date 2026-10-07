#include "Story/GraveSightSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "UI/HUD/HudGraveSightWidget.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

UGraveSightSubsystem* UGraveSightSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World ? World->GetSubsystem<UGraveSightSubsystem>() : nullptr;
}

bool UGraveSightSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::EditorPreview;
}

void UGraveSightSubsystem::Deinitialize()
{
	// The level is going: a flash on screen goes with it, and so does the overlay (its player is the level's).
	Current.Stop();
	if (Overlay)
	{
		Overlay->RemoveFromParent();
		Overlay = nullptr;
	}
	Super::Deinitialize();
}

void UGraveSightSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Advance(DeltaTime);
}

bool UGraveSightSubsystem::IsTickable() const
{
	// Only while a flash plays: between flashes it costs nothing.
	return Current.IsPlaying();
}

TStatId UGraveSightSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UGraveSightSubsystem, STATGROUP_Tickables);
}

void UGraveSightSubsystem::Flash(float Seconds)
{
	Current.Start(Seconds);
	++Flashes;
	UE_LOG(LogLooter, Log, TEXT("Grave Sight: a flash of %.1f s."), Current.GetSeconds());
	ShowFlash();
}

void UGraveSightSubsystem::Advance(float DeltaSeconds)
{
	if (!Current.IsPlaying())
	{
		return;
	}
	Current.Advance(DeltaSeconds);
	ShowFlash();
}

void UGraveSightSubsystem::ShowFlash()
{
	// Made only while a flash wants showing: a level that never flashes never makes it.
	UHudGraveSightWidget* Shown = Current.IsPlaying() ? FindOrMakeOverlay() : Overlay.Get();
	if (Shown)
	{
		Shown->SetFlash(Current.GetOverlayAlpha(), Current.GetProgress());
	}
}

UHudGraveSightWidget* UGraveSightSubsystem::FindOrMakeOverlay()
{
	if (Overlay)
	{
		return Overlay;
	}
	UWorld* World = GetWorld();
	APlayerController* Viewer = World ? World->GetFirstPlayerController() : nullptr;
	// A test level has nobody to show it to.
	if (!Viewer || !Viewer->IsLocalController())
	{
		return nullptr;
	}
	Overlay = CreateWidget<UHudGraveSightWidget>(Viewer, UHudGraveSightWidget::StaticClass());
	if (Overlay)
	{
		Overlay->AddToViewport(UHudGraveSightWidget::ViewportZOrder);
	}
	return Overlay;
}
