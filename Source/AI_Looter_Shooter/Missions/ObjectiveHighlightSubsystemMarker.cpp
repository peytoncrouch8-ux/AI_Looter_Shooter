// UObjectiveHighlightSubsystem: the markers over the lit papers the overlay can't be seen on (NeedsMarker,
// ObjectiveHighlightSubsystemLook.cpp says which): taken from a pool as a paper lights, moved with its highlight every
// frame while anything is lit, back to the pool hidden as it goes dark.

#include "Missions/ObjectiveHighlightSubsystem.h"
#include "UI/World/ObjectiveMarkerActor.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

namespace
{
	/** More than the lit at once (MaxLit): those fading out keep theirs a moment while new ones light. */
	constexpr int32 MaxMarkers = UObjectiveHighlightSubsystem::MaxLit * 2;
}

void UObjectiveHighlightSubsystem::ShowMarker(FObjectiveHighlight& Highlight)
{
	AActor* Actor = Highlight.Actor.Get();
	if (!Actor || Highlight.Marker.IsValid())
	{
		return;
	}
	if (AObjectiveMarkerActor* Marker = TakeMarker())
	{
		Marker->Show(Actor, AObjectiveMarkerActor::MarkPoint(*Actor, Highlight.Focus));
		Highlight.Marker = Marker;
	}
}

void UObjectiveHighlightSubsystem::HideMarker(FObjectiveHighlight& Highlight)
{
	if (AObjectiveMarkerActor* Marker = Highlight.Marker.Get())
	{
		Marker->Release();
	}
	Highlight.Marker.Reset();
}

void UObjectiveHighlightSubsystem::MoveMarkers(float DeltaSeconds)
{
	for (const FObjectiveHighlight& Highlight : Highlights)
	{
		AObjectiveMarkerActor* Marker = Highlight.Marker.Get();
		const AActor* Actor = Highlight.Actor.Get();
		if (Marker && Actor)
		{
			// Its spot asked again each frame: a paper on a door that swings carries its mark.
			Marker->Advance(DeltaSeconds, ShownStrength(Highlight.Strength), static_cast<float>(Highlight.Distance),
				AObjectiveMarkerActor::MarkPoint(*Actor, Highlight.Focus));
		}
	}
}

AObjectiveMarkerActor* UObjectiveHighlightSubsystem::TakeMarker()
{
	Markers.RemoveAll([](const TObjectPtr<AObjectiveMarkerActor>& Each) { return !IsValid(Each); });
	for (AObjectiveMarkerActor* Each : Markers)
	{
		if (Each->IsFree())
		{
			return Each;
		}
	}
	UWorld* World = GetWorld();
	if (!World || World->bIsTearingDown || Markers.Num() >= MaxMarkers)
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.ObjectFlags |= RF_Transient;
	AObjectiveMarkerActor* Made = World->SpawnActor<AObjectiveMarkerActor>(AObjectiveMarkerActor::StaticClass(), FTransform::Identity, Params);
	if (Made)
	{
		Markers.Add(Made);
	}
	return Made;
}

void UObjectiveHighlightSubsystem::DestroyMarkers()
{
	for (AObjectiveMarkerActor* Each : Markers)
	{
		if (IsValid(Each))
		{
			Each->Destroy();
		}
	}
	Markers.Reset();
}
