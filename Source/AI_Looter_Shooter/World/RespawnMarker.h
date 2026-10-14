#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "World/GraveMarkRules.h"
#include "World/RoostRules.h"
#include "RespawnMarker.generated.h"

class ARespawnMarker;
class ARoostCrows;
class UArrowComponent;
class UAudioComponent;
class UInstancedStaticMeshComponent;
class UMissionDefinition;
class UMissionRunner;
class UPointLightComponent;
class URoostBeaconComponent;
class UStaticMesh;
class UStaticMeshComponent;
struct FCampaignRecord;

/** Where a death wakes the player (ARespawnMarker::ChooseWakeSpot): the roost last found, or the level's own start. */
struct AI_LOOTER_SHOOTER_API FRespawnWakeSpot
{
	/** A roost's spot on the ground (the player stands on it), or a player start's (the middle of the player goes there). */
	FVector Location = FVector::ZeroVector;

	/** Which way they face: the roost's or the start's, level. */
	FRotator Facing = FRotator::ZeroRotator;

	/** The roost they wake at; null at the level's start. (Named for the graves the roosts replaced.) */
	const ARespawnMarker* Grave = nullptr;

	/** The level's start they wake at when no roost has been found here yet; null at a roost. */
	const AActor* Start = nullptr;

	/**
	 * Hob's crows carry them there: always to a roost, and to the level's start on a map with roosts while a story is played
	 * (a death before the first roost is found). A map without roosts (Skyreach) keeps the plain fade.
	 */
	bool bByCrows = false;

	/** Somewhere was found (a level with neither a roost found nor a start of its own has nowhere). */
	bool IsSet() const { return Grave || Start; }
};

/**
 * One of Hob's roosts (the user's call, 2026-10-13; two a map since 2026-10-14, Ransom's Rest's the Depot and Calder's
 * Post): the one save, respawn and fast-travel point on every map, the same everywhere: the Crow's Mast (the user's pick from look sheet 41: a mast on a cairn, its yard cockbilled, crosstrees, a
 * stork's nest round the topmast, a fire basket on top, an old bell, the crow's-foot mark burnt in a board; SM_HobRoost, its
 * bell SM_HobRoostBell; a greybox stand-in until they're imported), Hob's crows on its perches and, once found, circling its
 * beacon (ARoostCrows), and the beacon itself, a flame in the basket under a tall column of light in the roost's one colour
 * (Hob's Ember) seen from across the map (URoostBeaconComponent), dark until the roost is found. It replaced the respawn
 * graves and keeps their class, ids and save data: a save's marked graves are found roosts (MarkerId,
 * FCampaignRecord::LastGraves, MarkedGraves and ActiveRespawns are unchanged), so the code's names still say grave where the
 * player now reads roost.
 *
 * The actor stands where the player wakes (on the ground, facing its arrow); the mast stands behind them, placed by the
 * model's Wake socket (or the stand-in's), so the player waking or arriving looks out over the land with the crows bursting
 * off over their head. Tools/Unreal/build_area_roosts.py places every roost from the layout's gameplay.roosts. It's never
 * streamed out: a death far away may wake the player here.
 *
 * Found (marked): walking up to it (alive, outside a scene, within MarkRadius with a clear line to the roost) marks it. The
 * first time: the beacon's column shoots up out of the basket, the bell rings and swings, the crows lift off and settle and
 * a few take to circling the beacon (RoostMark::Found), the ROOST FOUND banner, the session saved soon, and it opens (if the
 * story hadn't) and becomes its area's save point (FCampaignRecord::LastGraves). The first roost of a story, Hob explains
 * them. Walking up to one found before, when another is the save point, makes it the save point again with a quieter sound,
 * a tap of the bell and a pulse up its column; so does arriving at it by fast travel (MarkOnArrival). The save point, the
 * roost last found here (FindLastFound), is where a death wakes the player and where Continue does (the user's rule,
 * 2026-10-14: never the nearest); before any is found a death wakes at the level's own start. Open (fast travel goes
 * there): from the start (bStartActive), with its mission's finish (ActiveAfterMission), recorded open, or found. Its crows
 * carry Ellis to it (Welcome).
 *
 * Cheap: it checks the player a few times a second on a timer, never every frame (one distance while they're far); its
 * crows are updated by the fauna's subsystem; the beacon ticks only for its show or a near camera; the actor ticks only for
 * its light's flare and its bell's swing; its sound plays only while the player is near.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ARespawnMarker : public AActor
{
	GENERATED_BODY()

public:
	ARespawnMarker();

	/**
	 * Its name in the campaign record ("FamilyPlot", "MainStreet"): unique in the campaign, and never changed once a session
	 * may have saved it (the roosts that replaced the graves kept the graves' ids). None: the actor's name, which a rebuilt
	 * level may change.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Roost")
	FName MarkerId;

	/** Open from the start. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Roost")
	bool bStartActive = false;

	/** Opens when this mission is finished (its id). None: only the start, being found or the console opens it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Roost")
	FName ActiveAfterMission;

	/** What the map and the banner call it ("Main Street"). Empty: its id in words ("Main Street" for MainStreet). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Roost")
	FText DisplayName;

	// --- Found on approach (the save point) ---

	/** Walking up to it finds it (marks it as the save point). Off: only the console does. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Save point")
	bool bMarkOnApproach = true;

	/** How close the player's feet come to find it (cm): a roost stands beside a road, so passing by finds it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Save point", meta = (ClampMin = "50", Units = "cm"))
	float MarkRadius = GraveMarkRules::DefaultRadius;

	/**
	 * It can't be found before this mission is finished (its id). None: ActiveAfterMission's, so a roost the story opens
	 * can't be found early; with neither, any time.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Save point")
	FName MarkAfterMission;

	// --- The roost's model ---

	/** SM_HobRoost (found by itself once it's imported; the build script sets it too). None: the greybox stand-in. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Roost|Model")
	TObjectPtr<UStaticMesh> RoostMesh;

	/** SM_HobRoostBell, hung on the roost's Bell socket by the top of its loop. None: a box stands in for it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Roost|Model")
	TObjectPtr<UStaticMesh> BellMesh;

	/** How far the mast's foot stands above the player's spot (cm; the build script measures the ground at both). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Roost|Model", meta = (Units = "cm"))
	float RoostLift = 0.f;

	// --- Components ---

	/** The roost: SM_HobRoost, placed so its Wake socket is this actor's spot. Solid, as a mast is. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Roost;

	/** Its bell on the Bell socket: it swings and rings when the roost is found, made the save point again, or the crows land. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Bell;

	/** The greybox stand-in while there's no model: the mast, yard, crosstrees and nest in weathered wood, the basket in iron, the cairn in stone. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> GreyboxWood;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> GreyboxIron;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> GreyboxStone;

	/** The beacon on the post's top: the column of light seen from across the map, dark until found. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<URoostBeaconComponent> Beacon;

	/** Its light on the post once found: small, in the roost's colour, never a shadow. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPointLightComponent> BeaconLight;

	/** MarkerId, or the actor's name without one. */
	FName GetMarkerId() const;

	/** DisplayName, or the id in words. */
	FText GetGraveName() const;

	/**
	 * Where the player gets up here (a death's wake-up, a fast travel's arrival, a Continue): standing on its spot, facing its
	 * arrow, level. The spot is on the ground: the player's middle goes half their height above it.
	 */
	FRespawnWakeSpot GetWakeSpot() const;

	/** It's open in this story: from the start, recorded open, or its mission finished. */
	bool IsActive(const FCampaignRecord& Campaign) const;

	/** Records it open in Campaign. True when it wasn't recorded before. */
	bool Activate(FCampaignRecord& Campaign) const;

	/** Why it's open ("open from the start", "open: Main1 finished", "opened"), or "closed", for logs and the console. */
	FString DescribeState(const FCampaignRecord& Campaign) const;

	/**
	 * Where a death wakes the player (the user's rule, 2026-10-14): at the roost they last found on this map (walked up to,
	 * or arrived at by fast travel: FindLastFound), wherever they fell; with none found yet, the level's own start (Ransom's
	 * Rest's: Ellis's family plot), never a trip's landing. Without a campaign (no story) only the start. Unset when the
	 * level has neither.
	 */
	static FRespawnWakeSpot ChooseWakeSpot(const UWorld* World, const FCampaignRecord* Campaign);

	/** World has a roost of its own (one that isn't a trip's landing): Hob's crows work there. */
	static bool HasRoosts(const UWorld* World);

	// --- Found and the save point (RespawnMarkerSave.cpp) ---

	/** The roost in World with this id, or null. */
	static ARespawnMarker* FindById(const UWorld* World, FName Id);

	/** The area the level plays (the mission runner's, else its map's), None for a level that is no area's. */
	static FName FindAreaId(const UWorld* World);

	/**
	 * Area's save point as it stands in World: the roost the player last found there (FCampaignRecord::LastGraves) when it
	 * is here and open; else the latest found (MarkedGraves, newest first) still standing here and open, which takes over
	 * for a save naming a roost the level no longer has (the cut to two a map, 2026-10-14) or a find taken back; else null.
	 * Never a trip's landing, never one only opened and not found. A death wakes here, and so does Continue.
	 */
	static const ARespawnMarker* FindLastFound(const UWorld* World, const FCampaignRecord& Campaign, FName Area);

	/** The mission whose finish lets a walk find it: MarkAfterMission, else ActiveAfterMission, else None. */
	FName GetMarkGate() const;

	/** It has been found at some point in this story (its beacon is lit). */
	bool IsMarked(const FCampaignRecord& Campaign) const;

	/** Found or open: the map shows it, and the minimap from afar (one the story opened is a travel point). */
	bool IsFound(const FCampaignRecord& Campaign) const;

	/** The same in the story being played here; without a story, open from the start. */
	bool IsFound() const;

	/** It is Area's save point (FindLastFound): where a death and a Continue wake the player. */
	bool IsSavePoint(const FCampaignRecord& Campaign, FName Area) const;

	/** It is the save point of the story being played here (the map page's "Save point"). False without a story. */
	bool IsSavePoint() const;

	/** What the rules see for Player now (Check decides). bClearLine is left true: HasSightLine looks, only when the rest allows. */
	FGraveMarkSenses GatherSenses(const AActor* Player) const;

	/** A line from Player's eyes to the roost (its beacon, or its post at chest height) is clear of the world's solid shapes. */
	bool HasSightLine(const AActor* Player) const;

	/** Why Player's walk up to it finds nothing now (None: it does), looking at the line last. */
	EGraveMarkBlock CheckMark(const AActor* Player) const;

	/**
	 * The poll's work, for Player: finds it when the rules allow (see the class), with the beacon, sound, crows, banner and
	 * save soon. Returns what it did; None when it was blocked or it is the save point already. bForce skips the walk's rules
	 * (distance, sight, story, scene) for the console; a dead or missing player never finds it.
	 */
	EGraveMark TryMark(AActor* Player, bool bForce = false);

	/**
	 * Hob's crows have just set Player down here by fast travel (UGraveTravelSubsystem): arriving counts as walking up, so it
	 * becomes the save point. One never found before (the story opened it) is found, show and all; one found before becomes
	 * the save point quietly (the crows' Welcome is its show) and the session is saved soon. The walk's own rules hold but
	 * for the distance and the line (they stand on its spot): no find for a landing, a dead player or a closed story gate.
	 */
	EGraveMark MarkOnArrival(AActor* Player);

	// --- Its look and its crows (RespawnMarkerRoost.cpp) ---

	/** Its beacon is lit (it has been found). */
	bool IsLit() const { return bLit; }

	/** Lights or darkens it at once, no show (a level beginning with it found; a mark taken back; tests). */
	void SetLit(bool bInLit);

	/** Found (a first mark): the column shoots up, the light flares. Marked again: a pulse climbs it, the light lifts. */
	void StartFlare(EGraveMark Kind);

	/** Moves the light's flare and the bell's swing on by DeltaSeconds (the tick does; a test level never ticks, so tests call it). */
	void AdvanceLook(float DeltaSeconds);

	/** The light's flare is running. */
	bool IsFlaring() const { return FlareAge >= 0.f; }

	/** The light on the post now (candela), for the tests; zero when dark. */
	float GetLightCandela() const;

	/** The roost's model is in (not the stand-in). */
	bool HasModel() const { return RoostMesh != nullptr; }

	/** Where one of the roost's sockets is in the world (RoostKit's names): the model's, or the stand-in's. False without one. */
	bool GetRoostSocket(FName Socket, FTransform& OutWorld) const;

	/** Where the beacon burns (world): the Beacon socket. */
	FVector GetBeaconLocation() const;

	/** The crows' perches (world), Perch_1 to Perch_6. */
	TArray<FTransform> GetPerches() const;

	/** Hob's crows on it (made as play begins; null before). */
	ARoostCrows* GetCrows() const;

	/** The crows circling its beacon (made once it's found and play has begun; null while dark). */
	ARoostCrows* GetCirclingCrows() const;

	/** Rings its bell: it swings out on its loop and settles (Strength 0 to 1: a find 1, a mark again a tap). */
	void RingBell(float Strength);

	/** The bell's swing now (degrees off its rest), for the tests; 0 at rest. */
	float GetBellSwing() const;

	/**
	 * Hob's crows have just set the player down here (UGraveTravelSubsystem's arrival, UPlayerVitalsSubsystem's wake-up):
	 * the roost's crows burst off round them, the sound of it, a pulse up a lit beacon, and Hob's word
	 * (URoostCarrySubsystem::SayOnArrival).
	 */
	void Welcome(ERoostArrival Arrival, APawn* Player) const;

	/** Its sound is playing (the player is near). */
	bool IsAmbientPlaying() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Destroyed() override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

private:
	/** Its crows are its own: they go with it (an end of play, or a removal without one). */
	void DestroyCrows();

	void HandleMissionFinished(const UMissionDefinition& Mission, bool bRewarded);

	/** The timer's look at the player: its sound and its beacon near them, and walking up to it finds it. */
	void PollApproach();

	/** Says the find out loud: the sound at the bell, the crows, the banner for a first find, Hob's word, the save soon. */
	void Announce(EGraveMark Result, AActor* Player);

	/** Puts the model (or the stand-in) where its Wake socket is this spot, and the beacon and light on its sockets. */
	void FitRoost();
	void BuildGreybox();
	void ApplyLook();
	void ApplyLightFlare();

	/** Hob's crows on its perches, as play begins. */
	void SpawnCrows();

	/** Crows circling the beacon while it's lit (made in play only): on as it's found, gone as it goes dark. */
	void UpdateCirclingCrows();

	/** Its sound on while the player is within the settings' reach, off a little past it. */
	void UpdateAmbient(bool bNear);

	/** Moves the bell's swing on (the tick). */
	void SwingBell(float DeltaSeconds);

	/** Ticks only while the light flares or the bell swings. */
	void UpdateTicking();

	TWeakObjectPtr<UMissionRunner> BoundRunner;
	FDelegateHandle FinishedHandle;
	FTimerHandle PollTimer;

	bool bLit = false;
	/** Its play has begun (and not ended): only then does it make crows (never in the editor's construction). */
	bool bInPlay = false;
	/** Seconds into the light's flare (below zero: none), how long it lasts, and the kind. */
	float FlareAge = -1.f;
	float FlareLength = 1.f;
	EGraveMark FlareKind = EGraveMark::None;

	/** The bell's swing: how hard it was rung, seconds since, and its rest on the socket. */
	float BellStrength = 0.f;
	float BellTime = 0.f;
	FQuat BellRest = FQuat::Identity;

	TWeakObjectPtr<ARoostCrows> Crows;
	TWeakObjectPtr<ARoostCrows> CirclingCrows;
	TWeakObjectPtr<UAudioComponent> AmbientSound;

#if WITH_EDITORONLY_DATA
	/** The way the player faces on waking (the editor shows it; the game never does). */
	UPROPERTY()
	TObjectPtr<UArrowComponent> Arrow;
#endif
};
