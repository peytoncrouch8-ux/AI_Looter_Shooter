#include "UI/World/GiverMarkSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Interaction/Interactable.h"
#include "Interaction/InteractionSubsystem.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionRunner.h"
#include "Story/SpeakerPointComponent.h"
#include "UI/World/GiverMarkActor.h"
#include "World/MissionArtifact.h"
#include "World/NoticeBoard.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

namespace
{
	/** How far over the speaker point (about a door's middle, or a character's mouth) a speaker's mark floats (cm). */
	constexpr double OverSpeaker = 85.0;
	/** Over a board's face, past its header. */
	constexpr double OverBoard = 115.0;
	/** Over the top of an artifact lying about. */
	constexpr double OverArtifact = 45.0;
	/** Over anything else's point of use. */
	constexpr double OverOther = 85.0;
}

UGiverMarkSubsystem* UGiverMarkSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World ? World->GetSubsystem<UGiverMarkSubsystem>() : nullptr;
}

bool UGiverMarkSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	// Played worlds, and the editor preview worlds the automated tests build (there it only looks when asked).
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::EditorPreview;
}

void UGiverMarkSubsystem::Deinitialize()
{
	if (UMissionRunner* Runner = BoundRunner.Get())
	{
		Runner->OnChanged.Remove(ChangedHandle);
	}
	BoundRunner.Reset();
	ChangedHandle.Reset();
	WorldMarks.Reset();
	Marks.Reset();
	Super::Deinitialize();
}

void UGiverMarkSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Advance(DeltaTime);
}

bool UGiverMarkSubsystem::IsTickable() const
{
	// Only where someone plays: a test level asks for its looks.
	const UWorld* World = GetWorld();
	return World && World->IsGameWorld();
}

TStatId UGiverMarkSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UGiverMarkSubsystem, STATGROUP_Tickables);
}

void UGiverMarkSubsystem::Advance(float DeltaSeconds)
{
	Settled += DeltaSeconds;
	SinceRefresh += DeltaSeconds;
	if (SinceRefresh >= RefreshInterval || bDirty)
	{
		Refresh();
	}
}

void UGiverMarkSubsystem::HandleRunnerChanged()
{
	bDirty = true;
}

// ---------------------------------------------------------------------------
// The marks
// ---------------------------------------------------------------------------

void UGiverMarkSubsystem::Refresh()
{
	SinceRefresh = 0.f;
	bDirty = false;
	UMissionRunner* Runner = UMissionRunner::Get(this);
	if (!Runner || !Runner->IsActive())
	{
		Marks.Reset();
		SyncWorldMarks();
		return;
	}
	// The runner's changes (a turn-in, an acceptance) bring the next look forward, so a ! comes and goes with them.
	if (Runner != BoundRunner.Get())
	{
		if (UMissionRunner* Old = BoundRunner.Get())
		{
			Old->OnChanged.Remove(ChangedHandle);
		}
		BoundRunner = Runner;
		ChangedHandle = Runner->OnChanged.AddUObject(this, &UGiverMarkSubsystem::HandleRunnerChanged);
	}
	Marks = GatherMarks(*Runner, FindCandidates());
	Chime(*Runner);
	SyncWorldMarks();
}

TArray<AActor*> UGiverMarkSubsystem::FindCandidates() const
{
	TArray<AActor*> Found;
	const UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this);
	if (!Registry)
	{
		return Found;
	}
	for (const TWeakObjectPtr<AActor>& Each : Registry->GetInteractables())
	{
		AActor* Candidate = Each.Get();
		if (Candidate && !Candidate->IsActorBeingDestroyed() && !Candidate->IsHidden())
		{
			Found.Add(Candidate);
		}
	}
	return Found;
}

TArray<FGiverMark> UGiverMarkSubsystem::GatherMarks(const UMissionRunner& Runner, TConstArrayView<AActor*> Candidates)
{
	using MissionMarkIcons::EMark;
	TArray<FGiverMark> Found;
	for (AActor* Giver : Candidates)
	{
		if (!Giver || Giver->IsHidden())
		{
			continue;
		}
		FGiverMark Mark;
		bool bAny = false;
		for (const UMissionDefinition* Mission : Runner.GetDefinitions())
		{
			if (!Mission)
			{
				continue;
			}
			const FName MissionId = Mission->GetMissionId();
			const bool bTurnIn = Mission->NeedsTurnIn() && Giver->ActorHasTag(Mission->TurnIn.SpeakerTag) && Runner.IsRunning(MissionId)
				&& Runner.IsReadyToTurnIn(MissionId);
			const bool bOffer = Mission->IsOffered() && Giver->ActorHasTag(Mission->Offer.GiverTag) && Runner.IsAvailable(MissionId);
			if (bOffer)
			{
				Mark.Offers.Add(MissionId);
			}
			if (!bTurnIn && !bOffer)
			{
				continue;
			}
			// A ? over a ! (a talk turns in before it offers), then the story over a side job.
			const EMark Kind = bTurnIn ? EMark::TurnIn : EMark::Offer;
			const bool bMain = Mission->Kind == EMissionKind::Main;
			const bool bBetter = !bAny || (Kind == EMark::TurnIn && Mark.Mark == EMark::Offer) || (Kind == Mark.Mark && bMain && !Mark.bMain);
			if (bBetter)
			{
				Mark.Mark = Kind;
				Mark.bMain = bMain;
				Mark.MissionId = MissionId;
			}
			bAny = true;
		}
		if (bAny)
		{
			Mark.Giver = Giver;
			Mark.Location = MarkSpot(*Giver);
			Found.Add(MoveTemp(Mark));
		}
	}
	return Found;
}

FVector UGiverMarkSubsystem::MarkSpot(const AActor& Giver)
{
	const FVector Up(0.0, 0.0, 1.0);
	if (const AMissionArtifact* Artifact = Cast<AMissionArtifact>(&Giver))
	{
		if (Artifact->Mesh && Artifact->Mesh->GetStaticMesh())
		{
			const FBoxSphereBounds& Bounds = Artifact->Mesh->Bounds;
			return Bounds.Origin + Up * (Bounds.BoxExtent.Z + OverArtifact);
		}
		return Giver.GetActorLocation() + Up * (OverArtifact + 20.0);
	}
	if (const ANoticeBoard* Board = Cast<ANoticeBoard>(&Giver))
	{
		return Board->GetActorTransform().TransformPosition(Board->FaceOffset) + Up * OverBoard;
	}
	if (const USpeakerPointComponent* Point = Giver.FindComponentByClass<USpeakerPointComponent>())
	{
		return Point->GetComponentLocation() + Up * OverSpeaker;
	}
	if (const IInteractable* Usable = Cast<IInteractable>(&Giver))
	{
		if (const TOptional<FVector> At = Usable->GetInteractionLocation())
		{
			return *At + Up * OverOther;
		}
	}
	return Giver.GetActorLocation() + Up * 200.0;
}

// ---------------------------------------------------------------------------
// The chime
// ---------------------------------------------------------------------------

void UGiverMarkSubsystem::Chime(const UMissionRunner& Runner)
{
	const TOptional<FVector> Player = Runner.GetPlayerLocation();
	if (!Player.IsSet())
	{
		return;
	}
	bool bNews = false;
	for (const FGiverMark& Mark : Marks)
	{
		const AActor* Giver = Mark.Giver.Get();
		// Only as a ! is seen: offers behind a ? chime once the turn-in leaves the ! showing.
		if (!Giver || Mark.Mark != MissionMarkIcons::EMark::Offer || FVector::Dist(*Player, Giver->GetActorLocation()) > ShowRange)
		{
			continue;
		}
		for (const FName MissionId : Mark.Offers)
		{
			if (Chimed.Contains(MissionId))
			{
				continue;
			}
			// What was offered nearby as the level began was there before: known, not news.
			if (Settled < SettleSeconds)
			{
				Chimed.Add(MissionId);
				continue;
			}
			// Under a menu or a scene it waits, to chime as the ! is seen.
			if (!bLabelsShown)
			{
				continue;
			}
			Chimed.Add(MissionId);
			bNews = true;
			UE_LOG(LogLooter, Log, TEXT("Givers: %s offered nearby, at %s."), *MissionId.ToString(), *Giver->GetActorNameOrLabel());
		}
	}
	if (bNews)
	{
		++ChimeCount;
		LooterSound::Play2D(this, LooterSoundCue::Giver::NewOffer);
	}
}

// ---------------------------------------------------------------------------
// The marks over the world
// ---------------------------------------------------------------------------

void UGiverMarkSubsystem::SyncWorldMarks()
{
	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return;
	}
	WorldMarks.RemoveAll([](const TObjectPtr<AGiverMarkActor>& Each) { return !IsValid(Each); });
	TSet<const AGiverMarkActor*> Placed;
	for (const FGiverMark& Mark : Marks)
	{
		const AActor* Giver = Mark.Giver.Get();
		if (!Giver)
		{
			continue;
		}
		// The one already over this giver (fading or not), else a free one, else a new one.
		AGiverMarkActor* Over = nullptr;
		for (const TObjectPtr<AGiverMarkActor>& Each : WorldMarks)
		{
			if (Each->GetGiver() == Giver)
			{
				Over = Each.Get();
				break;
			}
		}
		for (int32 Index = 0; !Over && Index < WorldMarks.Num(); ++Index)
		{
			AGiverMarkActor* Candidate = WorldMarks[Index].Get();
			Over = Candidate->IsFree() && !Placed.Contains(Candidate) ? Candidate : nullptr;
		}
		if (!Over)
		{
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Params.ObjectFlags |= RF_Transient;
			Over = World->SpawnActor<AGiverMarkActor>(Mark.Location, FRotator::ZeroRotator, Params);
			if (!Over)
			{
				continue;
			}
			WorldMarks.Add(Over);
		}
		Over->Show(Mark);
		Placed.Add(Over);
	}
	for (const TObjectPtr<AGiverMarkActor>& Each : WorldMarks)
	{
		if (!Placed.Contains(Each.Get()) && !Each->IsFree())
		{
			Each->Retire();
		}
	}
}
