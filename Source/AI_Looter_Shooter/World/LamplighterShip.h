#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "World/LamplighterCourse.h"
#include "World/LamplighterRules.h"
#include "LamplighterShip.generated.h"

class AActor;
class ACompanyTender;
class ACreatureBase;
class ALandfallGrapple;
class ALandfallMarker;
class ALandfallWinch;
class ALanternFlame;
class AWardenCrewMember;
class UAudioComponent;
class UBoxComponent;
class UPointLightComponent;
class UPrimitiveComponent;
class USceneComponent;
class UStaticMeshComponent;

/** Where the Lamplighter is in Landfall (Main 1). */
UENUM(BlueprintType)
enum class EShipFlight : uint8
{
	/** Sailing in along her course, the player free on her deck (a moving floor under them). */
	Sailing,
	/** Pinned by the tender's grapples: dead still, so the boarders stand on a still deck. */
	Grappled,
	/** The lines cut: she breaks free with her bag torn, sinking and listing (still the player's to walk). */
	Breaking,
	/** The hard landing (scene HardLanding): Holloway takes her down onto the Mooring Ledge. */
	Landing,
	/** Down on the ledge at Moored, for the rest of the game: the crew's ship. */
	Moored,
};

/**
 * The Lantern Wardens' airship, the Lamplighter (Docs/Work/2026-10-31-Wardens/CONTRACT.md, FLIGHT): Landfall's stage. A new
 * game opens on her deck as she sails in from the west over Gravewind Canyon (the story's first arrival,
 * Landing_Lamplighter), the crew at their stations: Holloway at the wheel, Finch at the Great Lantern on the prow (its
 * flame leaning toward the farm), Junie at the engine; Hob's perch up in the rigging. The player walks her deck with full
 * control while she flies (her deck is a moving floor the character rides).
 *
 * The sequence (each step its reason, the contract's table): the player reaches the prow (Main 1 step 0) -> Vane's voice
 * breaks in on her horn -> the Company tender runs her down from astern, comes alongside and fires two grapples into her
 * rail -> she lurches to a dead stop (Grappled) -> the boarders cross a gangplank in two waves (DREDGER's riflemen and a
 * hose-man) while two riflemen fire from the tender -> all four boarders down = mission event Landfall.DeckClear -> the
 * player cuts both lines (hold Interact) -> she breaks free (Breaking), the tender falls burning into the canyon, her bag
 * torn -> the scene HardLanding takes her down onto the Mooring Ledge -> Moored at Moored, groaning toward the drop until
 * the anchor winch is wound (Main 1 step 4). Saves are held from the flight's start until she's moored.
 *
 * How she begins comes from the campaign (LandfallRules::StartFor): BeginLandfall, which UColdOpenSubsystem::BeginIfDue
 * calls as the level begins (and the ship itself a second in, should nothing have). Placed by WORLD's build script
 * (build_area_landfall.py): Course, GrappleStop, Moored and the sides. Stand-in look: SM_Skiff_A_Packet scaled to ~24 m
 * (the Tripo Lamplighter replaces it: HullMeshPath in LamplighterShipParts.cpp), with her own walkable deck boxes and rail
 * walls that keep the player and the boarders aboard.
 *
 * Split by topic: LamplighterShip.cpp (life, the flight's clock, sailing), LamplighterShipParts.cpp (her pieces, crew and
 * lantern), LamplighterShipBoarding.cpp (the tender, the grapples, the boarders), LamplighterShipLanding.cpp (breaking free,
 * the landing, moored and the anchor), LamplighterShipStory.cpp (the campaign, the player on deck, the crew's lines).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ALamplighterShip : public AActor
{
	GENERATED_BODY()

public:
	ALamplighterShip();

	/** The level's Lamplighter (the first found), or null. */
	static ALamplighterShip* Find(const UWorld* World);

	virtual void Tick(float DeltaSeconds) override;

	/** Moves her on by DeltaSeconds in whatever she's doing (the tick's work; tests call it with no play running). */
	void Advance(float DeltaSeconds);

	// --- Placed by WORLD's build script ---

	/** Her course in (world cm): from far out west over the canyon, low over the plains, into the canyon toward the ledge. */
	UPROPERTY(EditAnywhere, Category = "Lamplighter|Course")
	TArray<FVector> Course;

	/** The Course point where the tender grapples her, over open air (~120 m short of the ledge): she creeps from here. */
	UPROPERTY(EditAnywhere, Category = "Lamplighter|Course", meta = (ClampMin = "0"))
	int32 GrappleStop = 0;

	/** Where she lies on the Mooring Ledge after the landing: her keel's middle on the ground, her bow along +X. Left at
	 *  identity, where she was placed. */
	UPROPERTY(EditAnywhere, Category = "Lamplighter|Course")
	FTransform Moored = FTransform::Identity;

	/** The side the tender comes alongside: +1 her starboard (+Y), -1 port. */
	UPROPERTY(EditAnywhere, Category = "Lamplighter|Course")
	float TenderSide = 1.f;

	/** Moored, the side her gangway opens to the ledge: +1 starboard, -1 port. */
	UPROPERTY(EditAnywhere, Category = "Lamplighter|Moored")
	float GangwaySide = -1.f;

	/** Moored, how far along her the gangway opens (her frame, cm from midships toward the bow; the rail's length there opens):
	 *  forward, where her bow lies on the ledge and her stern hangs over the drop. */
	UPROPERTY(EditAnywhere, Category = "Lamplighter|Moored", meta = (ClampMin = "-1150", ClampMax = "750"))
	float GangwayAt = 475.f;

	/** Moored and not yet anchored, the way she slides (world, toward the canyon's edge); zero: she only groans and shifts. */
	UPROPERTY(EditAnywhere, Category = "Lamplighter|Moored")
	FVector CanyonSide = FVector::ZeroVector;

	/** Where an anchor winch is spawned when the level has none placed (Moored-local cm, dropped to the ground; the default
	 *  is beside a port gangway: flip Y with GangwaySide). */
	UPROPERTY(EditAnywhere, Category = "Lamplighter|Moored")
	FVector WinchOffset = FVector(-300.0, -750.0, 0.0);

	/** What the Great Lantern's flame leans toward (an actor tag): Delia's porch, where they took Abel's soul. */
	UPROPERTY(EditAnywhere, Category = "Lamplighter|Look")
	FName LanternTarget;

	/** The hull's scale: 1, the user's Tripo Lamplighter (SM_Lamplighter) is imported at her real size (31 m from the bag's
	 *  nose to its tail, the hull 28 x 8.5 m, her deck 22 x 7.5 m; lanes/IMPORT.md). The old stand-in wanted (2.7, 2.9, 1.3). */
	UPROPERTY(EditAnywhere, Category = "Lamplighter|Look")
	FVector HullScale = FVector(1.0, 1.0, 1.0);

	/** The crew at their stations (off in tests that don't need them). */
	UPROPERTY(EditAnywhere, Category = "Lamplighter|Crew")
	bool bSpawnCrew = true;

	/** The boarders and the tender's riflemen (off in tests: the waves are then empty, and the deck clears at once). */
	UPROPERTY(EditAnywhere, Category = "Lamplighter|Fight")
	bool bSpawnBoarders = true;

	// --- Her deck's size (the Tripo hull's, measured in Blender: lanes/IMPORT.md; cm, her own frame) ---

	/** The main deck's top above her keel (the pivot), and her rails out from her middle. */
	static constexpr float DeckTop = 422.f;
	static constexpr float RailOut = 375.f;

	// --- The flight ---

	EShipFlight GetFlight() const { return Flight; }

	/**
	 * As the level begins: reads the campaign (LandfallRules::StartFor, ApplyStart) and begins her there: the flight with the
	 * player on her deck, or moored (anchored or not). True when the flight plays. Once a level; later calls do nothing.
	 */
	bool BeginLandfall();
	bool HasBegun() const { return bBegun; }

	/** Puts her in State at once (the dev command, the tests): its place, its pieces, the player kept aboard where it flies. */
	void SetFlight(EShipFlight State);

	/** Her course was built (two or more points). */
	bool HasCourse() const { return Path.IsValid(); }
	const FLamplighterCourse& GetCourse() const { return Path; }
	float GetDistance() const { return Distance; }
	float GetSpeed() const { return Speed; }
	float GetGrappleStopDistance() const;

	/** Where she is with no bob, bank or pitch: what the tender keeps station on. */
	FTransform GetCalmTransform() const { return CalmTransform; }

	// --- The tender and the fight (LamplighterShipBoarding.cpp) ---

	/** The player reached the prow: Vane on the horn, and the tender comes. Once. */
	void CallTender();
	bool IsTenderCalled() const { return bTenderCalled; }
	ACompanyTender* GetTender() const;

	/** The grapples bite now and she's pinned (the dev jumps; the tender's own fire does it in play). */
	void GrappleNow();

	/** A grapple bit her rail; when both have, she's pinned. */
	void NotifyGrappleBit(ALandfallGrapple& Grapple);

	/** A grapple was cut; when both are, she breaks free. */
	void NotifyGrappleCut(ALandfallGrapple& Grapple);

	/** The grapples may be cut: pinned, and every boarder down. */
	bool CanCutGrapples() const;
	bool IsDeckClear() const { return bDeckClear; }
	int32 GetBoardersLeft() const;
	int32 CountGrapples(bool bBitingOnly) const;

	/** Every boarder down at once (the dev command): the deck is clear. */
	void ClearDeck();

	// --- Breaking free and the landing (LamplighterShipLanding.cpp) ---

	/** Where she broke free from (her still pose when the lines parted). */
	const FTransform& GetBreakFrom() const { return BreakFrom; }

	/** Her pose SecondsFromBreak into breaking free and the landing (HardLanding::PoseAt), with its gas and its scrape. */
	void ApplyLanding(float SecondsFromBreak);

	/** The hull meets the ledge (HardLanding's Touch): the crash's sound, dust, sparks and jolt (none when bSkipping). */
	void Touchdown(bool bSkipping);

	/** The landing is over: she lies at Moored. */
	void FinishLanding();

	// --- Moored (LamplighterShipLanding.cpp) ---

	bool IsAnchored() const { return bAnchored; }

	/** The winch was wound: she's held, the creaking stops, Junie cuts the engine (bPlayed: with its sounds and lines). */
	void SetAnchored(bool bPlayed);

	/** Where the anchor chain ties on her hull (world). */
	FVector GetAnchorPoint() const;

	// --- Her pieces (LamplighterShipParts.cpp) ---

	/** Her deck (the floor the player rides, and what's fixed to her: grapples, crew, the prow's marker). */
	UPrimitiveComponent* GetDeck() const;

	/** A point on her deck in her own frame (cm, Z from the deck's top) as a world point. */
	FVector DeckPoint(const FVector& Local) const;

	/** Every crew member braces against a lurch for Seconds. */
	void BraceCrew(float Seconds);

	/** Puts the player on her deck at the arrival spot, standing on it as their floor. False without a player. */
	bool PutPlayerOnDeck();

	/** The player stands aboard her (on her deck, between her rails). */
	bool IsPlayerAboard() const;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Hull;

	/** The walkable deck (the floor the player rides) and the prow's, world-static so creatures stand on them. */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UBoxComponent> Deck;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UBoxComponent> ProwDeck;

	/** The Tripo hull's deck rises aft: the last few metres before the wheel's step, a step up from the main deck. */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UBoxComponent> SternDeck;

	/** The rail walls: tall and unseen, they stop pawns only (bullets and the camera pass over the visible gunwale). */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TArray<TObjectPtr<UBoxComponent>> Rails;

	/** Landing_Lamplighter's spot (tagged so): where the player arrives on deck, facing the prow. */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> LandingSpot;

	/** Hob's perch in the rigging (tagged Lamplighter_HobPerch). */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> HobPerch;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> LanternPost;

	/** The Great Lantern (stand-in: the Keeper's Lantern made 1.5 m tall), its glass lit, its light warm and shadowless. */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> GreatLantern;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UPointLightComponent> LanternLight;

	/** The wheel Holloway holds, and the engine housing Junie tends (stand-ins). */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Wheel;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TArray<TObjectPtr<UStaticMeshComponent>> EngineHousing;

	/** The gangway down to the ledge once she's moored. */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Gangway;

	/** The hull at HullScale as the build script set it, in the editor too. */
	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	// --- LamplighterShip.cpp ---
	void SetupFromPlacement();
	void PlaceOnCourse(float AtDistance, float DeltaSeconds);
	void AdvanceSailing(float DeltaSeconds);
	void AdvanceGrappled(float DeltaSeconds);
	/** Puts the player back aboard should they leave her while she flies (a fall, a death's wake at a roost on the ground). */
	void KeepPlayerAboard(float DeltaSeconds);
	void UpdateSounds();
	void EnterFlight(EShipFlight State);
	/** Takes the tender, the grapples and the boarders away and forgets the fight (a jump, the level's end). */
	void ResetFight();

	// --- LamplighterShipParts.cpp ---
	void BuildRails();
	void ApplyLook();
	void SpawnPieces();
	void LeanLantern();
	void ShowGangway(bool bShown);

	// --- LamplighterShipBoarding.cpp ---
	/** Moves the tender and the grapples on after her, so they never lag her by a frame (they don't tick themselves). */
	void AdvancePieces(float DeltaSeconds);
	void AdvanceTender(float DeltaSeconds);
	void FireGrapples();
	void PinnedByGrapples();
	void AdvanceBoarding(float DeltaSeconds);
	void SpawnWave(int32 Wave);
	void SpawnBoarder(uint8 BoarderRole, int32 Spot);
	void CheckDeckClear();
	void BreakFree();

	// --- LamplighterShipLanding.cpp ---
	void AdvanceBreaking(float DeltaSeconds);
	void StartHardLanding();
	void AdvanceMoored(float DeltaSeconds);
	ALandfallWinch* FindOrSpawnWinch();

	// --- LamplighterShipStory.cpp ---
	void BindMissions();
	void HandleMissionsChanged();
	void HoldSaves(bool bHold);
	int32 GetMainStep() const;

	FLamplighterCourse Path;
	EShipFlight Flight = EShipFlight::Moored;
	bool bBegun = false;
	float BeganWait = 0.f;
	float Clock = 0.f;
	float Distance = 0.f;
	float Speed = 0.f;
	FTransform CalmTransform = FTransform::Identity;

	// Sailing and the tender.
	bool bTenderCalled = false;
	float SinceCalled = 0.f;
	bool bGrapplesFired = false;
	float LanternWait = 0.f;
	bool bDeckLinesSaid = false;
	float DeckLinesWait = 0.f;
	int32 VaneHornConversation = INDEX_NONE;

	// Grappled.
	float GrappledClock = 0.f;
	FTransform LurchFrom = FTransform::Identity;
	FTransform GrappleSpot = FTransform::Identity;
	int32 WavesSpawned = 0;
	float WaveClock = 0.f;
	float NextWaveAt = -1.f;

	/** A boarder still to come over the gangplank: its role (EDredgerRole), its spot on deck, and when it comes. */
	struct FPendingBoarder
	{
		uint8 Role = 0;
		int32 Spot = 0;
		float At = 0.f;
	};
	TArray<FPendingBoarder> PendingBoarders;
	bool bDeckClear = false;
	bool bRiflemenOut = false;
	bool bPlankDown = false;

	// Breaking and landing.
	FTransform BreakFrom = FTransform::Identity;
	float BreakClock = 0.f;
	float LastLanding = 0.f;
	float GasWait = 0.f;
	float SparkWait = 0.f;
	bool bTouched = false;

	// Moored.
	bool bAnchored = false;
	float SlideClock = 0.f;
	float Slid = 0.f;
	float NextGroan = 0.f;

	// The player kept aboard.
	float OffDeckFor = 0.f;
	bool bSavesHeld = false;

	// The missions.
	int32 LastMainStep = INDEX_NONE;
	bool bPointLinesSaid = false;
	bool bAnchorLinesSaid = false;
	FDelegateHandle MissionsChangedHandle;

	UPROPERTY(Transient)
	TObjectPtr<ACompanyTender> Tender;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ALandfallGrapple>> Grapples;

	TArray<TWeakObjectPtr<ACreatureBase>> Boarders;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AWardenCrewMember>> Crew;

	UPROPERTY(Transient)
	TObjectPtr<ALandfallMarker> ProwMarker;

	UPROPERTY(Transient)
	TObjectPtr<ALanternFlame> LanternFlame;

	/** What the flame leans toward (found by LanternTarget's tag), and when to look for it again if it isn't there. */
	TWeakObjectPtr<AActor> LanternTargetActor;
	float LanternSearchWait = 0.f;

	TWeakObjectPtr<UAudioComponent> EngineSound;
	TWeakObjectPtr<UAudioComponent> CreakSound;
};
