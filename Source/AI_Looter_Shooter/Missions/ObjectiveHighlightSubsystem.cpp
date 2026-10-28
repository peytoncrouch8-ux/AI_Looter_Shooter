// UObjectiveHighlightSubsystem: when it looks, what it gathers, who's in range, the fades and the rules, and the console's
// Looter.Highlight.* (ObjectiveHighlightSubsystemLook.cpp puts the highlight on the meshes and picks them).

#include "Missions/ObjectiveHighlightSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Interaction/Interactable.h"
#include "Missions/MissionRunner.h"
#include "Missions/MissionSubsystem.h"
#include "Missions/ObjectiveHighlightTargets.h"
#include "Scenes/SceneSubsystem.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/MeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "HAL/IConsoleManager.h"

const TCHAR* UObjectiveHighlightSubsystem::MaterialPath = TEXT("/Game/Art/Materials/Masters/M_ObjectiveHighlight.M_ObjectiveHighlight");

namespace
{
	TAutoConsoleVariable<float> CVarHighlightRadius(TEXT("Looter.Highlight.Radius"), UObjectiveHighlightSubsystem::DefaultRadius,
		TEXT("How near (cm) the player comes to what the tracked objective is about before it glows orange (default 1300)."));

	TAutoConsoleVariable<bool> CVarHighlightEnabled(TEXT("Looter.Highlight.Enabled"), true,
		TEXT("The tracked objective's orange highlight (0: off, and what's lit fades out)."));
}

UObjectiveHighlightSubsystem* UObjectiveHighlightSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World ? World->GetSubsystem<UObjectiveHighlightSubsystem>() : nullptr;
}

bool UObjectiveHighlightSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	// Played worlds, and the editor preview worlds the automated tests build (there it only looks when asked).
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::EditorPreview;
}

void UObjectiveHighlightSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	// Loaded as the level starts, not the first time something comes into range (a hitch right as the player arrives).
	if (InWorld.IsGameWorld())
	{
		LoadMaterial();
	}
}

void UObjectiveHighlightSubsystem::Deinitialize()
{
	if (UMissionRunner* Runner = BoundRunner.Get())
	{
		Runner->OnChanged.Remove(RunnerHandle);
	}
	if (UMissionSubsystem* Display = BoundDisplay.Get())
	{
		Display->OnMissionsChanged.Remove(DisplayHandle);
	}
	BoundRunner.Reset();
	BoundDisplay.Reset();
	RunnerHandle.Reset();
	DisplayHandle.Reset();
	// A level going away takes its meshes with it: only one that stays (a test's, the editor's) gets them back as they were.
	const UWorld* World = GetWorld();
	if (World && !World->bIsTearingDown)
	{
		ClearAll();
		DestroyMarkers();
	}
	Highlights.Reset();
	Markers.Reset();
	Candidates.Reset();
	FreeLooks.Reset();
	Looks.Reset();
	Super::Deinitialize();
}

void UObjectiveHighlightSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Advance(DeltaTime);
}

bool UObjectiveHighlightSubsystem::IsTickable() const
{
	// Only where someone plays: a test level asks for its looks.
	const UWorld* World = GetWorld();
	return World && World->IsGameWorld();
}

TStatId UObjectiveHighlightSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UObjectiveHighlightSubsystem, STATGROUP_Tickables);
}

// ---------------------------------------------------------------------------
// Looking
// ---------------------------------------------------------------------------

void UObjectiveHighlightSubsystem::Advance(float DeltaSeconds)
{
	SinceRefresh += DeltaSeconds;
	SinceGather += DeltaSeconds;
	if (bDirty || SinceGather >= GatherInterval)
	{
		Refresh();
	}
	else if (SinceRefresh >= RefreshInterval)
	{
		LookAround();
	}
	// Per frame only while something is lit or easing.
	if (!Highlights.IsEmpty())
	{
		Fade(DeltaSeconds);
		Reclaim();
		MoveMarkers(DeltaSeconds);
	}
}

void UObjectiveHighlightSubsystem::Refresh()
{
	SinceGather = 0.f;
	bDirty = false;
	Candidates.Reset();
	if (UMissionRunner* Runner = ActiveRunner())
	{
		BindMissions(*Runner);
		Gather(*Runner);
	}
	LookAround();
}

UMissionRunner* UObjectiveHighlightSubsystem::ActiveRunner() const
{
	UMissionRunner* Runner = UMissionRunner::Get(this);
	return Runner && Runner->IsActive() ? Runner : nullptr;
}

void UObjectiveHighlightSubsystem::Gather(const UMissionRunner& Runner)
{
	const FName Tracked = Runner.GetTrackedMission();
	if (Tracked.IsNone())
	{
		return;
	}
	TArray<AActor*> Found;
	ObjectiveHighlightTargets::Gather(Runner, Tracked, Found);
	for (AActor* Each : Found)
	{
		Candidates.Add(Each);
	}
}

void UObjectiveHighlightSubsystem::LookAround()
{
	SinceRefresh = 0.f;
	// Each look decides again; what isn't wanted any more fades out (Fade).
	for (FObjectiveHighlight& Each : Highlights)
	{
		Each.bWanted = false;
	}
	const UMissionRunner* Runner = ActiveRunner();
	const TOptional<FVector> Player = Runner ? Runner->GetPlayerLocation() : TOptional<FVector>();
	if (!Player.IsSet() || IsCovered())
	{
		return;
	}
	struct FInRange
	{
		AActor* Actor = nullptr;
		FVector Focus = FVector::ZeroVector;
		double Distance = 0.0;
	};
	TArray<FInRange, TInlineAllocator<16>> InRange;
	const double Radius = GetRadius();
	for (const TWeakObjectPtr<AActor>& Weak : Candidates)
	{
		AActor* Candidate = Weak.Get();
		// Asked again at every look, not only as they're gathered: an item taken, a sac burst, a poster torn go dark at once.
		if (!Candidate || !ObjectiveHighlightTargets::CanLight(*Candidate))
		{
			continue;
		}
		const FVector Focus = FocusOf(*Candidate);
		const double Distance = FVector::Dist(Focus, *Player);
		FObjectiveHighlight* Known = FindMutable(Candidate);
		const bool bIn = IsWithinRange(Distance, Known && Known->bInRange, Radius);
		if (Known)
		{
			Known->bInRange = bIn;
			Known->Focus = Focus;
			Known->Distance = Distance;
		}
		if (bIn)
		{
			InRange.Add({ Candidate, Focus, Distance });
		}
	}
	InRange.Sort([](const FInRange& A, const FInRange& B) { return A.Distance < B.Distance; });
	for (int32 Index = 0; Index < FMath::Min(InRange.Num(), MaxLit); ++Index)
	{
		const FInRange& Near = InRange[Index];
		FObjectiveHighlight* Highlight = FindMutable(Near.Actor);
		if (!Highlight)
		{
			Highlight = &Highlights.AddDefaulted_GetRef();
			Highlight->Actor = Near.Actor;
		}
		Highlight->Focus = Near.Focus;
		Highlight->Distance = Near.Distance;
		Highlight->bInRange = true;
		Highlight->bWanted = true;
		if (!Highlight->bLit)
		{
			Light(*Highlight);
		}
		else
		{
			// A spot follows its point (a carried thing, a door that swings).
			ShowStrength(*Highlight);
		}
	}
}

void UObjectiveHighlightSubsystem::Fade(float DeltaSeconds)
{
	for (int32 Index = Highlights.Num() - 1; Index >= 0; --Index)
	{
		FObjectiveHighlight& Highlight = Highlights[Index];
		const AActor* Actor = Highlight.Actor.Get();
		const float Before = Highlight.Strength;
		Highlight.Strength = StepStrength(Before, Highlight.bWanted, DeltaSeconds);
		// Gone (taken, destroyed), or faded all the way out: its meshes as they were, its instances back in the pool.
		if (!Actor || Actor->IsActorBeingDestroyed() || (!Highlight.bWanted && Highlight.Strength <= 0.f))
		{
			Unlight(Highlight);
			Highlights.RemoveAtSwap(Index);
			continue;
		}
		if (Highlight.Strength != Before)
		{
			ShowStrength(Highlight);
		}
	}
}

bool UObjectiveHighlightSubsystem::IsCovered() const
{
	if (!CVarHighlightEnabled.GetValueOnGameThread())
	{
		return true;
	}
	// A scene's shots are framed without the game's signs in them.
	const USceneSubsystem* Scenes = USceneSubsystem::Get(this);
	return Scenes && Scenes->IsPlaying();
}

void UObjectiveHighlightSubsystem::ClearAll()
{
	for (FObjectiveHighlight& Highlight : Highlights)
	{
		Unlight(Highlight);
	}
	Highlights.Reset();
	Candidates.Reset();
}

void UObjectiveHighlightSubsystem::BindMissions(UMissionRunner& Runner)
{
	// A step moving on, a mission tracked or turned in: the next frame gathers again rather than up to a second later.
	if (&Runner != BoundRunner.Get())
	{
		if (UMissionRunner* Old = BoundRunner.Get())
		{
			Old->OnChanged.Remove(RunnerHandle);
		}
		BoundRunner = &Runner;
		RunnerHandle = Runner.OnChanged.AddUObject(this, &UObjectiveHighlightSubsystem::HandleMissionsChanged);
	}
	// The Missions page tracks through the display too.
	UMissionSubsystem* Display = GetWorld() ? GetWorld()->GetSubsystem<UMissionSubsystem>() : nullptr;
	if (Display && Display != BoundDisplay.Get())
	{
		if (UMissionSubsystem* Old = BoundDisplay.Get())
		{
			Old->OnMissionsChanged.Remove(DisplayHandle);
		}
		BoundDisplay = Display;
		DisplayHandle = Display->OnMissionsChanged.AddUObject(this, &UObjectiveHighlightSubsystem::HandleMissionsChanged);
	}
}

void UObjectiveHighlightSubsystem::HandleMissionsChanged()
{
	bDirty = true;
}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------

const FObjectiveHighlight* UObjectiveHighlightSubsystem::FindHighlight(const AActor* Actor) const
{
	return Actor ? Highlights.FindByPredicate([Actor](const FObjectiveHighlight& Each) { return Each.Actor.Get() == Actor; }) : nullptr;
}

FObjectiveHighlight* UObjectiveHighlightSubsystem::FindMutable(const AActor* Actor)
{
	return Actor ? Highlights.FindByPredicate([Actor](const FObjectiveHighlight& Each) { return Each.Actor.Get() == Actor; }) : nullptr;
}

TArray<AActor*> UObjectiveHighlightSubsystem::GetCandidates() const
{
	TArray<AActor*> Found;
	for (const TWeakObjectPtr<AActor>& Weak : Candidates)
	{
		if (AActor* Candidate = Weak.Get())
		{
			Found.Add(Candidate);
		}
	}
	return Found;
}

// ---------------------------------------------------------------------------
// The rules
// ---------------------------------------------------------------------------

float UObjectiveHighlightSubsystem::GetRadius()
{
	const float Tuned = CVarHighlightRadius.GetValueOnGameThread();
	return Tuned > 0.f ? Tuned : DefaultRadius;
}

bool UObjectiveHighlightSubsystem::IsWithinRange(double Distance, bool bWasInRange, double Radius)
{
	return Distance <= (bWasInRange ? Radius + RangeMargin : Radius);
}

float UObjectiveHighlightSubsystem::StepStrength(float Strength, bool bWanted, float DeltaSeconds)
{
	const float Rate = 1.f / (bWanted ? FadeInSeconds : FadeOutSeconds);
	const float Step = Rate * FMath::Max(DeltaSeconds, 0.f);
	return FMath::Clamp(bWanted ? Strength + Step : Strength - Step, 0.f, 1.f);
}

float UObjectiveHighlightSubsystem::ShownStrength(float Strength)
{
	// Eased at both ends, so it swells in and settles rather than switching on.
	const float T = FMath::Clamp(Strength, 0.f, 1.f);
	return T * T * (3.f - 2.f * T);
}

FVector UObjectiveHighlightSubsystem::FocusOf(const AActor& Actor)
{
	if (const IInteractable* Usable = Cast<IInteractable>(&Actor))
	{
		const TOptional<FVector> Point = Usable->GetInteractionLocation();
		if (Point.IsSet())
		{
			return Point.GetValue();
		}
	}
	// The middle of what it shows: an egg sac hangs far over its pivot, a gun lies across a rack.
	FBox Shown(ForceInit);
	TInlineComponentArray<UMeshComponent*> Meshes(&Actor);
	for (const UMeshComponent* Mesh : Meshes)
	{
		if (Mesh && Mesh->IsRegistered() && Mesh->IsVisible() && Mesh->Bounds.SphereRadius > 1.0 && !Mesh->IsA<UInstancedStaticMeshComponent>())
		{
			Shown += Mesh->Bounds.GetBox();
		}
	}
	return Shown.IsValid ? Shown.GetCenter() : Actor.GetActorLocation();
}

// ---------------------------------------------------------------------------
// Console
// ---------------------------------------------------------------------------

#if !UE_BUILD_SHIPPING
namespace
{
	/** The game world a command is for: the one it was typed in, or the running play-in-editor session. */
	UWorld* FindHighlightWorld(UWorld* World)
	{
		if (World && World->IsGameWorld())
		{
			return World;
		}
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.World() && Context.World()->IsGameWorld())
			{
				return Context.World();
			}
		}
		return nullptr;
	}

	void ListHighlights(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* Played = FindHighlightWorld(World);
		UObjectiveHighlightSubsystem* Highlighter = UObjectiveHighlightSubsystem::Get(Played);
		const UMissionRunner* Runner = UMissionRunner::Get(Played);
		if (!Highlighter || !Runner)
		{
			UE_LOG(LogLooter, Display, TEXT("Highlight: no game is running."));
			return;
		}
		Highlighter->Refresh();
		const TOptional<FVector> Player = Runner->GetPlayerLocation();
		const TArray<AActor*> Found = Highlighter->GetCandidates();
		UE_LOG(LogLooter, Display, TEXT("Highlight: tracking %s, radius %.1f m, material %s; %d target(s), %d lit or easing."),
			Runner->GetTrackedMission().IsNone() ? TEXT("nothing") : *Runner->GetTrackedMission().ToString(),
			UObjectiveHighlightSubsystem::GetRadius() / 100.f, Highlighter->HasMaterial() ? TEXT("loaded") : TEXT("MISSING"),
			Found.Num(), Highlighter->GetHighlights().Num());
		for (const AActor* Target : Found)
		{
			const FObjectiveHighlight* Lit = Highlighter->FindHighlight(Target);
			const double Distance = Player.IsSet() ? FVector::Dist(UObjectiveHighlightSubsystem::FocusOf(*Target), *Player) : -100.0;
			int32 Spots = 0;
			int32 Figures = 0;
			if (Lit)
			{
				for (const FObjectiveHighlightMesh& Mesh : Lit->Meshes)
				{
					Spots += Mesh.Fit == EObjectiveHighlightFit::Spot ? 1 : 0;
					Figures += Mesh.Fit == EObjectiveHighlightFit::Figure ? 1 : 0;
				}
			}
			UE_LOG(LogLooter, Display, TEXT("  %s at %.1f m: %s"), *Target->GetActorNameOrLabel(), Distance / 100.0,
				Lit ? *FString::Printf(TEXT("strength %.2f%s, %d mesh(es) whole, %d as a spot, %d as a figure%s"), Lit->Strength,
					Lit->bWanted ? TEXT("") : TEXT(" (fading)"), Lit->Meshes.Num() - Spots - Figures, Spots, Figures,
					Lit->Marker.IsValid() ? TEXT(", marked") : TEXT("")) : TEXT("not lit"));
		}
	}

	FAutoConsoleCommandWithWorldAndArgs HighlightListCommand(TEXT("Looter.Highlight.List"),
		TEXT("The tracked objective's highlight: what it's about, how far, what's lit and on which meshes."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ListHighlights));
}
#endif
