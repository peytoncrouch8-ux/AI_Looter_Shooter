#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "UI/Style/MissionMarkIcons.h"
#include "GiverMarkSubsystem.generated.h"

class AActor;
class AGiverMarkActor;
class UMissionRunner;

/** One giver's mark as the missions stand: who, where it floats, what it says. */
struct AI_LOOTER_SHOOTER_API FGiverMark
{
	TWeakObjectPtr<AActor> Giver;
	/** Over the giver's head (world cm). */
	FVector Location = FVector::ZeroVector;
	MissionMarkIcons::EMark Mark = MissionMarkIcons::EMark::Offer;
	bool bMain = false;
	/** The mission the mark is for (a turn-in over an offer, a main one over a side one). */
	FName MissionId;
	/** Every mission this giver offers now. */
	TArray<FName> Offers;
};

/**
 * The givers' marks: a ? over everyone, every board and every artifact with a mission to offer now
 * (UMissionRunner::IsAvailable), a ! over whoever a mission waits to be turned in to (the user's call, 2026-10-13: the
 * other way round from Borderlands; a turn-in comes first, as a talk turns it in first), gold for the story, white for side jobs. A few times a second it reads the missions and the level's
 * interactables (givers are all things the player uses), keeps the marks for the minimap (UHudMinimapWidget), floats one
 * AGiverMarkActor over each marked giver (labels over the world, hidden under menus and scenes), and the first time a
 * mission is offered within 40 m of the player it chimes (Giver::NewOffer: once per mission per play, never saved; not
 * for what was offered nearby as the level began, and held while a menu or a scene covers the game).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UGiverMarkSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Seconds between looks at the missions. */
	static constexpr float RefreshInterval = 0.25f;

	/** The marks show over givers this near the player (cm), and a mission newly offered this near chimes. */
	static constexpr float ShowRange = 4000.f;

	/** Seconds after the level begins in which what's offered nearby is taken as already known (no chime). */
	static constexpr float SettleSeconds = 2.f;

	static UGiverMarkSubsystem* Get(const UObject* WorldContextObject);

	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;

	/** Reads the missions and the givers again now: the marks, the chime, the marks over the world. Advance does it; tests may. */
	void Refresh();

	/** Moves its clock on: the settling time, and a look every RefreshInterval (sooner after the missions change). The tick calls it; tests too. */
	void Advance(float DeltaSeconds);

	/** The marks as they stood at the last look. */
	const TArray<FGiverMark>& GetMarks() const { return Marks; }

	/** The marks a runner's missions put on the givers among Candidates (hidden ones left out). */
	static TArray<FGiverMark> GatherMarks(const UMissionRunner& Runner, TConstArrayView<AActor*> Candidates);

	/** Where a giver's mark floats: over a speaker's head, over a board, a little over an artifact. */
	static FVector MarkSpot(const AActor& Giver);

	/** The first offer of Mission near the player has chimed (or was there already) this play. */
	bool HasChimed(FName MissionId) const { return Chimed.Contains(MissionId); }

	/** How many times it has chimed this play (tests). */
	int32 GetChimeCount() const { return ChimeCount; }

	/** The labels over the world show (no menu over the game, no scene): the HUD sets it every frame. */
	void SetLabelsShown(bool bShown) { bLabelsShown = bShown; }

private:
	/** The level's interactables still standing and shown: every giver is something the player uses. */
	TArray<AActor*> FindCandidates() const;
	void Chime(const UMissionRunner& Runner);
	/** One mark actor per marked giver, the rest retired (played worlds only: a test level has no screen to float them on). */
	void SyncWorldMarks();
	void HandleRunnerChanged();

	TArray<FGiverMark> Marks;
	TSet<FName> Chimed;
	int32 ChimeCount = 0;
	bool bLabelsShown = true;
	float SinceRefresh = 0.f;
	/** Seconds of play looked at so far (the settling time). */
	float Settled = 0.f;
	/** The missions changed since the last look: the next tick looks again at once (an offer's ? comes up as a turn-in ends). */
	bool bDirty = false;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AGiverMarkActor>> WorldMarks;

	TWeakObjectPtr<UMissionRunner> BoundRunner;
	FDelegateHandle ChangedHandle;
};
