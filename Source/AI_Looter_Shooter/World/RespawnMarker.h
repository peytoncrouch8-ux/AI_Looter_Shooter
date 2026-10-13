#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "World/GraveMarkRules.h"
#include "RespawnMarker.generated.h"

class ARespawnMarker;
class UArrowComponent;
class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UMissionDefinition;
class UMissionRunner;
class UPointLightComponent;
class UStaticMesh;
class UStaticMeshComponent;
struct FCampaignRecord;

/** Where a death or a Continue wakes the player (ARespawnMarker::ChooseWakeSpot): a grave, or the level's own start. */
struct AI_LOOTER_SHOOTER_API FRespawnWakeSpot
{
	/** A grave's spot on the ground (the player stands on it), or a player start's (the middle of the player goes there). */
	FVector Location = FVector::ZeroVector;

	/** Which way they face: the grave's or the start's, level. */
	FRotator Facing = FRotator::ZeroRotator;

	/** The grave they wake at; null at the level's start. */
	const ARespawnMarker* Grave = nullptr;

	/** The level's start they wake at when no grave is open; null at a grave. */
	const AActor* Start = nullptr;

	/** Somewhere was found (a level with neither a grave open nor a start of its own has nowhere). */
	bool IsSet() const { return Grave || Start; }
};

/**
 * A respawn grave (the family plot, the chapel yard, boot hill, the keeper's grave), and Borderlands' save station: a death
 * wakes the player at the open one nearest where they fell, standing on it and facing its arrow, or else at the level's own
 * start as before, never at a trip's landing (UPlayerVitalsSubsystem). The wake-up is the death fade for now; the grave
 * wake-up camera is a scene.
 *
 * A grave opens with the story: it's open from the start (bStartActive), opens when ActiveAfterMission is finished (Main 1
 * opens the family plot, Main 4 the chapel yard), when the player walks up to it (below), or from the console. Openings are
 * kept in the session's campaign record by MarkerId, so they last through saves and trips. Place it on the ground beside its
 * grave, where the player gets up; it's never streamed out, since the nearest open grave can be far from where the player
 * fell.
 *
 * Save points (the user's call, 2026-10-13): walking up to a grave (alive, outside a scene, within MarkRadius with a clear
 * line to its lantern) marks it. Its lantern, a post beside the grave (SM_LanternPost by default; the build scripts place
 * it), is dark until then. The first mark opens the grave if the story hadn't, makes it the last marked grave of its area
 * (FCampaignRecord::LastGraves, which Continue wakes at), catches the lantern with a flare of light and flame, sounds
 * (GraveMark::Marked), shows the "GRAVE MARKED" banner and asks the session to save soon. Walking up to a grave marked
 * before, when another is the save point, only moves the save point back: the flame lifts (GraveMark::Remarked), no banner.
 * A marked grave stays lit for good. Death and fast travel are unchanged: the nearest open grave, and the open graves.
 *
 * It checks the player a few times a second on a timer, never every frame; its lantern ticks only for the second of a flare.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ARespawnMarker : public AActor
{
	GENERATED_BODY()

public:
	ARespawnMarker();

	/**
	 * Its name in the campaign record ("FamilyPlot", "KeepersGrave"): unique in the campaign, and never changed once a
	 * session may have saved it. None: the actor's name, which a rebuilt level may change.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Respawn")
	FName MarkerId;

	/** Open from the start. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Respawn")
	bool bStartActive = false;

	/** Opens when this mission is finished (its id). None: only the start, a walk up to it or the console opens it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Respawn")
	FName ActiveAfterMission;

	/** What the map calls it ("The family plot"). Empty: its id in words ("Family Plot" for FamilyPlot). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Respawn")
	FText DisplayName;

	// --- Save point (marking on approach) ---

	/** Walking up to it marks it as the save point. Off: only the console marks it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Save point")
	bool bMarkOnApproach = true;

	/** How close the player's feet come to mark it (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Save point", meta = (ClampMin = "50", Units = "cm"))
	float MarkRadius = 600.f;

	/**
	 * It can't be marked before this mission is finished (its id). None: ActiveAfterMission's, so a grave the story opens
	 * can't be opened early by a walk; with neither, any time.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Save point")
	FName MarkAfterMission;

	/** The lantern's model, a post with the lantern hanging from it (glass slot LanternGlow, a Light socket at the flame). Set by the build scripts. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Save point|Lantern")
	TObjectPtr<UStaticMesh> LanternMesh;

	/** Where the lantern stands, from the marker's spot in its own frame (cm): beside it, so the player never wakes inside the post. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Save point|Lantern")
	FVector LanternOffset = FVector(0.0, 130.0, 0.0);

	/** The lantern's turn from the marker's (degrees): the model's front, where its lantern hangs, toward the marker's spot. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Save point|Lantern", meta = (Units = "deg"))
	float LanternYaw = -90.f;

	/** The model's glass slot, and what it shows while the lantern is dark. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Save point|Lantern")
	FName GlassSlot = TEXT("LanternGlow");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Save point|Lantern")
	TObjectPtr<UMaterialInterface> DarkGlass;

	/** The lit lantern's light at rest (candela): a first mark spikes it and it eases down to this. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Save point|Lantern", meta = (ClampMin = "0"))
	float LanternCandela = 10.f;

	/** The flame's glow: three crossed cards this wide (cm) and this strong, round the lantern's glass (the glass is opaque, so the glow rings it). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Save point|Lantern", meta = (ClampMin = "1", Units = "cm"))
	float FlameSize = 56.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Save point|Lantern", meta = (ClampMin = "0"))
	float FlameStrength = 5.f;

	// --- Components ---

	/** The lantern post. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Lantern;

	/** Its light once the grave is marked: warm, small, never a shadow. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPointLightComponent> LanternLight;

	/** Its flame: crossed glow cards on the lantern's Light socket (one instanced draw), shown once the grave is marked. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> Flame;

	/** MarkerId, or the actor's name without one. */
	FName GetMarkerId() const;

	/** DisplayName, or the id in words. */
	FText GetGraveName() const;

	/**
	 * Where the player gets up here (a death's wake-up, a fast travel's arrival, a Continue): standing on its spot, facing
	 * its arrow, level. The spot is on the ground: the player's middle goes half their height above it.
	 */
	FRespawnWakeSpot GetWakeSpot() const;

	/** It's open in this story: from the start, recorded open, or its mission finished. */
	bool IsActive(const FCampaignRecord& Campaign) const;

	/** Records it open in Campaign. True when it wasn't recorded before. */
	bool Activate(FCampaignRecord& Campaign) const;

	/** Why it's open ("open from the start", "open: Main1 finished", "opened"), or "closed", for logs and the console. */
	FString DescribeState(const FCampaignRecord& Campaign) const;

	/** The open grave nearest From in World, never one that is also a trip's landing; null when none is open. */
	static ARespawnMarker* FindNearestActive(const UWorld* World, const FVector& From, const FCampaignRecord& Campaign);

	/**
	 * Where a death at DeathLocation wakes the player: the nearest open grave, else the level's own start, never a trip's
	 * landing. Without a campaign (no story to have opened anything) only the start. Unset when the level has neither.
	 */
	static FRespawnWakeSpot ChooseWakeSpot(const UWorld* World, const FVector& DeathLocation, const FCampaignRecord* Campaign);

	// --- Save points (RespawnMarkerSave.cpp) ---

	/** The grave in World with this id, or null. */
	static ARespawnMarker* FindById(const UWorld* World, FName Id);

	/** The area the level plays (the mission runner's, else its map's), None for a level that is no area's. */
	static FName FindAreaId(const UWorld* World);

	/**
	 * The grave a Continue wakes the player at: the last one marked in Area when it's here and open, else the open grave
	 * nearest SavedLocation, else null (they stand where they were saved, as before graves). Never a trip's landing.
	 */
	static const ARespawnMarker* ChooseContinueGrave(const UWorld* World, const FVector& SavedLocation, const FCampaignRecord& Campaign,
		FName Area);

	/** The mission whose finish lets a walk mark it: MarkAfterMission, else ActiveAfterMission, else None. */
	FName GetMarkGate() const;

	/** It has been marked at some point in this story (its lantern is lit). */
	bool IsMarked(const FCampaignRecord& Campaign) const;

	/** It is Area's save point: the last grave marked there, where Continue wakes. */
	bool IsSavePoint(const FCampaignRecord& Campaign, FName Area) const;

	/** It is the save point of the story being played here (the map page's "Save point"). False without a story. */
	bool IsSavePoint() const;

	/** What the rules see for Player now (Check decides). bClearLine is left true: HasSightLine looks, only when the rest allows. */
	FGraveMarkSenses GatherSenses(const AActor* Player) const;

	/** A line from Player's eyes (or the grave's own spot) to the lantern is clear of the world's solid shapes. */
	bool HasSightLine(const AActor* Player) const;

	/** Why Player's walk up to it marks nothing now (None: it marks), looking at the line last. */
	EGraveMarkBlock CheckMark(const AActor* Player) const;

	/**
	 * The poll's work, for Player: marks the grave when the rules allow (see the class), with the lantern, sound, banner and
	 * save soon. Returns what it did; None when it was blocked or it is the save point already. bForce skips the walk's
	 * rules (distance, sight, story, scene) for the console; a dead or missing player never marks.
	 */
	EGraveMark TryMark(AActor* Player, bool bForce = false);

	// --- The lantern (RespawnMarkerLantern.cpp) ---

	/** Its lantern is lit (the grave has been marked). */
	bool IsLit() const { return bLit; }

	/** Lights or darkens the lantern at once, no flare (a level beginning with the grave marked; tests). */
	void SetLit(bool bInLit);

	/** Lights it with a flare (a first mark) or lifts the flame (a mark again): the light spikes and eases down over about a second. */
	void StartFlare(EGraveMark Kind);

	/** Moves a flare on by DeltaSeconds (the tick does; a test level never ticks, so tests call it). */
	void AdvanceLantern(float DeltaSeconds);

	/** A flare is running. */
	bool IsFlaring() const { return FlareAge >= 0.f; }

	/** The light's intensity now (candela), for the tests; zero when dark. */
	float GetLanternCandela() const;

	/** The flame's height scale now, for the tests (1 at rest). */
	float GetFlameScale() const;

	/** Where the flame burns (world): the lantern's Light socket, or its place on the post without the model. */
	FVector GetLanternLocation() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void HandleMissionFinished(const UMissionDefinition& Mission, bool bRewarded);

	/** The timer's look at the player: walking up to the grave marks it. */
	void PollApproach();

	/** Says the mark out loud: the sound at the lantern, the banner for a first mark, the session asked to save soon. */
	void Announce(EGraveMark Result, AActor* Player);

	/** Puts the lantern's pieces where the model's sockets are, its model in, its glass lit or dark, its light and flame shown or not. */
	void FitLantern();
	void ApplyLantern();
	void ApplyFlare();
	/** The flame's cards: three tongues crossed about its axis and a round heart. */
	void BuildFlame();

	TWeakObjectPtr<UMissionRunner> BoundRunner;
	FDelegateHandle FinishedHandle;
	FTimerHandle PollTimer;

	bool bLit = false;
	/** Seconds into a flare (below zero: none), how long it lasts, and the kind. */
	float FlareAge = -1.f;
	float FlareLength = 1.f;
	EGraveMark FlareKind = EGraveMark::None;

	/** The model's own glass material, kept for lighting the lantern again after the dark one. */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> LitGlass;

#if WITH_EDITORONLY_DATA
	/** The way the player faces on waking (the editor shows it; the game never does). */
	UPROPERTY()
	TObjectPtr<UArrowComponent> Arrow;
#endif
};
