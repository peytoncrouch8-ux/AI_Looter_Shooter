#pragma once

#include "CoreMinimal.h"
#include "Audio/CreatureVoiceBarks.h"
#include "Engine/TimerHandle.h"
#include "Math/RandomStream.h"
#include "Subsystems/WorldSubsystem.h"
#include "CreatureVoiceDirector.generated.h"

class ACreatureBarkActor;
class UCreatureVoiceComponent;

/**
 * The level's creature voices together: every creature's voice joins as its play begins. It keeps the barks in check
 * (FCreatureBarkBoard: at most one on screen near the player, a pause after each, each kind resting a while), picks each
 * line (never one said lately) and shows it over its speaker (one ACreatureBarkActor, reused), while the speaker murmurs
 * it. Every IdleCheckSeconds one creature near the player at rest may mutter (an Unpaid) or make its idle call (a spider's
 * chitter, a slime's gurgle); when a creature dies, the nearest of its pack may answer. Timers only, no tick, and nothing
 * walks the level's actors. Played worlds, and the editor preview worlds the automated tests build.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UCreatureVoiceDirector : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	static UCreatureVoiceDirector* Get(const UObject* WorldContextObject);

	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void Deinitialize() override;

	/** A creature's voice joins (as its play begins) or leaves (as it ends). */
	void Register(UCreatureVoiceComponent* Voice);
	void Unregister(UCreatureVoiceComponent* Voice);

	/**
	 * Speaker would say a bark of Situation now: if the board lets it (near the player, nothing more important on screen,
	 * the pauses over), its line for its rank goes up over it and it murmurs it. True when it was said.
	 */
	bool RequestBark(UCreatureVoiceComponent& Speaker, ECreatureBark Situation);

	/** A creature died: the nearest living one of its pack within PackmateRadius may say something about it. */
	void NotifyDeath(const UCreatureVoiceComponent& Dead);

	/** One look for a creature at rest near the player to mutter or call (the timer's; tests call it too). */
	void CheckIdle();

	/** Where the barks are heard from: a test's stand-in, else the first local player's pawn, else its camera. Unset: nobody. */
	TOptional<FVector> GetListener() const;

	/** A test's stand-in for the player: a test level has no player controller. */
	void SetTestListener(AActor* StandIn);

	/** The barks' board, the line said last (an index into CreatureBarks::AllLines, or INDEX_NONE) and who said it. */
	const FCreatureBarkBoard& GetBoard() const { return Board; }
	int32 GetLastLine() const { return RecentLines.IsEmpty() ? INDEX_NONE : RecentLines.Last(); }
	const UCreatureVoiceComponent* GetLastSpeaker() const { return LastSpeaker.Get(); }

	/** The words over the speaker (spawned with the first bark), or null. */
	ACreatureBarkActor* GetBarkActor() const { return BarkActor.Get(); }

	/** The voices registered now. */
	int32 NumVoices() const;

	/** The dice, fixed for a test. */
	void SetRandomSeed(int32 Seed) { Random.Initialize(Seed); }

	/** Seconds between idle looks; how near the player a creature at rest is looked at (cm); how near a packmate answers. */
	static constexpr float IdleCheckSeconds = 3.f;
	static constexpr float IdleRadius = 2000.f;
	static constexpr float PackmateRadius = 1500.f;

	/** A spider's or slime's idle call: how likely a picked one makes it, and the pauses after (s): any creature's, its own. */
	static constexpr float IdleCallChance = 0.3f;
	static constexpr float IdleCallGap = 8.f;
	static constexpr float IdleCallRestMin = 25.f;
	static constexpr float IdleCallRestMax = 45.f;

private:
	void UpdateTimer();
	ACreatureBarkActor* FindOrSpawnBarkActor();

	TArray<TWeakObjectPtr<UCreatureVoiceComponent>> Voices;
	FCreatureBarkBoard Board;
	/** The lines said lately, oldest first (at most CreatureBarks::RecentLines). */
	TArray<int32> RecentLines;
	TWeakObjectPtr<UCreatureVoiceComponent> LastSpeaker;
	TWeakObjectPtr<ACreatureBarkActor> BarkActor;
	TWeakObjectPtr<AActor> TestListener;
	FTimerHandle IdleTimer;
	FRandomStream Random = FRandomStream(20261009);
	/** No idle call from any creature before this (world seconds). */
	double NextIdleCall = 0.0;
};
