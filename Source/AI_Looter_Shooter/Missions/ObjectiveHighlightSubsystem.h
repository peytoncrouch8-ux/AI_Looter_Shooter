#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ObjectiveHighlightSubsystem.generated.h"

class AActor;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UMeshComponent;
class UMissionRunner;
class UMissionSubsystem;

/** How a mesh wears the highlight: all of it, or a spot of it round the thing's point of use (a door in a house's mesh). */
enum class EObjectiveHighlightFit : uint8
{
	Whole,
	Spot,
};

/** One mesh wearing a highlight, and what was changed on it so the highlight is seen. */
struct FObjectiveHighlightMesh
{
	TWeakObjectPtr<UMeshComponent> Component;
	EObjectiveHighlightFit Fit = EObjectiveHighlightFit::Whole;
	/** Nanite draws no overlay, so on High and Epic a Nanite mesh shows its fallback (Medium's own look) while it's lit. */
	bool bForcedFallback = false;
};

/** One thing lit for the tracked objective, or easing in or out. */
struct FObjectiveHighlight
{
	TWeakObjectPtr<AActor> Actor;
	/** Its point of use, else the middle of what it shows: its distance is measured to it, and a spot is round it. */
	FVector Focus = FVector::ZeroVector;
	/** From the player at the last look (cm). */
	double Distance = 0.0;
	/** How far lit, 0 to 1, linear (the material gets it eased: ShownStrength). */
	float Strength = 0.f;
	/** It should be lit: one of the objective's, within range, nothing covering the game. */
	bool bWanted = false;
	/** Within range at the last look (a lit one lets go RangeMargin farther out than it takes hold). */
	bool bInRange = false;
	/** Its meshes were picked and given the highlight. */
	bool bLit = false;
	TArray<FObjectiveHighlightMesh> Meshes;
	/** Its instances of the highlight material, whole and spot; pooled (UObjectiveHighlightSubsystem::Looks keeps them). */
	UMaterialInstanceDynamic* WholeLook = nullptr;
	UMaterialInstanceDynamic* SpotLook = nullptr;
};

/**
 * The tracked objective's orange highlight (the user's ask, 2026-10-13): when a mission is tracked and the player comes
 * within a close radius (Looter.Highlight.Radius, 13 m) of what its current objective is about (the item to pick up, the
 * thing to use, the person or door to talk to, an egg sac to shoot, a readable, the giver once it waits to be turned in;
 * ObjectiveHighlightTargets has the rules, and never lights a creature), those things glow in the game's orange
 * (LooterUI::Color::Accent, the tracker's): a soft fill and a brighter rim, breathing slowly, easing in and out, gone when
 * the objective is done, the mission untracked, the player out of range or a scene playing.
 *
 * The look is an overlay material (M_ObjectiveHighlight, Tools/Unreal/build_highlight_material.py) put on the lit things'
 * own meshes: each is drawn once more, translucent, only while it's lit, so it costs nothing anywhere else and needs no
 * post process or custom depth (the same on Medium, which draws every mesh's Nanite fallback, as on Epic, where a lit
 * Nanite mesh shows its fallback while lit, since Nanite draws no overlay). A thing with no mesh of its own, or whose point
 * of use is part of something bigger (Delia's screen door in the farmhouse's mesh, a poster's wall, the bell's rope),
 * lights a spot of what's there, round that point.
 *
 * It looks a few times a second: the tracked mission's targets every second (sooner when the missions change), their
 * distances every RefreshInterval; per frame only while something is lit (the fades, and taking the overlay back after a
 * hit flash used it). At most MaxLit things at once, the nearest. Looter.Highlight.List says what's lit and why.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UObjectiveHighlightSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Seconds between looks at the distances. */
	static constexpr float RefreshInterval = 0.2f;
	/** Seconds between gathering the tracked mission's targets (the runner's changes bring it forward). */
	static constexpr float GatherInterval = 1.f;
	/** Within this of the player (cm) a target lights, unless Looter.Highlight.Radius says otherwise. */
	static constexpr float DefaultRadius = 1300.f;
	/** A lit thing lets go this much farther out (cm), so walking along the edge doesn't flicker it. */
	static constexpr float RangeMargin = 200.f;
	static constexpr float FadeInSeconds = 0.4f;
	static constexpr float FadeOutSeconds = 0.6f;
	/** The most things lit at once (the nearest). */
	static constexpr int32 MaxLit = 8;
	/** The most meshes one thing lights. */
	static constexpr int32 MaxMeshes = 12;
	/** A mesh bigger than this (its bounds' radius, cm) is a building or a rig: it lights a spot, never all of it. */
	static constexpr float LargeMeshRadius = 400.f;
	/** A spot's radius round the point of use (cm): about a door's height from its middle. */
	static constexpr float SpotRadius = 140.f;
	/** How near the point of use a mesh is looked for, for a thing with none of its own there (cm). */
	static constexpr float SpotSearchRadius = 100.f;
	/** Nothing this big lights even a spot (cliffs, the ground): its whole redraw would cost more than the spot is worth (cm). */
	static constexpr float MaxSpotMeshRadius = 3000.f;
	/** The highlight's material (made by Tools/Unreal/build_highlight_material.py). */
	static const TCHAR* MaterialPath;

	static UObjectiveHighlightSubsystem* Get(const UObject* WorldContextObject);

	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;

	/**
	 * Moves its clock on: the targets gathered every GatherInterval (at once after the missions change), the distances every
	 * RefreshInterval, the fades every call. The tick calls it; tests too.
	 */
	void Advance(float DeltaSeconds);

	/** Looks now: the tracked mission's targets gathered again, then who's in range (and lit as they come in). */
	void Refresh();

	/** Every highlight off at once, its meshes as they were. */
	void ClearAll();

	/** The things lit or easing, as of the last look. */
	const TArray<FObjectiveHighlight>& GetHighlights() const { return Highlights; }

	/** Actor's highlight, or null when it has none (not lit, nor easing out). */
	const FObjectiveHighlight* FindHighlight(const AActor* Actor) const;

	/** The actors the last gather found for the tracked mission (in range or not). */
	TArray<AActor*> GetCandidates() const;

	/** Its material loaded (Main runs build_highlight_material.py once; without it the rules still run, nothing is drawn). */
	bool HasMaterial();

	// --- The rules, apart from the world (tests) ---

	/** The radius in effect (cm): Looter.Highlight.Radius, else DefaultRadius. */
	static float GetRadius();

	/** In range now: within Radius, or within Radius + RangeMargin for one already in range. */
	static bool IsWithinRange(double Distance, bool bWasInRange, double Radius);

	/** Strength moved on by DeltaSeconds toward 1 (wanted, over FadeInSeconds) or 0 (over FadeOutSeconds). */
	static float StepStrength(float Strength, bool bWanted, float DeltaSeconds);

	/** What the material shows for a strength: eased at both ends. */
	static float ShownStrength(float Strength);

	/** Where Actor's distance is measured and its spot centred: its point of use (IInteractable), else the middle of its meshes. */
	static FVector FocusOf(const AActor& Actor);

	/**
	 * The meshes Actor lights, and how (ObjectiveHighlightSubsystemLook.cpp): its own small opaque meshes whole; its big
	 * ones as a spot round Focus; and for a thing used at a point away from them (or with no mesh of its own), what's there
	 * as a spot. Never a see-through mesh (glass, a beam), a leafy card, instanced scatter, or the ground.
	 */
	static void PickMeshes(const AActor& Actor, const FVector& Focus, TArray<FObjectiveHighlightMesh>& Out);

private:
	/** The level's mission runner while it plays missions, else null. */
	UMissionRunner* ActiveRunner() const;
	/** The tracked mission's targets, into Candidates. */
	void Gather(const UMissionRunner& Runner);
	/** Who's wanted now: the candidates in range of the player (nearest first, MaxLit), nothing covering the game; lights the newly wanted. */
	void LookAround();
	/** Moves every strength on; drops a highlight faded out, or whose actor is gone. */
	void Fade(float DeltaSeconds);
	/** Nothing is lit now: the highlight is off (Looter.Highlight.Enabled 0), or a scene plays. */
	bool IsCovered() const;
	void BindMissions(UMissionRunner& Runner);
	void HandleMissionsChanged();
	FObjectiveHighlight* FindMutable(const AActor* Actor);

	// ObjectiveHighlightSubsystemLook.cpp
	/** Picks its meshes and puts the highlight on them. */
	void Light(FObjectiveHighlight& Highlight);
	/** Takes the highlight off its meshes, as they were; its instances back to the pool. */
	void Unlight(FObjectiveHighlight& Highlight);
	/** Puts the overlay back on a lit mesh whose own was cleared (a hit flash ending), and keeps Nanite off it. */
	void Reclaim();
	/** Its strength (and a spot's middle) into its instances. */
	void ShowStrength(FObjectiveHighlight& Highlight) const;
	void ApplyTo(FObjectiveHighlightMesh& Mesh, UMaterialInstanceDynamic* Look) const;
	UMaterialInstanceDynamic* TakeLook(float MaskRadius);
	void ReturnLook(UMaterialInstanceDynamic*& Look);
	UMaterialInterface* LoadMaterial();

	TArray<FObjectiveHighlight> Highlights;
	TArray<TWeakObjectPtr<AActor>> Candidates;

	/** Every instance of the highlight material made (kept alive here); FreeLooks are the ones not in use. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> Looks;
	TArray<UMaterialInstanceDynamic*> FreeLooks;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> HighlightMaterial;
	/** Looked for the material once (a missing one is warned about once, not every look). */
	bool bMaterialLookedFor = false;

	float SinceRefresh = 0.f;
	/** Starts due, so the first look gathers. */
	float SinceGather = GatherInterval;
	/** The missions changed since the last look: the next frame looks again, gathering. */
	bool bDirty = false;

	TWeakObjectPtr<UMissionRunner> BoundRunner;
	FDelegateHandle RunnerHandle;
	TWeakObjectPtr<UMissionSubsystem> BoundDisplay;
	FDelegateHandle DisplayHandle;
};
