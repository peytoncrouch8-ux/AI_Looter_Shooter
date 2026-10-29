#include "World/FindSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Core/LooterMenuGameMode.h"
#include "Loot/Chest.h"
#include "Missions/MissionObjective.h"
#include "Missions/MissionRunner.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "Session/CampaignRecord.h"
#include "World/FindGlints.h"
#include "World/FindSpot.h"
#include "World/ReadableProp.h"
#include "World/RoostCrows.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

// UFindSubsystem's spine: the spots, what's found, the legacy pass, what it listens to and its 4 Hz look. The moment of a
// find (banner, rewards, XP) is FindSubsystemMoment.cpp; the tells (crows, glints) FindSubsystemTells.cpp.

namespace
{
	/** An empty record for counting with no runner (nothing is found). */
	const FCampaignRecord& NoRecord()
	{
		static const FCampaignRecord Empty;
		return Empty;
	}
}

UFindSubsystem* UFindSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World ? World->GetSubsystem<UFindSubsystem>() : nullptr;
}

void UFindSubsystem::BannerDone(const UObject* WorldContextObject, int32 Shows)
{
	if (UFindSubsystem* Finds = Get(WorldContextObject))
	{
		Finds->FinishShows(FMath::Max(Shows, 1));
	}
}

bool UFindSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	// Editor preview worlds too, for the automated tests: there it waits for BeginForTesting.
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::EditorPreview;
}

void UFindSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	// Played worlds only, never behind the main menu. The spots begin play after this and register themselves.
	if (bActive || !InWorld.IsGameWorld() || ALooterMenuGameMode::IsMenuWorld(&InWorld))
	{
		return;
	}
	Begin(nullptr);
}

void UFindSubsystem::BeginForTesting(AActor* Player)
{
	bTesting = true;
	Begin(Player);
	// A test's world is whole when it starts it: the old save's credit at once, then the watch.
	RunLegacyPass();
}

void UFindSubsystem::Begin(AActor* InTestPlayer)
{
	if (bActive)
	{
		return;
	}
	bActive = true;
	TestPlayer = InTestPlayer;
	Clock = 0.f;
	SincePoll = 0.f;
	if (UMissionRunner* Runner = GetRunner())
	{
		EventHandle = Runner->OnEvent.AddUObject(this, &UFindSubsystem::HandleEvent);
	}
	if (UPlayerProgressionSubsystem* Progression = GetProgression())
	{
		KillHandle = Progression->OnKillXP.AddUObject(this, &UFindSubsystem::HandleKillXP);
		BoundProgression = Progression;
	}
}

void UFindSubsystem::Deinitialize()
{
	// Leaving the level with a banner still up: its experience is paid now rather than lost (its lines aren't said).
	for (const FPendingShow& Show : Pending)
	{
		PayXp(Show.Xp);
	}
	Pending.Reset();
	if (UMissionRunner* Runner = GetRunner())
	{
		Runner->OnEvent.Remove(EventHandle);
	}
	if (UPlayerProgressionSubsystem* Progression = BoundProgression.Get())
	{
		Progression->OnKillXP.Remove(KillHandle);
	}
	// The crows and the glints' actor go with the world itself: nothing is destroyed while it tears down.
	Glints = nullptr;
	States.Reset();
	bActive = false;
	bWatching = false;
	Super::Deinitialize();
}

bool UFindSubsystem::IsTickable() const
{
	return bActive && !bTesting && !IsTemplate();
}

TStatId UFindSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UFindSubsystem, STATGROUP_Tickables);
}

void UFindSubsystem::Tick(float DeltaTime)
{
	Advance(DeltaTime);
}

void UFindSubsystem::Advance(float DeltaSeconds)
{
	if (!bActive)
	{
		return;
	}
	Clock += DeltaSeconds;
	// The session's world is back (opened chests open again) by now: an old save's finds are credited before anything is
	// announced, so loading one never storms banners.
	if (!bWatching && Clock >= FindRules::LegacyDelay)
	{
		RunLegacyPass();
	}
	UpdatePending(DeltaSeconds);
	if (HasLiveGlints())
	{
		UpdateGlints(DeltaSeconds);
	}
	SincePoll += DeltaSeconds;
	if (SincePoll >= FindRules::PollInterval)
	{
		const float Elapsed = SincePoll;
		SincePoll = 0.f;
		Poll();
		UpdateLeavingCrows(Elapsed);
		PollGlints(Elapsed);
	}
}

// ---------------------------------------------------------------------------
// Spots
// ---------------------------------------------------------------------------

void UFindSubsystem::RegisterSpot(AFindSpot* Spot)
{
	if (!Spot || Spot->FindId.IsNone() || States.ContainsByPredicate([Spot](const FSpotState& Each) { return Each.Spot.Get() == Spot; }))
	{
		return;
	}
	if (FindSpot(Spot->FindId))
	{
		UE_LOG(LogLooter, Warning, TEXT("Finds: %s is placed twice (%s): the second is left out."), *Spot->FindId.ToString(), *Spot->GetName());
		return;
	}
	FSpotState& State = States.AddDefaulted_GetRef();
	State.Spot = Spot;
	State.GlintRandom.Initialize(FindRules::SeedFor(Spot->FindId));
	// The first star comes a random part of a wait in, so the level's glints never flash together.
	State.NextGlint = State.GlintRandom.FRandRange(0.f, FindRules::GlintEveryMax);
	Spot->SetTellsShown(!IsFound(Spot->FindId));
	if (bWatching)
	{
		UpdateTellsFor(State);
	}
}

void UFindSubsystem::UnregisterSpot(AFindSpot* Spot)
{
	const int32 Index = States.IndexOfByPredicate([Spot](const FSpotState& Each) { return Each.Spot.Get() == Spot; });
	if (Index == INDEX_NONE)
	{
		return;
	}
	FSpotState& State = States[Index];
	for (TWeakObjectPtr<ARoostCrows>* Flock : { &State.Crows, &State.LeavingCrows })
	{
		if (ARoostCrows* Birds = Flock->Get())
		{
			Birds->Destroy();
		}
	}
	States.RemoveAt(Index);
}

TArray<AFindSpot*> UFindSubsystem::GetSpots() const
{
	TArray<AFindSpot*> Spots;
	for (const FSpotState& State : States)
	{
		if (AFindSpot* Spot = State.Spot.Get())
		{
			Spots.Add(Spot);
		}
	}
	return Spots;
}

AFindSpot* UFindSubsystem::FindSpot(FName FindId) const
{
	const FSpotState* State = FindState(FindId);
	return State ? State->Spot.Get() : nullptr;
}

UFindSubsystem::FSpotState* UFindSubsystem::FindState(FName FindId)
{
	return States.FindByPredicate([FindId](const FSpotState& Each) { return Each.Spot.IsValid() && Each.Spot->FindId == FindId; });
}

const UFindSubsystem::FSpotState* UFindSubsystem::FindState(FName FindId) const
{
	return States.FindByPredicate([FindId](const FSpotState& Each) { return Each.Spot.IsValid() && Each.Spot->FindId == FindId; });
}

// ---------------------------------------------------------------------------
// What's found
// ---------------------------------------------------------------------------

UMissionRunner* UFindSubsystem::GetRunner() const
{
	return GetWorld() ? GetWorld()->GetSubsystem<UMissionRunner>() : nullptr;
}

FCampaignRecord* UFindSubsystem::GetCampaign() const
{
	// The runner's record: the session's (saved with it), a play's own without one, a test's in tests.
	UMissionRunner* Runner = GetRunner();
	return Runner ? &Runner->GetCampaign() : nullptr;
}

bool UFindSubsystem::IsFound(FName FindId) const
{
	const FCampaignRecord* Campaign = GetCampaign();
	return Campaign && Campaign->HasFlag(FindRules::FlagFor(FindId));
}

TArray<FFindEntry> UFindSubsystem::GatherEntries() const
{
	TArray<FFindEntry> Entries;
	for (const FSpotState& State : States)
	{
		if (const AFindSpot* Spot = State.Spot.Get())
		{
			Entries.Add(Spot->ToEntry());
		}
	}
	return Entries;
}

FFindTally UFindSubsystem::GetTally() const
{
	const FCampaignRecord* Campaign = GetCampaign();
	return FindRules::Tally(GatherEntries(), Campaign ? *Campaign : NoRecord(), /*bLore*/ false);
}

FFindTally UFindSubsystem::GetLoreTally() const
{
	const FCampaignRecord* Campaign = GetCampaign();
	return FindRules::Tally(GatherEntries(), Campaign ? *Campaign : NoRecord(), /*bLore*/ true);
}

bool UFindSubsystem::MarkFound(FName FindId, bool bAnnounce)
{
	AFindSpot* Spot = FindSpot(FindId);
	return Spot && Found(*Spot, bAnnounce, Spot->GetSourceActor());
}

bool UFindSubsystem::Forget(FName FindId)
{
	FCampaignRecord* Campaign = GetCampaign();
	AFindSpot* Spot = FindSpot(FindId);
	if (!Campaign || !Spot || !Campaign->ClearFlag(FindRules::FlagFor(FindId)))
	{
		return false;
	}
	Spot->SetTellsShown(true);
	if (FSpotState* State = FindState(FindId))
	{
		UpdateTellsFor(*State);
	}
	return true;
}

int32 UFindSubsystem::ForgetAll()
{
	int32 Forgotten = 0;
	for (AFindSpot* Spot : GetSpots())
	{
		Forgotten += Forget(Spot->FindId) ? 1 : 0;
	}
	return Forgotten;
}

// ---------------------------------------------------------------------------
// The legacy pass and the look
// ---------------------------------------------------------------------------

void UFindSubsystem::RunLegacyPass()
{
	if (bWatching)
	{
		return;
	}
	const FCampaignRecord* Campaign = GetCampaign();
	int32 Credited = 0;
	// What an old save already holds counts, silently: its secret chests opened before finds existed, its pages read.
	for (AFindSpot* Spot : GetSpots())
	{
		if (IsFound(Spot->FindId))
		{
			continue;
		}
		const AActor* Source = Spot->GetSourceActor();
		const AChest* Chest = Cast<AChest>(Source);
		const AReadableProp* Readable = Cast<AReadableProp>(Source);
		const bool bOpened = Chest && Chest->IsUsedUp();
		const bool bRead = Readable && Campaign && Campaign->HasFlag(Readable->SetsFlag);
		if ((bOpened || bRead) && Found(*Spot, /*bAnnounce*/ false, Source))
		{
			++Credited;
		}
	}
	if (Credited > 0)
	{
		UE_LOG(LogLooter, Log, TEXT("Finds: %d found before (an older save), counted silently."), Credited);
	}
	bWatching = true;
	StartTells();
}

void UFindSubsystem::Poll()
{
	if (!bWatching)
	{
		return;
	}
	// A chest counts once its loot is out, so the banner follows the lid (and a save taken mid-swing keeps it unfound, as it
	// keeps the chest closed). All of them are looked at, so one opened from the console counts as well.
	for (AFindSpot* Spot : GetSpots())
	{
		AChest* Chest = Cast<AChest>(Spot->GetSourceActor());
		if (Chest && Chest->HasGivenLoot() && !IsFound(Spot->FindId))
		{
			Found(*Spot, /*bAnnounce*/ true, Chest);
		}
	}
}

void UFindSubsystem::HandleEvent(const FMissionEvent& Event)
{
	NotifyEvent(Event);
}

void UFindSubsystem::NotifyEvent(const FMissionEvent& Event)
{
	if (!bActive)
	{
		return;
	}
	const AActor* About = Event.Actor.Get();
	for (AFindSpot* Spot : GetSpots())
	{
		if (IsFound(Spot->FindId))
		{
			continue;
		}
		const AActor* Source = Spot->GetSourceActor();
		// Chests are looked at by their lids (Poll), never on the tap.
		const bool bChest = Cast<AChest>(Source) != nullptr;
		const AReadableProp* Readable = Cast<AReadableProp>(Source);
		const bool bUsed = !bChest && Source && About == Source
			&& (Event.Name == FMissionEvent::Interact || (Readable && Event.Name == Readable->GetReadEvent()));
		const bool bNamed = !Spot->SourceEvent.IsNone() && Event.Name == Spot->SourceEvent;
		if (bUsed || bNamed)
		{
			Found(*Spot, /*bAnnounce*/ true, Source ? Source : About);
		}
	}
}

void UFindSubsystem::HandleKillXP(int64 XP, const AActor* Victim)
{
	NotifyKill(Victim);
}

void UFindSubsystem::NotifyKill(const AActor* Victim)
{
	if (!bActive || !Victim)
	{
		return;
	}
	for (AFindSpot* Spot : GetSpots())
	{
		if (!Spot->SourceTag.IsNone() && Victim->ActorHasTag(Spot->SourceTag) && !IsFound(Spot->FindId))
		{
			Found(*Spot, /*bAnnounce*/ true, Victim);
		}
	}
}

// ---------------------------------------------------------------------------
// The player
// ---------------------------------------------------------------------------

AActor* UFindSubsystem::GetPlayer() const
{
	if (bTesting)
	{
		return TestPlayer.Get();
	}
	const UWorld* World = GetWorld();
	const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	return Controller ? Controller->GetPawn() : nullptr;
}

bool UFindSubsystem::GetView(FVector& OutLocation, FVector& OutForward) const
{
	const UWorld* World = GetWorld();
	const APlayerController* Controller = (World && !bTesting) ? World->GetFirstPlayerController() : nullptr;
	if (Controller)
	{
		FRotator Rotation;
		Controller->GetPlayerViewPoint(OutLocation, Rotation);
		OutForward = Rotation.Vector();
		return true;
	}
	// A test's stand-in: its eyes at its place, looking where it faces.
	const AActor* Player = GetPlayer();
	if (!Player)
	{
		return false;
	}
	FRotator Rotation;
	Player->GetActorEyesViewPoint(OutLocation, Rotation);
	OutForward = Rotation.Vector();
	return true;
}

UPlayerProgressionSubsystem* UFindSubsystem::GetProgression() const
{
	if (bTesting)
	{
		return nullptr;
	}
	const UWorld* World = GetWorld();
	const UGameInstance* Game = World ? World->GetGameInstance() : nullptr;
	const ULocalPlayer* Local = Game ? Game->GetFirstGamePlayer() : nullptr;
	return Local ? Local->GetSubsystem<UPlayerProgressionSubsystem>() : nullptr;
}
