#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "World/FindRules.h"
#include "FindSubsystem.generated.h"

class AActor;
class AFindGlints;
class AFindSpot;
class ARoostCrows;
class UDirectionalLightComponent;
class UMissionRunner;
class UPlayerProgressionSubsystem;
struct FCampaignRecord;
struct FFindBannerShow;
struct FMissionEvent;

/** A find was made (announced or not): its id and the count after it (the area's finds; Lore: the Ledger pages'). */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnFindFound, FName /*FindId*/, int32 /*Found*/, int32 /*Total*/);

/**
 * The level's finds off the path (Docs/Work/2026-10-28-Core/FINDS.md §3): every AFindSpot registers here as play begins.
 * It hears the missions' events (a readable read or used; a named SourceEvent), the player's kills (a creature carrying an
 * Elite's SourceTag) and looks at the chests' lids 4 Hz (a chest counts once its loot is out: the banner follows the lid,
 * not the tap). A find sets the story flag Find.<Id> (saved with the session's campaign record), shows the banner
 * (ALooterHUD::ShowFind), plays World.Find.Discover, sends its crows off and hides its tell props, tosses its silver out at
 * the source and drops its named gun there once; its experience (EXPSource::Find) and Hob's lines come when the banner is
 * done (BannerDone), so LEVEL UP follows the find. The area's last: "EVERY FIND IN RANSOM'S REST", World.Find.Complete,
 * +1,000 XP and Hob's word.
 *
 * An old save's finds count silently (no banner, no XP): LegacyDelay into play, a find whose chest is used up or whose
 * readable's SetsFlag is set is marked found. The tells start after that pass: crows circling low over each unfound find
 * (ARoostCrows::MakeCircling) and a glint off its brass (AFindGlints, at most FindRules::MaxGlints at once).
 *
 * Game worlds (and PIE), never behind the main menu; editor preview worlds (the tests) wait for BeginForTesting. Looks at
 * the world 4 Hz; per frame only while a glint shows. Console: World/FindCommands.cpp (Looter.Finds.*).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UFindSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static UFindSubsystem* Get(const UObject* WorldContextObject);

	/**
	 * The HUD's find banner is done with Shows of the finds announced (UHudFindBannerWidget, as a show fades: 1, or a merged
	 * show's Merged): their experience paid and their lines played, oldest first.
	 */
	static void BannerDone(const UObject* WorldContextObject, int32 Shows = 1);

	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;

	// --- Spots (AFindSpot calls these as play begins and ends) ---

	void RegisterSpot(AFindSpot* Spot);
	void UnregisterSpot(AFindSpot* Spot);

	/** The spots registered, live ones only. */
	TArray<AFindSpot*> GetSpots() const;

	AFindSpot* FindSpot(FName FindId) const;

	// --- What's found ---

	bool IsFound(FName FindId) const;

	/** The area's finds: every spot but Lore, found against total, per region. */
	FFindTally GetTally() const;

	/** The Ledger pages (Lore spots): found against total. */
	FFindTally GetLoreTally() const;

	/**
	 * Marks a find found: its flag, and with bAnnounce the whole moment (banner, cue, silver, named gun, XP after the banner,
	 * Hob's lines, the completion when it's the last). Without, silent (the legacy pass: nothing paid). Either way its tells
	 * go. False when it isn't a spot here or was found already.
	 */
	bool MarkFound(FName FindId, bool bAnnounce);

	/** Forgets a find (the console): its flag cleared, its tells back. Its source stays as it is (an opened chest stays open). */
	bool Forget(FName FindId);

	/** Forgets every find here. Returns how many were found. */
	int32 ForgetAll();

	/** The tells on or off (Looter.Finds.Tells 0/1): crows and glints for unfound finds. */
	void SetTellsEnabled(bool bEnabled);
	bool AreTellsEnabled() const { return bTellsEnabled; }

	/** The glints showing now (never more than FindRules::MaxGlints). */
	int32 GetGlintsShowing() const;

	/** An unfound find's circling crows, or null (found, tells off, none wanted, not yet started). */
	ARoostCrows* GetCrows(FName FindId) const;

	/** The legacy pass has run: the tells and the live watch are on. */
	bool IsWatching() const { return bWatching; }

	/** A kill credited to the player (UPlayerProgressionSubsystem::OnKillXP calls it; tests call it): an Elite's tag finds it. */
	void NotifyKill(const AActor* Victim);

	/** An event the missions heard (UMissionRunner::OnEvent calls it; tests call it). */
	void NotifyEvent(const FMissionEvent& Event);

	FOnFindFound OnFound;

	// --- Tests ---

	/**
	 * Starts it in a test world, Player standing in for the player's pawn (distances, sight, rewards), against the runner's
	 * campaign record (UMissionRunner::BeginForTesting first). No HUD: a banner lasts BannerSeconds. The legacy pass runs at once.
	 */
	void BeginForTesting(AActor* Player);

	/** Moves its clock on (the tick does in play; a test level never ticks). */
	void Advance(float DeltaSeconds);

	/** The experience it has paid in all (a test world has no player to pay it to: it's still counted). */
	int64 GetXpPaid() const { return XpPaid; }

	/** Finds announced whose banner isn't done (their experience unpaid). */
	int32 GetPendingShows() const { return Pending.Num(); }

private:
	/** A find announced (or the completion), waiting for its banner to be done: what it pays then. */
	struct FPendingShow
	{
		FName FindId;
		int64 Xp = 0;
		bool bComplete = false;
		/** Seconds since it was announced: paid at BannerSeconds without a HUD, at BannerWaitLimit at the latest. */
		float Age = 0.f;
		bool bOnHud = false;
	};

	/** One spot's live state: its tells. */
	struct FSpotState
	{
		TWeakObjectPtr<AFindSpot> Spot;
		TWeakObjectPtr<ARoostCrows> Crows;
		/** Crows sent off by the find: gone once out of the player's sight (or after a while). */
		TWeakObjectPtr<ARoostCrows> LeavingCrows;
		float LeavingAge = 0.f;
		FRandomStream GlintRandom;
		float NextGlint = 0.f;
		/** A star showing: its age (s); negative: none. */
		float GlintAge = -1.f;
		bool bGlintHeard = false;
	};

	// FindSubsystem.cpp
	void Begin(AActor* InTestPlayer);
	FCampaignRecord* GetCampaign() const;
	UMissionRunner* GetRunner() const;
	AActor* GetPlayer() const;
	bool GetView(FVector& OutLocation, FVector& OutForward) const;
	UPlayerProgressionSubsystem* GetProgression() const;
	FSpotState* FindState(FName FindId);
	const FSpotState* FindState(FName FindId) const;
	TArray<FFindEntry> GatherEntries() const;
	void RunLegacyPass();
	void Poll();
	void HandleEvent(const FMissionEvent& Event);
	void HandleKillXP(int64 XP, const AActor* Victim);

	// FindSubsystemMoment.cpp
	/** The find's flag set, its tells gone, listeners told; with bAnnounce its rewards (by At: a chest, a body) and its banner. */
	bool Found(AFindSpot& Spot, bool bAnnounce, const AActor* At);
	int32 ZoneLevelOf(const AFindSpot& Spot) const;
	FText AreaName() const;
	void Announce(AFindSpot& Spot, const AActor* At);
	void GiveSilver(const AFindSpot& Spot, const FVector& Where);
	void GiveNamedGun(const AFindSpot& Spot, const AActor* At, const FVector& Where);
	/** Hands a banner to the HUD (none in a test world) and waits on it for Xp's payment. */
	void Show(const FFindBannerShow& Words, FName FindId, int64 Xp, bool bComplete);
	void UpdatePending(float DeltaSeconds);
	/** The oldest Count shows done: paid, their lines played; the completion shown once the last find's is done. */
	void FinishShows(int32 Count);
	void PayXp(int64 Xp);

	// FindSubsystemTells.cpp
	void StartTells();
	void UpdateTellsFor(FSpotState& State);
	void SendCrowsOff(FSpotState& State);
	void UpdateLeavingCrows(float DeltaSeconds);
	void PollGlints(float DeltaSeconds);
	void UpdateGlints(float DeltaSeconds);
	void DrawGlints();
	bool HasLiveGlints() const;
	bool IsSunSideOf(const FVector& Spot, const FVector& Player) const;
	bool HasClearLine(const FVector& Eyes, const FVector& Spot) const;

	TArray<FSpotState> States;
	TArray<FPendingShow> Pending;

	/** A test's stand-in for the player (BeginForTesting). */
	TWeakObjectPtr<AActor> TestPlayer;

	UPROPERTY(Transient)
	TObjectPtr<AFindGlints> Glints;

	/** The sun the glints catch (the sky's sun light, else any directional light), looked for once. */
	mutable TWeakObjectPtr<const UDirectionalLightComponent> Sun;
	mutable bool bSunLooked = false;

	TWeakObjectPtr<UPlayerProgressionSubsystem> BoundProgression;
	FDelegateHandle EventHandle;
	FDelegateHandle KillHandle;

	int64 XpPaid = 0;
	float Clock = 0.f;
	float SincePoll = 0.f;
	/** When the Discover cue last played: several finds at once (the console) ring once. */
	float LastCueAt = -100.f;
	bool bActive = false;
	bool bWatching = false;
	bool bTellsEnabled = true;
	/** A test world: no HUD, no session, the test's player. */
	bool bTesting = false;
	/** The completion banner is queued behind the last find's: its cue (World.Find.Complete) rings as it comes up. */
	bool bCompleteWaiting = false;
};
