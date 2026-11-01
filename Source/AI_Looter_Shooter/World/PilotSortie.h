#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "World/LamplighterCourse.h"
#include "World/PilotSortieRules.h"
#include "PilotSortie.generated.h"

class ABargeBoarding;
class ACompanyTender;
class ACreatureBase;
class ALamplighterLauncher;
class ALamplighterShip;
class ALandfallMarker;
class UBoxComponent;
class USceneComponent;
class UStaticMeshComponent;

/**
 * The Lamplighter's part of Main 6, "The Pilot Light" (Docs/Work/2026-10-31-Wardens/MAINS_2_7.md section 10; lane MAIN67,
 * W2): starved of both her hoses, Vane's barge has come down over the chapel at dusk to hose Saint Ada herself, the one time
 * she's in reach. The crew fly the Lamplighter at her.
 *
 *  - Step 0, cast off: the anchor winch's hold (ALandfallWinch's cast-off mode) brings the anchor up; she lifts off the ledge,
 *    turns clear of the cliff and the crew brace (Holloway: "Anchor's up. Hold on to something, Warden.").
 *  - Step 1, the tenders: her course east over the farm and the town (placed by build_area_main6_sortie.py; ~90 s with the
 *    fight). A fifth of the way out two Company tenders come up from astern: one stands off her port side while its two
 *    riflemen fire across; the other runs in alongside, drops its plank and boards two riflemen and a hose-man (level 6), then
 *    pulls off to stand off her starboard with its own riflemen. Each tender's gas-bag is its weak spot (ACompanyTender's
 *    escort mode): shot through it leaks, lists and tears, and the tender falls burning into the fields with a blast heard
 *    across the valley and a smoke column. Both down and every boarder dead sends Pilot.TendersDown. She slows to fight and,
 *    should the course run out first, hangs at its end until they're down.
 *  - Step 2, the grapples: she comes up to her standoff 15 m off the barge's side (ABargeBoarding::GetBerth); Junie's two
 *    launchers on her starboard rail (ALamplighterLauncher) arm; each fired with a hold bites the barge's rail; both bitten,
 *    the chains haul her into the berth (a strain, a lurch, the crew braced) and the plank drops across to the barge's deck
 *    (Finch on the tube: "Her light's flickering! Hurry!"; Holloway: "Plank's down. Go.").
 *  - Steps 3-6 she holds the berth dead still: the plank is the way over and back; the player is the barge's (W3), so she
 *    doesn't keep them aboard.
 *  - Step 7, the escape: Holloway cuts the grapples and brings her under the dying barge's broken gangway (GetJumpBerth),
 *    following it as it lists and drifts ("Jump, Warden!"). A player who misses is caught under her and put on her deck
 *    (Hob's catch); one who lands on her deck plays BargeFall from it. Then she flies home on her return course and sets
 *    down on the ledge as Holloway talks (the turn-in at her wheel), anchored.
 *
 * Nothing of it is saved apart from Main 6's step: a level works her phase out from it (PilotLight::PhaseForStory): a load
 * mid-air flies again from the lift (saves are held while she flies), in the barge's steps she's in the berth, in the escape
 * under the gangway, after it on the ledge. Split by topic: PilotSortie.cpp (life, the story, the phases),
 * PilotSortieFlight.cpp (her pose in each phase), PilotSortieFight.cpp (the tenders, the boarders, the launchers, the plank),
 * PilotSortieJump.cpp (the jump, the catch, the fall).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API APilotSortie : public AActor
{
	GENERATED_BODY()

public:
	APilotSortie();

	/** The level's sortie (the first found), or null. */
	static APilotSortie* Find(const UWorld* World);

	virtual void Tick(float DeltaSeconds) override;

	// --- Placed by build_area_main6_sortie.py ---

	/** Her course out (world cm): from off the ledge east over the farm and the town to the barge's side, its end about where
	 *  she comes up to her standoff. */
	UPROPERTY(EditAnywhere, Category = "Sortie|Course")
	TArray<FVector> Course;

	/** Her course home (world cm): from by the barge back over the town and the farm to over the ledge, its end over Moored. Empty:
	 *  her course out, reversed. Her place when she leaves is put in front of it. */
	UPROPERTY(EditAnywhere, Category = "Sortie|Course")
	TArray<FVector> ReturnCourse;

	/** The ship (none: the level's Lamplighter) and the barge (none: the level's ABargeBoarding). */
	UPROPERTY(EditAnywhere, Category = "Sortie")
	TObjectPtr<ALamplighterShip> Ship;

	UPROPERTY(EditAnywhere, Category = "Sortie")
	TObjectPtr<ABargeBoarding> Barge;

	/** Her berth when there's no barge in the level (the tests; a level before W3's barge): where she's grappled. */
	UPROPERTY(EditAnywhere, Category = "Sortie")
	FTransform BerthWithoutBarge = FTransform::Identity;

	/** The tenders come, and board her (off in tests: the tenders' step is down at once). */
	UPROPERTY(EditAnywhere, Category = "Sortie|Fight")
	bool bSpawnTenders = true;

	UPROPERTY(EditAnywhere, Category = "Sortie|Fight")
	bool bSpawnBoarders = true;

	/** A tender's gas-bag's damage (0: PilotLight::BagHealthAt(FightLevel)). */
	UPROPERTY(EditAnywhere, Category = "Sortie|Fight", meta = (ClampMin = "0"))
	float BagHealth = 0.f;

	/** She follows Main 6's steps (off: the console and the tests drive her phases by hand). */
	UPROPERTY(EditAnywhere, Category = "Sortie")
	bool bFollowStory = true;

	// --- Her phase ---

	EPilotPhase GetPhase() const { return Phase; }
	float GetPhaseTime() const { return PhaseTime; }

	/** Jumps to Phase now, as a load would find it (bFresh: the player put on deck where she flies, things set at once). */
	void SetPhase(EPilotPhase NewPhase, bool bFresh = true);

	/** The winch's cast-off hold: the anchor's up and she lifts off (step 0's end). False when she isn't waiting to. */
	bool CastOff();

	/** Reads Main 6 again and puts her where its step says (as it changes; at the level's start once she's begun). */
	void RefreshStory();

	/** Her frame: her pose, the tenders, the launchers, the plank, the catch (called from her own tick; the tests call it). */
	void DriveShip(float DeltaSeconds);

	// --- The fight (PilotSortieFight.cpp) ---

	/** Both tenders down and every boarder dead (Pilot.TendersDown sent). */
	bool AreTendersDown() const { return bTendersDown; }

	/** The tenders called up (two escorts), and each now (null once gone). */
	bool AreTendersCalled() const { return bTendersCalled; }
	ACompanyTender* GetTender(int32 Index) const;

	/** Boarders on her deck still standing (or still to come over). */
	int32 GetBoardersLeft() const;

	/** Every tender down and boarder gone at once (the console): Pilot.TendersDown. */
	void TendersDownNow();

	/** Tears tender Index's bag (the console). False without it. */
	bool BurstTenderBag(int32 Index);

	/** Her two launchers (null before they're spawned), how many bite, and every armed one fired (the console). */
	ALamplighterLauncher* GetLauncher(int32 Index) const;
	int32 NumLaunchersBiting() const;
	int32 FireLaunchers();

	/** Her course out as built, and how far along it she is. */
	const FLamplighterCourse& GetCourse() const { return OutPath; }
	float GetDistance() const { return Distance; }

	/** Her berth, standoff and the jump's berth now (the barge's, or BerthWithoutBarge's). */
	FTransform GetBerth() const;
	FTransform GetJumpBerth() const;
	FVector GetGrappleAnchor(int32 Index) const;

	/** How many times Hob has caught the player under her this visit. */
	int32 GetCatches() const { return Catches; }

	/** The plank to the barge (shown in the berth), and the walk on it. */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Plank;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UBoxComponent> PlankFloor;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	// --- PilotSortie.cpp ---
	ALamplighterShip* FindShip();
	ABargeBoarding* FindBarge() const;
	void HandleMissionsChanged();
	/** Phase is consistent with what the story asks (Wanted): a live run carries on through its own beats. */
	static bool Agrees(EPilotPhase Current, EPilotPhase Wanted);
	void EnterPhase(EPilotPhase NewPhase, bool bFresh);
	/** Everything of the sortie put away, the ship moored (Idle). */
	void Stand();

	// --- PilotSortieFlight.cpp ---
	void BuildCourses();
	void BuildHomeCourse();
	FTransform Bobbed(const FTransform& Calm, float Sway) const;
	void AdvancePose(float DeltaSeconds, FTransform& OutPose, FTransform& OutCalm);
	/** The phase's own end: on to the next one. */
	void AdvanceBeats();

	// --- PilotSortieFight.cpp ---
	void CallTenders();
	void AdvanceTenders(float DeltaSeconds);
	void SpawnBoarder(uint8 BoarderRole, int32 Spot);
	void CheckTendersDown();
	void SpawnLaunchers();
	void AdvanceLaunchers(float DeltaSeconds);
	void CutLaunchers();
	/** The escorts and their boarders gone (the fight's over, or a jump past it). */
	void DestroyTenders();
	/** Everything the sortie spawned gone: the escorts, the boarders, the launchers, the deck's marker. */
	void DestroyFight();
	void ShowPlank(bool bShown, bool bAtOnce);
	void PlacePlank(float Alpha);
	void AdvancePlank(float DeltaSeconds);
	void AdvanceJump(float DeltaSeconds);
	void PlayFall();
	void AdvanceFalling();
	void SpawnDeckMarker();

	EPilotPhase Phase = EPilotPhase::Idle;
	float PhaseTime = 0.f;
	float Clock = 0.f;
	bool bBound = false;
	bool bStoryPending = true;
	FDelegateHandle MissionsChangedHandle;
	TWeakObjectPtr<ALamplighterShip> ShipNow;

	// Her poses: the courses, where she is on them, the glides' ends.
	FLamplighterCourse OutPath;
	FLamplighterCourse HomePath;
	float Distance = 0.f;
	float Speed = 0.f;
	FTransform GlideFrom = FTransform::Identity;
	/** The settle's keel has found the ledge (its dust and groan, once). */
	bool bSettleTouched = false;
	FTransform LastPose = FTransform::Identity;
	FTransform LastCalm = FTransform::Identity;

	// The tenders and the boarders.
	bool bTendersCalled = false;
	bool bTendersDown = false;
	int32 TendersFallen = 0;
	UPROPERTY(Transient)
	TArray<TObjectPtr<ACompanyTender>> Tenders;
	TArray<bool> TenderSeenDown;
	float BoardClock = -1.f;
	bool bBoardRiflemen = false;
	bool bPortRiflemen = false;
	bool bBoardPlank = false;
	bool bBoarded = false;
	bool bBoardPulledOff = false;
	struct FPendingBoarder
	{
		uint8 Role = 0;
		int32 Spot = 0;
		float At = 0.f;
	};
	TArray<FPendingBoarder> PendingBoarders;
	TArray<TWeakObjectPtr<ACreatureBase>> Boarders;

	// The launchers, the plank, the deck's marker.
	UPROPERTY(Transient)
	TArray<TObjectPtr<ALamplighterLauncher>> Launchers;
	bool bPulled = false;
	float PlankAlpha = 0.f;
	bool bPlankWanted = false;
	UPROPERTY(Transient)
	TObjectPtr<ALandfallMarker> DeckMarker;

	// The jump, the catch, the fall.
	bool bAtJump = false;
	float LandedFor = 0.f;
	int32 Catches = 0;
	bool bFallAsked = false;
};
