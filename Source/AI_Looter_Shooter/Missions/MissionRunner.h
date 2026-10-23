#pragma once

#include "CoreMinimal.h"
#include "Missions/MissionObjective.h"
#include "Missions/MissionRewards.h"
#include "Session/CampaignRecord.h"
#include "Subsystems/WorldSubsystem.h"
#include "UObject/ObjectKey.h"
#include "MissionRunner.generated.h"

class AActor;
class AController;
class UMissionActorWatch;
class UMissionDefinition;
class UMissionSubsystem;
struct FMissionItemInfo;
struct FMissionTrackerParts;

/** Where a mission stands for this session. */
UENUM(BlueprintType)
enum class EMissionStatus : uint8
{
	/** Its prerequisites aren't finished (or it starts only from code and hasn't). */
	Locked,
	/**
	 * Ready: in another area's level, or waiting for its start event; a mission picked up from a giver (Offered) whose
	 * prerequisites are finished and that hasn't been accepted (UMissionRunner::IsAvailable: offered here, now).
	 */
	Available,
	/**
	 * Being played: running here, or the campaign's main mission waiting in its own area; a mission ready to turn in too
	 * (UMissionRunner::IsReadyToTurnIn tells it apart).
	 */
	Active,
	/** Finished: turned in, or done by itself (an automatic one). */
	Completed,
};

/**
 * Something a mission list shows changed: a mission started or finished, a step, progress, tracking, and so what's
 * offered (a prerequisite finished, a mission accepted).
 */
DECLARE_MULTICAST_DELEGATE(FOnMissionRunnerChanged);

/** A mission was finished; bRewarded is false when it had been finished before (played again: no second reward). */
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnMissionFinished, const UMissionDefinition& /*Mission*/, bool /*bRewarded*/);

/**
 * A mission was finished (turned in, or done by itself), with what it gave: empty when it gave nothing (finished before).
 * The HUD's mission-complete banner announces it. Comes right after OnMissionFinished.
 */
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnMissionCompleted, const UMissionDefinition& /*Mission*/, const FMissionRewardsGiven& /*Given*/);

/** An event the missions heard, passed on once they have settled (UEncounterSubsystem starts waves on it). */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnMissionEventSent, const FMissionEvent& /*Event*/);

/** One objective of a running mission's current step, for the Missions page. */
struct FMissionObjectiveView
{
	/** Its words, with the player's keys in them. */
	FString Text;
	/** "2 / 4"; empty when it counts a single thing. */
	FString Progress;
	/** The count that finishes it (a bar's segments). */
	int32 Required = 1;
	float Fraction = 0.f;
	bool bDone = false;
	/** Optional (UMissionObjective::bOptional): the step goes on without it. */
	bool bOptional = false;
};

/**
 * Plays the missions (UMissionDefinition assets) in this level: one per world. It starts the ones that are due (their
 * prerequisites finished, in their area's level), follows the current step of each running mission (its objectives'
 * progress, from the player's spot, the level's deaths and hits, and the events the game sends), moves on to the next
 * step when every objective of one is done, and after the last waits for the turn-in: the mission is "ready to turn in"
 * (its step one past its last, so the story's "from step" conditions for that last talk still hold), and talking to its
 * giver (a Talk event about an actor carrying its TurnIn.SpeakerTag) finishes it: its rewards (once per session), its
 * record. An automatic one finishes after its last step at once. It feeds each running mission's title, current objective
 * (or its turn-in) and waypoint to UMissionSubsystem, so the minimap's arrow and the Missions page follow it, and keeps
 * the session's campaign record (completed missions, the ones ready to turn in, the main mission being played and its
 * step), which a session saves and the next level starts from.
 *
 * A mission picked up from a giver (EMissionStart::Offered) is offered, not started, once it's due: available at its giver
 * (a speaker point, a notice board, a mission artifact), it starts when the player accepts it on the offer card
 * (AcceptMission) and from then on in every level of its area, as an automatic one does. A main mission is never offered
 * (the user's call, 2026-10-22): it's taken and started the moment it's due, tracked, its giver's words said on the move;
 * one whose giver must be met first (FMissionOffer::bMeetFirst) is listed and tracked as "Talk to <giver>" until the talk
 * that starts it (MissionRunnerMeetings.cpp).
 *
 * Optional objectives (UMissionObjective::bOptional) never hold a step and pay a bonus at the turn-in; mission items
 * (FMissionItemInfo) are carried in the campaign while their mission runs; a turn-in may ask the player to choose between
 * two rewards, each setting a story flag (FMissionTurnIn::Choices).
 *
 * Kills and hits come from the health components of every actor in the level, ones spawned later too
 * (UWorld::AddOnActorSpawnedHandler). Nothing runs behind the main menu. Tests use an editor preview world, where it
 * waits for BeginForTesting.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UMissionRunner : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Seconds between looks at the player (places, distances, what they carry), as the tutorial always checked. */
	static constexpr float UpdateInterval = 0.2f;

	static UMissionRunner* Get(const UObject* WorldContextObject);

	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;

	/** It plays missions in this level (not behind the main menu, nor in an idle test world). */
	bool IsActive() const { return bActive; }

	// --- Missions known (MissionRunner.cpp) ---

	/** Every mission it knows: the project's assets, and any registered from code. */
	const TArray<TObjectPtr<UMissionDefinition>>& GetDefinitions() const { return Definitions; }

	UMissionDefinition* FindDefinition(FName MissionId) const;

	/** Adds a mission made in code (the tutorial's built-in steps when its asset is missing). Ignored when its id is known. */
	void RegisterDefinition(UMissionDefinition* Mission);

	// --- Playing them (MissionRunnerFlow.cpp) ---

	/**
	 * Starts a mission here at Step (0: its first). Unless bForce, only one that isn't running or finished, whose
	 * prerequisites are finished, in its area's level. bForce also restarts a running one at Step. False when it didn't start.
	 */
	bool StartMission(FName MissionId, int32 Step = 0, bool bForce = false);

	/** Moves a running mission to Step, its objectives from the start (one ready to turn in goes back to playing). */
	bool SetStep(FName MissionId, int32 Step);

	/**
	 * Finishes the current step of a running mission as if its objectives were done: after the last, the mission is ready
	 * to turn in (or finished, an automatic one); one ready to turn in is turned in.
	 */
	bool CompleteStep(FName MissionId);

	/**
	 * Finishes a mission, turn-in or not: a running one ends here; one not running is recorded as finished. Its rewards
	 * come with it, once per session. False when no mission has that id.
	 */
	bool CompleteMission(FName MissionId);

	/**
	 * Turns in a mission ready to turn in, as talking to its giver does: it's finished, with its rewards
	 * (MissionRunnerTurnIn.cpp, as are the turn-in queries below). False when it isn't ready here. One turned in with a
	 * choice of two takes its first here (the console, a board's screen): the player's way is TurnInWithChoice.
	 */
	bool TurnIn(FName MissionId);

	/**
	 * Turns in a mission ready to turn in with one of its two choices (0 or 1; FMissionTurnIn::Choices), as the choice card
	 * does: the choice's flag set and recorded, its rewards given in place of the mission's own (MissionRunnerRewards.cpp).
	 * False when it isn't ready here or has no such choice.
	 */
	bool TurnInWithChoice(FName MissionId, int32 Choice);

	/** The choice a mission was turned in with (0 or 1), or INDEX_NONE. */
	int32 GetTurnInChoice(FName MissionId) const;

	/** A story flag the campaign keeps (FCampaignRecord::StoryFlags: BackPay.Kept, PorchPlate.Taken). */
	bool HasFlag(FName Flag) const;

	/** Sets a story flag, saved soon after, and tells listeners. False when it already was set. */
	bool SetFlag(FName Flag);

	// --- Mission items (MissionRunnerItems.cpp) ---

	/**
	 * A mission item into the player's pocket (an AMissionItem picked up, a console command): kept by the campaign until its
	 * mission finishes, and the missions hear a Collect event about From (else the item's id as the event's tag). False when
	 * no mission has it, its mission is finished, or it's carried already.
	 */
	bool GiveItem(FName ItemId, AActor* From = nullptr);

	/** The player carries it (FCampaignRecord::CarriedItems). */
	bool IsCarrying(FName ItemId) const;

	/** The mission an item belongs to, and the item (OutItem), or null. */
	const UMissionDefinition* FindItemMission(FName ItemId, const FMissionItemInfo** OutItem = nullptr) const;

	/** A mission's items the player carries, in the order they were picked up (its Missions page's Carried list). */
	TArray<FMissionItemInfo> GetCarriedItems(FName MissionId) const;

	// --- Optional objectives (MissionRunnerOptional.cpp) ---

	/**
	 * A running mission's optional objectives still in play and their progress: the step being played's, or the last step's
	 * while it waits for its turn-in (they stay doable until it's turned in). Empty when it isn't running.
	 */
	TArray<FMissionObjectiveView> GetOptionalViews(FName MissionId) const;

	/** How many of a mission's optional objectives are done so far (each pays its bonus at the turn-in). */
	int32 CountOptionalDone(FName MissionId) const;

	/** Records a mission as finished without playing it or rewarding it (the tutorial, done before missions were data). */
	void RecordCompleted(FName MissionId);

	/** Something happened that objectives may wait for (FMissionEvent); OnEvent missions starting on it start after. */
	void NotifyEvent(const FMissionEvent& Event);

	/** Looks at the player and moves the running missions on; the tick calls it every UpdateInterval (tests call it). */
	void Update(float DeltaSeconds);

	// --- What's going on (MissionRunner.cpp) ---

	bool IsRunning(FName MissionId) const;

	/** The step a running mission is on (its step count once it's ready to turn in), or INDEX_NONE. */
	int32 GetStep(FName MissionId) const;

	EMissionStatus GetStatus(const UMissionDefinition& Mission) const;

	/** Finished: turned in, or done by itself. */
	bool IsCompleted(FName MissionId) const;

	/** Its objectives are all done and it waits for its turn-in: running here, or kept so by the campaign elsewhere. */
	bool IsReadyToTurnIn(FName MissionId) const;

	/**
	 * The mission ready to turn in to whoever Speaker is (an actor carrying its giver's speaker tag), running here; null
	 * when none. With several, the first started.
	 */
	const UMissionDefinition* FindTurnInAt(const AActor* Speaker) const;

	/** Where a mission's giver stands, nearest the player: the actor carrying its turn-in's speaker tag. Unset when it's not in the level. */
	TOptional<FVector> FindGiver(const UMissionDefinition& Mission) const;

	// --- Offers: missions picked up from their givers (MissionRunnerOffers.cpp) ---

	/**
	 * Offered at its giver now: it's picked up from a giver (UMissionDefinition::IsOffered), its prerequisites are finished,
	 * it's played in this level's area, and it hasn't been accepted, finished or started. A main mission that starts by
	 * itself (IsMainByItself) never is; one met first (IsMetFirst) is, at its giver, until they're talked to.
	 */
	bool IsAvailable(FName MissionId) const;

	/**
	 * Taken from its giver: accepted (the campaign record), or, from a save made before missions were offered, already
	 * played (the campaign's main mission, one waiting for its turn-in, or one running here).
	 */
	bool IsAccepted(FName MissionId) const;

	/** The missions Giver offers now (available, its actor carrying their giver's tag): main ones first, then by the list's order. */
	TArray<const UMissionDefinition*> GetOffersAt(const AActor* Giver) const;

	/**
	 * The player takes a mission from its giver: it's recorded accepted (FCampaignRecord::Accept), then started here; the
	 * minimap and the tracker follow it when nothing else was tracked (a main mission always). One already running is only
	 * recorded. False when it can't be taken now (finished, its prerequisites not finished, another area's, no steps).
	 */
	bool AcceptMission(FName MissionId);

	// --- Main missions met first (MissionRunnerMeetings.cpp) ---

	/**
	 * A main mission due whose giver is still to be met (UMissionDefinition::IsMetFirst, available here): on the Missions
	 * page and the tracker as "Talk to <giver>" with the arrow on them, under way (EMissionStatus::Active) though not
	 * running, so the story's "during" conditions don't hold yet. Talking to them starts it (USpeakerPointComponent::Talk).
	 */
	bool IsMeeting(FName MissionId) const;

	/** "Talk to Grandma Delia.": a meeting's objective (bShort: the tracker's line, no stop), the giver as the played hero names them. */
	static FText MeetText(const UMissionDefinition& Mission, bool bShort = false);

	/** Where a mission's offering giver stands, nearest the player: a shown actor carrying its Offer.GiverTag. Unset when none is in the level. */
	TOptional<FVector> FindOfferGiver(const UMissionDefinition& Mission) const;

	/** The actor offering Mission nearest the player (shown ones only), or null. */
	AActor* FindOfferGiverActor(const UMissionDefinition& Mission) const;

	/** Where the player stands (a test's stand-in in tests); unset while there's no one. */
	TOptional<FVector> GetPlayerLocation() const;

	bool ArePrerequisitesMet(const UMissionDefinition& Mission) const;

	/** A running mission's current step's objectives and their progress (empty when it isn't running). */
	TArray<FMissionObjectiveView> GetObjectiveViews(FName MissionId) const;

	/** The area this level is (UAreaDefinition::GetAreaId), or None. */
	FName GetAreaHere() const { return AreaHere; }

	/** The session's campaign record (kept in memory without a session; a test's own in tests). Never null. */
	FCampaignRecord& GetCampaign() const;

	/** The mission the minimap guides to, or None. */
	FName GetTrackedMission() const;

	/** Has the minimap guide to a running mission; None tracks none. */
	void TrackMission(FName MissionId);

	FOnMissionRunnerChanged OnChanged;
	FOnMissionFinished OnMissionFinished;
	FOnMissionCompleted OnMissionCompleted;
	FOnMissionEventSent OnEvent;

	// --- From the actors' health (UMissionActorWatch; MissionRunnerEvents.cpp) ---

	void HandleKill(AActor& Victim, AController* Killer);
	void HandleHit(AActor& Victim, AController* Attacker);

	// --- Tests ---

	/**
	 * Plays Missions in this (test) world instead of the project's assets: against Campaign (the test's, which must
	 * outlive the world), with Player standing in for the player's pawn, in the area InArea.
	 */
	void BeginForTesting(const TArray<UMissionDefinition*>& Missions, FCampaignRecord& Campaign, AActor* Player, FName InArea = NAME_None);

	/** The actor is watched for deaths and hits. */
	bool IsWatching(const AActor* Actor) const;

private:
	/**
	 * A mission running here: its step and that step's objectives' progress, and its line in UMissionSubsystem. Ready to
	 * turn in, its step is one past its last and every objective it asks for is done; States then still holds the last
	 * step's, so its optional objectives stay doable (StatesStep).
	 */
	struct FRun
	{
		FName Id;
		TWeakObjectPtr<UMissionDefinition> Mission;
		int32 Step = 0;
		TArray<FMissionObjectiveState> States;
		int32 BookId = INDEX_NONE;
		bool bReady = false;
	};

	// MissionRunner.cpp
	void Begin();
	FMissionContext MakeContext() const;
	FRun* FindRun(FName MissionId);
	const FRun* FindRun(FName MissionId) const;
	bool IsInArea(const UMissionDefinition& Mission) const;
	bool CanStart(const UMissionDefinition& Mission) const;

	/** A meeting's line in the display: the mission's title, "Talk to <giver>", the arrow on them. */
	struct FMeeting
	{
		FName Id;
		TWeakObjectPtr<UMissionDefinition> Mission;
		int32 BookId = INDEX_NONE;
	};

	// MissionRunnerOffers.cpp
	/**
	 * Due to start without a word from anyone: an automatic one, one picked up from its giver and already accepted, or a
	 * main mission that is never offered (IsMainByItself).
	 */
	bool StartsByItself(const UMissionDefinition& Mission) const;
	/** Records a mission played before offers existed as accepted, so an old save's mission stays taken. */
	void UpgradeAccepted(const UMissionDefinition& Mission);

	// MissionRunnerMeetings.cpp
	/** The meetings' lines in the display, added as they come due and taken away once started (or no longer due). */
	void SyncMeetings(UMissionSubsystem& Display);
	/** Main missions just started by themselves: tracked, their giver's words said on the move (StartTalks). */
	void SayStartTalks();

	// MissionRunnerFlow.cpp
	/** Starts (or restarts) a run at Step; a mission the campaign keeps ready to turn in starts ready instead. */
	void StartRun(UMissionDefinition& Mission, int32 Step);
	void BeginStep(FRun& Run);
	bool IsStepDone(const FRun& Run) const;
	/** Moves a run to its next step; false when that was its last (the mission is finished, or ready to turn in). */
	bool AdvanceStep(FRun& Run);
	/** Past its last step, waiting for its turn-in, recorded in the campaign: only the last step's optional objectives left. */
	void EnterReady(FRun& Run);
	/** Steps done as they begin pass at once; finished missions end, and missions they unlock start. */
	void Settle();
	/** Ends a run, finished with Choice (one of its turn-in's two; INDEX_NONE: none). */
	void FinishRun(FName MissionId, int32 Choice = INDEX_NONE);
	/**
	 * Records Mission finished and, the first time, gives its rewards (into OutGiven), with Choice's in place of its own.
	 * Its items, drops and optional records go with it. True when it was the first time.
	 */
	bool RecordCompletion(const UMissionDefinition& Mission, bool bGiveRewards, FMissionRewardsGiven* OutGiven = nullptr,
		int32 Choice = INDEX_NONE);
	void RecordStep(const FRun& Run);
	void ResumeRecorded();
	void StartDue(FName StartedBy);
	void AskToSave() const;

	// MissionRunnerRewards.cpp
	/** What finishing Mission gives, with Choice's rewards in place of its own (INDEX_NONE: its own), given now. */
	FMissionRewardsGiven GrantRewards(const UMissionDefinition& Mission, int32 Choice);
	/** The player's level for rewards (1 without a player). */
	int32 GetPlayerLevel() const;
	/** Forgets what the campaign kept for a mission under way: its items carried and dropped, its optional objectives done. */
	void ForgetMissionThings(const UMissionDefinition& Mission);

	// MissionRunnerItems.cpp
	/** A creature carrying a mission item died: the item falls at its body (FMissionItemInfo::DroppedBy). */
	void DropCarriedItems(AActor& Victim);
	/** Items their carrier dropped that lie nowhere now (a level left before they were picked up): given. */
	void RecoverLostItems();

	// MissionRunnerOptional.cpp
	/** The step whose objectives a run's States hold: its step, or its last while it waits for its turn-in. */
	int32 StatesStep(const FRun& Run) const;
	/** An optional objective of a run just done: kept by the campaign and saved soon. */
	void NoteOptionalDone(const FRun& Run, int32 Index);
	/** A ready run's States as the last step's: what it asks for done, its optional objectives as the campaign kept them. */
	void PrepareReadyStates(FRun& Run);
	/** Fills a run's optional objectives into the tracker's parts. */
	void FillOptionalParts(const FRun& Run, FMissionTrackerParts& Tracker) const;

	// MissionRunnerTurnIn.cpp
	/** A talk at a speaker turns in every mission ready to turn in to them (not one with a choice to make). True when one was. */
	bool TurnInAt(const FMissionEvent& Event);
	/** Tells listeners a mission ended, with what it gave. */
	void BroadcastFinished(const UMissionDefinition& Mission, bool bRewarded, const FMissionRewardsGiven& Given);
	/** A ready run's line in the display: its turn-in, the arrow on its giver. */
	void SyncTurnIn(const FRun& Run, const UMissionDefinition& Mission, UMissionSubsystem& Display) const;

	// MissionRunnerEvents.cpp
	void WatchActor(AActor* Actor);
	void WatchExisting();
	void HandleActorSpawned(AActor* Actor);
	void PruneWatches();
	/** True when the objective's progress or done state changed. */
	bool RefreshDone(const UMissionObjective& Objective, const FMissionContext& Context, FMissionObjectiveState& State) const;
	/** Gives every running objective a chance at something that happened; Visit returns true when it counted. */
	void ForEachRunningObjective(TFunctionRef<bool(const UMissionObjective&, FMissionObjectiveState&)> Visit);
	/** Puts the running missions' titles, objectives and waypoints in UMissionSubsystem. */
	void SyncDisplay();
	void RemoveFromDisplay(FRun& Run);
	/** After anything that could have changed something: settle the missions, update the display, tell listeners. */
	void AfterChange();

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMissionDefinition>> Definitions;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMissionActorWatch>> Watches;

	/** The actors watched, to watch each once. */
	TSet<FObjectKey> WatchedActors;

	TArray<FRun> Runs;

	/** Main missions due whose giver is still to be met, as the display shows them (IsMeeting decides; this is their lines). */
	TArray<FMeeting> Meetings;

	/** Main missions started by themselves since the last change was settled: their words and tracking come after it. */
	TArray<FName> StartTalks;

	/** Kept here when there's no session to keep it (a level played with another game mode). */
	mutable FCampaignRecord LocalCampaign;

	/** A test's record, and its stand-in for the player. */
	FCampaignRecord* TestCampaign = nullptr;
	TWeakObjectPtr<AActor> TestPlayer;

	FName AreaHere;
	bool bActive = false;
	/** The first update resumes the campaign's mission and starts the due ones, once the level is up. */
	bool bStartPending = false;
	/** Something a list shows changed since listeners were last told. */
	bool bChanged = false;
	float SinceUpdate = 0.f;
	FDelegateHandle SpawnHandle;
};
