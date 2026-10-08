#pragma once

#include "CoreMinimal.h"
#include "Areas/StationBoard.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "SkiffJetty.generated.h"

class AStaticMeshActor;
class UMaterialInterface;
class UMissionRunner;
class USceneComponent;
class USplineMeshComponent;
class UStaticMesh;
class UStaticMeshComponent;
struct FCampaignRecord;

/** One mooring line: from a bollard on the jetty to a rope's end on the skiff. */
USTRUCT()
struct AI_LOOTER_SHOOTER_API FSkiffMooringLine
{
	GENERATED_BODY()

	/** The jetty's socket on the bollard's top. */
	UPROPERTY(EditAnywhere, Category = "Mooring")
	FName Bollard;

	/** The skiff's socket at the end of its modeled rope stub. */
	UPROPERTY(EditAnywhere, Category = "Mooring")
	FName SkiffEnd;
};

/**
 * Skyreach's skiff jetty (Docs/Story.md, "Leaving the tutorial island"): the timber jetty off the plateau's rim past the
 * lookout, with the packet skiff moored at it, its gangplank, two mooring lines drawn to the bollards, the bell on its post
 * and the chalk slate ("Skiff departs when you're ready."), each on the jetty's sockets. Every mesh is a setting; the
 * defaults are the imported models (/Game/Art/Props, /Game/Art/Vehicles).
 *  - The gangplank stays up until the tutorial is done (or skipped), and is always down once the first cast-off is
 *    recorded. Lowering it in play rings the bell and offers "Board the skiff" (DA_Mission_BoardSkiff) while the player
 *    hasn't left yet; it shows on a Board.Skiff event as they cast off.
 *  - Holding Interact at the gangplank (a second, like equipping) opens the station board (UStationBoardWidget): before
 *    the first cast-off its one line is the story's first area, and its confirm "Leave Skyreach? Your story begins. You
 *    can come back to practice any time."; after it, every opened area, each a plain trip to its station.
 *  - Casting off (CastOff, SkiffJettyCastOff.cpp): USceneSubsystem's ride carries the player out on the skiff's deck; the
 *    ropes slip as it leaves, and behind the white it ends in, UAreaTravelSubsystem takes them to the story's first
 *    arrival. Without a ride, straight on behind the white.
 *  - It carries its landing (Landing_Jetty, on the jetty's Landing socket): practice trips back to Skyreach arrive there.
 *
 * The skiff is an actor of its own in play (its hull the root, its Deck socket the ride's), spawned moored and fixed to
 * the jetty, so the crosshair on the plank finds the jetty; the editor shows it with stand-ins. Placed by the area's build
 * script.
 *
 * SkiffJetty.cpp is the jetty, the story it reads, the plank's and the bell's motion and boarding; SkiffJettyMooring.cpp
 * the moored skiff (where it lies, the plank's swing) and its mooring lines; SkiffJettyCastOff.cpp casting off.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ASkiffJetty : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	ASkiffJetty();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

	// --- IInteractable ---
	virtual FInteractionOptions GetInteractionOptions(const UInteractionComponent& User) const override;
	virtual bool Interact(UInteractionComponent& User, bool bHeld) override;
	virtual TOptional<FVector> GetInteractionLocation() const override;

	// --- The rules (apart from the world, for the tests) ---

	/** The gangplank is down once the tutorial is done (or skipped), and always once the first cast-off is recorded. */
	static bool IsGangplankDownFor(bool bTutorialDone, bool bFirstCastOff);

	/**
	 * "Board the skiff" shows while the tutorial is done and the player hasn't cast off for the first time: after the
	 * tutorial, after skipping it in the level, and for sessions that finished it before the skiff came.
	 */
	static bool ShowsBoardingFor(bool bTutorialDone, bool bFirstCastOff);

	/**
	 * Starts the boarding mission MissionId on Runner when ShowsBoardingFor says so (the story read from the runner's
	 * campaign record), and has the minimap guide to it. True when it's running.
	 */
	static bool OfferBoarding(UMissionRunner& Runner, FName MissionId, bool bTutorialDone);

	// --- The story and the gangplank ---

	/** The tutorial is done: the progress's flag, or its mission finished in the campaign record. */
	bool IsTutorialDone() const;

	/** The player has left Skyreach for the first time (the campaign record). */
	bool HasCastOff() const;

	/** Looks at the story again: once it says so, the plank lowers (the bell rings) and boarding is offered. */
	void RefreshFromStory();

	/** The plank going (or gone) down. */
	bool IsGangplankDown() const { return bGangplankDown; }

	/** How far up the plank is: 0 down on the deck, 1 raised. */
	float GetGangplankRaise() const { return GangplankRaise; }

	/** Swings the plank down or up (at once with bInstant). It rings no bell: RefreshFromStory does, as it lowers it. */
	void SetGangplankDown(bool bDown, bool bInstant);

	/** The plank up or down whatever the story says, until the level ends (Looter.Station.Gangplank). Down rings the bell. */
	void ForceGangplank(bool bDown);

	/** The bell swings, and settles over BellRingSeconds. */
	void RingBell();

	bool IsBellRinging() const { return BellTime >= 0.f; }

	/** Moves the plank, the bell and the lines on, and looks at the story while waiting: the tick calls it; tests do. */
	void Advance(float DeltaSeconds);

	// --- Leaving (SkiffJettyCastOff.cpp) ---

	/**
	 * The first cast-off from here: the boarding mission hears Board.Skiff, and the skiff's ride plays (USceneSubsystem),
	 * its ropes slipping as it leaves; behind the white it ends in, the trip goes (UAreaTravelSubsystem). Without a ride,
	 * straight on behind the white. False when it's casting off already or has no skiff.
	 */
	bool CastOff();

	bool IsCastingOff() const { return bCastingOff; }

	/** The mooring lines have been let go. */
	bool AreLinesSlipped() const { return bLinesSlipped; }

	/** The board's words here: "Skiff jetty", "Cast off". */
	FStationBoardWords GetBoardWords() const;

	/** The moored skiff in play (its hull the root), and its gangplank. Null before play. */
	AStaticMeshActor* GetSkiff() const { return Skiff.Get(); }
	UStaticMeshComponent* GetGangplank() const { return Gangplank.Get(); }

	/**
	 * Where the moored skiff lies in the jetty's own space: turned as the jetty is (both front along +X), its plank's
	 * hinge level with the deck's edge at GangplankLandSocket and the plank's length less GangplankOverlap out from it, so
	 * the lowered plank rests GangplankOverlap onto the deck. FallbackMooring when the sockets can't say.
	 */
	FTransform ComputeMooring() const;

	// --- Components ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Jetty;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> BellPost;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Bell;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Slate;

	/** Where trips back to Skyreach arrive: on the deck, facing inland (tagged LandingName). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Landing;

	// --- Art: the imported models by default ---

	UPROPERTY(EditAnywhere, Category = "Jetty|Art")
	TObjectPtr<UStaticMesh> JettyMesh;

	/** The packet skiff (the tutorial's paint). Its pivot is under the hull's middle at the keel, its front along +X. */
	UPROPERTY(EditAnywhere, Category = "Jetty|Art")
	TObjectPtr<UStaticMesh> SkiffMesh;

	/** The 3 m gangplank: its origin is the hinge, and it's modeled lowered. */
	UPROPERTY(EditAnywhere, Category = "Jetty|Art")
	TObjectPtr<UStaticMesh> GangplankMesh;

	UPROPERTY(EditAnywhere, Category = "Jetty|Art")
	TObjectPtr<UStaticMesh> BellPostMesh;

	/** The bell: its origin is its swing axis. */
	UPROPERTY(EditAnywhere, Category = "Jetty|Art")
	TObjectPtr<UStaticMesh> BellMesh;

	UPROPERTY(EditAnywhere, Category = "Jetty|Art")
	TObjectPtr<UStaticMesh> SlateMesh;

	/** A mooring line: this mesh stretched from bollard to rope's end (the engine's cylinder, along its Z). */
	UPROPERTY(EditAnywhere, Category = "Jetty|Art")
	TObjectPtr<UStaticMesh> MooringLineMesh;

	/** The lines' rope, the skiff's own canvas rope. */
	UPROPERTY(EditAnywhere, Category = "Jetty|Art")
	TObjectPtr<UMaterialInterface> MooringLineMaterial;

	// --- Sockets ---

	/** On the jetty: where the gangplank rests (on the deck's edge in the rail opening), the props' spots, the landing. */
	UPROPERTY(EditAnywhere, Category = "Jetty|Sockets")
	FName GangplankLandSocket = TEXT("Gangplank_Land");

	UPROPERTY(EditAnywhere, Category = "Jetty|Sockets")
	FName BellPostSocket = TEXT("BellPost");

	UPROPERTY(EditAnywhere, Category = "Jetty|Sockets")
	FName SlateSocket = TEXT("Slate");

	UPROPERTY(EditAnywhere, Category = "Jetty|Sockets")
	FName LandingSocket = TEXT("Landing");

	/** On the bell post: where the bell hangs. */
	UPROPERTY(EditAnywhere, Category = "Jetty|Sockets")
	FName BellSocket = TEXT("Bell");

	/** On the skiff: the gangplank's hinge, on the sill of its opening. */
	UPROPERTY(EditAnywhere, Category = "Jetty|Sockets")
	FName SkiffGangplankSocket = TEXT("Gangplank");

	// --- Mooring ---

	/** The lines: the stern line from Bollard_1 to Mooring_3, the bow line from Bollard_2 to Mooring_1. */
	UPROPERTY(EditAnywhere, Category = "Jetty|Mooring")
	TArray<FSkiffMooringLine> MooringLines;

	/** How far the lowered plank rests onto the deck past its edge (cm): 25 cm of a 3 m plank. */
	UPROPERTY(EditAnywhere, Category = "Jetty|Mooring", meta = (ClampMin = "0"))
	float GangplankOverlap = 25.f;

	/**
	 * Where the skiff lies when the sockets can't say (other meshes): in the jetty's space, its centreline 5.03 m off the
	 * jetty's, its keel 0.45 m under the deck's top, its opening level with the landing.
	 */
	UPROPERTY(EditAnywhere, Category = "Jetty|Mooring")
	FTransform FallbackMooring;

	/** The lines' thickness and how far they sag in the middle (cm). */
	UPROPERTY(EditAnywhere, Category = "Jetty|Mooring", meta = (ClampMin = "0.1"))
	float MooringLineThickness = 3.5f;

	UPROPERTY(EditAnywhere, Category = "Jetty|Mooring", meta = (ClampMin = "0"))
	float MooringLineSag = 30.f;

	// --- The gangplank and the bell ---

	/** Seconds the plank takes to swing down, or up. */
	UPROPERTY(EditAnywhere, Category = "Jetty|Motion", meta = (ClampMin = "0.1"))
	float GangplankSeconds = 2.5f;

	/** The bell's swing: how far either way at first (degrees), each swing's seconds, how long until it's still. */
	UPROPERTY(EditAnywhere, Category = "Jetty|Motion", meta = (ClampMin = "0"))
	float BellSwingDegrees = 24.f;

	UPROPERTY(EditAnywhere, Category = "Jetty|Motion", meta = (ClampMin = "0.1"))
	float BellSwingSeconds = 1.3f;

	UPROPERTY(EditAnywhere, Category = "Jetty|Motion", meta = (ClampMin = "0.1"))
	float BellRingSeconds = 6.f;

	/** The bell's swing axis in its own space: the headstock's pin, across the post's arm. */
	UPROPERTY(EditAnywhere, Category = "Jetty|Motion")
	FVector BellSwingAxis = FVector::ForwardVector;

	// --- Boarding ---

	/** What holding Interact at the gangplank does, in the prompt's words. */
	UPROPERTY(EditAnywhere, Category = "Jetty|Boarding")
	FText Prompt;

	/** How long Interact is held to open the board (a second, as the doc's equip hold). */
	UPROPERTY(EditAnywhere, Category = "Jetty|Boarding", meta = (ClampMin = "0.05"))
	float HoldSeconds = 1.f;

	/** How far from the player's eyes the gangplank can be used (cm); 0: the player's reach. */
	UPROPERTY(EditAnywhere, Category = "Jetty|Boarding", meta = (ClampMin = "0"))
	float Reach = 0.f;

	/** The landing it carries: Skyreach's (UAreaDefinition::Landings). */
	UPROPERTY(EditAnywhere, Category = "Jetty|Boarding")
	FName LandingName = TEXT("Landing_Jetty");

	/** "Board the skiff", the mission it offers (DA_Mission_BoardSkiff). */
	UPROPERTY(EditAnywhere, Category = "Jetty|Boarding")
	FName BoardingMissionId = TEXT("BoardSkiff");

	/** The tutorial's mission: finished, the tutorial is done. */
	UPROPERTY(EditAnywhere, Category = "Jetty|Boarding")
	FName TutorialMissionId = TEXT("Tutorial");

	/** What casting off tells the missions: Board.<Vehicle>. */
	UPROPERTY(EditAnywhere, Category = "Jetty|Boarding")
	FName Vehicle = TEXT("Skiff");

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	// SkiffJetty.cpp
	/** Puts each mesh on its component and each prop on its socket. */
	void ApplyArt();

	void ApplyGangplank();
	void ApplyBell();
	void RefreshTick();

	/** The record of the story here, or null. */
	FCampaignRecord* FindCampaign() const;

	// SkiffJettyMooring.cpp
	/** The plank's hinge-to-end direction in its own space (level), its length, and the turn that raises it. */
	void MeasureGangplank(FVector& OutAlong, float& OutLength) const;
	FQuat GetRaiseTurn() const;

	/** Spawns the moored skiff and its plank, fixed to the jetty. */
	void SpawnSkiff();

	/** The mooring lines from the bollards to the skiff's rope ends. */
	void MakeLines();
	void UpdateLines(float DeltaSeconds);
	void PlaceLine(USplineMeshComponent& Line, const FVector& Start, const FVector& End, float Sag) const;

	/** Lets the lines go: their skiff ends drop and hang from the bollards. */
	void SlipLines();

	// SkiffJettyCastOff.cpp
	/** The ride's moments: its "CastOff" slips the ropes and pulls the plank in. */
	void HandleSceneEvent(FName Event);

	/** The white is full: the trip goes, or (it can't) the skiff and the player come back to the jetty. */
	void HandleWhiteout();

	/** Back as moored: the skiff at its mooring and fixed to the jetty, the lines on, the plank down. */
	void RestoreMooring();

	void StopListeningToScene();

#if WITH_EDITORONLY_DATA
	/** The editor's stand-ins for the skiff and its plank (play spawns the real one). */
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> SkiffPreview;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> GangplankPreview;
#endif

	UPROPERTY(Transient)
	TObjectPtr<AStaticMeshActor> Skiff;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> Gangplank;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USplineMeshComponent>> Lines;

	/** Where each slipped line's skiff end was as it let go, in the jetty's space. */
	TArray<FVector> SlipFrom;

	/** Seconds a slipped line's end takes to fall and hang from its bollard. */
	static constexpr float LineDropSeconds = 0.9f;

	bool bGangplankDown = false;
	/** 0: down on the deck, 1: raised. */
	float GangplankRaise = 1.f;
	/** The turn (in its hinge socket's space) that raises the plank: its free end from level to straight up. */
	FQuat GangplankRaiseTurn = FQuat::Identity;
	/** The story no longer moves the plank (the dev command did). */
	bool bGangplankForced = false;

	/** Seconds since the bell was rung; negative while it's still. */
	float BellTime = -1.f;

	bool bCastingOff = false;
	/** The ride is playing the cast-off (USceneSubsystem holds the player until a trip, or gives them back). */
	bool bRiding = false;
	bool bLinesSlipped = false;
	/** Seconds since the lines were let go. */
	float SlipTime = 0.f;

	/** Seconds since the story was last looked at, while waiting for the tutorial. */
	float SinceStoryCheck = 0.f;

	/** The main menu's backdrop: no player, no story. */
	bool bMenuWorld = false;

	FDelegateHandle SceneEventHandle;
};
