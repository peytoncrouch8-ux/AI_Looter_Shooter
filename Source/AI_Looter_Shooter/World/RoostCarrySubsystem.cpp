#include "World/RoostCarrySubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundCues.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Story/CaptionSubsystem.h"
#include "UI/HUD/HudCrowVeilWidget.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformTime.h"

URoostCarrySubsystem* URoostCarrySubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World ? World->GetSubsystem<URoostCarrySubsystem>() : nullptr;
}

bool URoostCarrySubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	// Editor preview worlds too, for the automated tests (the carry moved on by Advance, Hob's words).
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::EditorPreview;
}

void URoostCarrySubsystem::Deinitialize()
{
	// The level is going: the crows on screen go with it, and so does the overlay (its player is the level's).
	Veil.Stop();
	if (Overlay)
	{
		Overlay->RemoveFromParent();
		Overlay = nullptr;
	}
	Super::Deinitialize();
}

void URoostCarrySubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Advance(DeltaTime);
}

bool URoostCarrySubsystem::IsTickable() const
{
	// Only while the crows are on the screen: between carries it costs nothing.
	return Veil.IsActive();
}

TStatId URoostCarrySubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(URoostCarrySubsystem, STATGROUP_Tickables);
}

// ---------------------------------------------------------------------------
// The carry
// ---------------------------------------------------------------------------

void URoostCarrySubsystem::BeginTravel(APlayerController* InViewer, float CoverSeconds)
{
	Begin(InViewer, ERoostVeilKind::Travel, CoverSeconds);
	// The rush of wings closing in and lifting: the player's own, so it's never lost under the world's sound.
	LooterSound::Play2D(this, LooterSoundCue::RoostMark::CarryOut);
}

void URoostCarrySubsystem::BeginDeath(APlayerController* InViewer, float CoverSeconds)
{
	const ERoostVeilPhase Now = Veil.GetPhase();
	if (Veil.IsActive() && (Now == ERoostVeilPhase::Cover || Now == ERoostVeilPhase::Hold))
	{
		// Killed in a fast travel's black (a fall as it went): the screen is the crows' already, and stays so until the wake-up.
		UE_LOG(LogLooter, Log, TEXT("Roost carry: a death while the crows already cover the screen; they hold it."));
		return;
	}
	Begin(InViewer, ERoostVeilKind::Death, CoverSeconds);
	LooterSound::Play2D(this, LooterSoundCue::RoostMark::DeathDrag);
}

void URoostCarrySubsystem::Begin(APlayerController* InViewer, ERoostVeilKind Kind, float CoverSeconds)
{
	Viewer = InViewer;
	const UWorld* World = GetWorld();
	// Every carry its own flock: seeded from the moment, so two carries never look alike.
	const int32 Seed = static_cast<int32>(FPlatformTime::Cycles() ^ static_cast<uint32>((World ? World->GetTimeSeconds() : 0.0) * 1000.0))
		^ (Carries * 7919);
	Veil.Start(Kind, CoverSeconds, Seed == 0 ? 1 : Seed);
	++Carries;
	UE_LOG(LogLooter, Log, TEXT("Roost carry: %s, the screen black in %.2f s."), Kind == ERoostVeilKind::Travel ? TEXT("fast travel") : TEXT("a death"),
		CoverSeconds);
	ShowVeil();
}

void URoostCarrySubsystem::Reveal()
{
	if (!Veil.IsActive())
	{
		return;
	}
	Veil.Reveal();
	ShowVeil();
}

void URoostCarrySubsystem::Advance(float DeltaSeconds)
{
	if (!Veil.IsActive())
	{
		return;
	}
	Veil.Advance(DeltaSeconds);
	ShowVeil();
}

void URoostCarrySubsystem::ShowVeil()
{
	// Made only while a carry wants showing: a level never carried never makes it.
	UHudCrowVeilWidget* Shown = Veil.IsActive() ? FindOrMakeOverlay() : Overlay.Get();
	if (Shown)
	{
		Shown->ShowVeil(Veil);
	}
}

UHudCrowVeilWidget* URoostCarrySubsystem::FindOrMakeOverlay()
{
	if (Overlay)
	{
		return Overlay;
	}
	UWorld* World = GetWorld();
	APlayerController* Shown = Viewer.Get();
	if (!Shown && World)
	{
		Shown = World->GetFirstPlayerController();
	}
	// A test level has nobody to show it to.
	if (!Shown || !Shown->IsLocalController())
	{
		return nullptr;
	}
	Overlay = CreateWidget<UHudCrowVeilWidget>(Shown, UHudCrowVeilWidget::StaticClass());
	if (Overlay)
	{
		Overlay->AddToViewport(UHudCrowVeilWidget::ViewportZOrder);
	}
	return Overlay;
}

// ---------------------------------------------------------------------------
// Hob's word
// ---------------------------------------------------------------------------

const FStoryLine* URoostCarrySubsystem::SayOnArrival(ERoostArrival Arrival)
{
	// Hob speaks only where the story is played; the once-only lines are the story's to keep (its flags, saved with it).
	UMissionRunner* Runner = UMissionRunner::Get(this);
	if (!Runner || !Runner->IsActive())
	{
		return nullptr;
	}
	FCampaignRecord& Campaign = Runner->GetCampaign();
	if (Arrival == ERoostArrival::Death)
	{
		if (Campaign.SetFlag(RoostRules::FirstDeathFlag))
		{
			LastSaid = RoostRules::FirstDeathLine();
		}
		else
		{
			if (LineOrder.IsEmpty() && LastLine == INDEX_NONE)
			{
				LineRandom.Initialize(static_cast<int32>(FPlatformTime::Cycles()));
			}
			LastSaid = RoostRules::DeathLine(RoostRules::NextLine(LineOrder, LastLine, RoostRules::NumDeathLines(), LineRandom));
		}
	}
	else if (Campaign.SetFlag(RoostRules::FirstTravelFlag))
	{
		LastSaid = RoostRules::FirstTravelLine();
	}
	else
	{
		return nullptr;
	}
	if (UCaptionSubsystem* Captions = UCaptionSubsystem::Get(this))
	{
		// After whatever is being said: a mission's line queued at the death still plays.
		Captions->Play({ LastSaid }, ECaptionPlay::Queue);
	}
	return &LastSaid;
}
