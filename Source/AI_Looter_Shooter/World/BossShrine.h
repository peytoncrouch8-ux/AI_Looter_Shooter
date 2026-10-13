#pragma once

#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "Session/BossRecallRecord.h"
#include "Story/StoryCondition.h"
#include "World/BossShrineRules.h"
#include "BossShrine.generated.h"

class AAbelKeeper;
class ACreatureBase;
class AEncounterSpawner;
class APawn;
class UBossComponent;
class UPointLightComponent;
class USceneComponent;
class UStaticMeshComponent;

/** How a vigil stone brings its boss back. */
UENUM(BlueprintType)
enum class EBossShrineCall : uint8
{
	/** Its boss's lair (an AEncounterSpawner with a LegendaryId: the Gravemother's den) brings it anew. */
	Lair,
	/** Its boss stands placed in the level (Abel on the burial deck) and is brought back to life at its spot. */
	Placed,
};

/** What a vigil stone's menu shows (UBossShrineWidget), read afresh each frame it's open. */
struct FBossShrineView
{
	EBossShrineState State = EBossShrineState::Dormant;
	double SecondsLeft = 0.0;
	FText BossName;
	int32 Level = 0;
	FText LastKill;
	int32 TimesBeaten = 0;
	FText LootHint;
	FText Status;
	bool CanCall() const { return State == EBossShrineState::Ready; }
};

/**
 * A vigil stone (the user's call, 2026-10-12: "a pedestal with a menu that spawns next to the boss arenas after you kill the
 * boss the first time so you can respawn the boss. 5 minute wait, or the boss respawns after a save and quit"): in the
 * Reaches the keepers sit the night with their dead by candlelight, and a stone where the candles are lit and the iron bell
 * is rung calls the laid-to-rest back to walk again. One stands beside each boss arena, placed by
 * Tools/Unreal/build_area_shrines.py where the player ends up after the fight and off its ground: at the Sink's ramp head
 * for the Gravemother, outside the Keeper's Gate for Abel. Its rules are BossShrineRules'.
 *
 *  - Until its boss is first beaten it isn't there (hidden, nothing to meet). After that it rises out of the ground the
 *    first time the player comes near (grave dirt thrown up, the candles catching), once no scene is playing and the boss
 *    is gone; the session remembers it risen, so later visits find it standing.
 *  - A tap of Interact ("Keep vigil") opens its menu (UBossShrineWidget): the boss's name and level, its last laying to
 *    rest, how many times it was beaten, what it drops, and "Call it back", ready while the boss isn't out there and five
 *    minutes of play have passed since its last death, or the session was loaded since (a Save & Quit). Otherwise the
 *    countdown (mm:ss) and the button refuses. Its candles burn while it's ready and are out otherwise.
 *  - Calling it back rings the bell: the candles flare and go out, soul-light rises at the stone and in the arena, the boss
 *    answers from its arena. A lair brings its boss anew (AEncounterSpawner::TriggerWave); a placed boss is readied at its
 *    spot (UBossComponent::PrepareRematch). Either way its fight starts as the first did (its seal, intro, phases), its
 *    story kept as it is: no scene, no mission, no story reward again. Its loot shower is its own, with a little less luck
 *    (UBossComponent::RematchLuckDrop). Abel: Pa leaves his board while the Keeper walks the boards again; beaten, he
 *    kneels, and before "Sit with Pa" could play again he's back on his board (BossShrineCallBack.cpp).
 *  - A lair's boss away for this visit (the Gravemother's twenty-minute rule) comes back as a level loaded from the session
 *    begins after a Save & Quit (BossShrineRules::ComesBackOnLoad): the user's "or the boss respawns after a save and quit".
 *
 * Its models are Art/Models/Props/VigilStone.py's (SM_VigilStone, the bell SM_VigilStoneBell on its SOCKET_Bell, the candle
 * flames SM_VigilStoneFlames), found once imported; plain shapes stand in until then. Solid, tagged VigilStone (and Obstacle
 * for the minimap once it stands). BossShrine.cpp is its life, state and show; BossShrineCallBack.cpp calls its boss back.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ABossShrine : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	/** The tag the map and the console find vigil stones by. */
	static const FName ShrineTag;

	/** Art/Models/Props/VigilStone.py's models. */
	static const TCHAR* const ModelPath;
	static const TCHAR* const BellPath;
	static const TCHAR* const FlamesPath;

	/** SM_VigilStone's sockets: where it's used from, where the bell hangs, the candles' middle. */
	static const FName InteractSocket;
	static const FName BellSocket;
	static const FName LightSocket;

	ABossShrine();

	virtual void Tick(float DeltaSeconds) override;

	// --- IInteractable ---
	virtual FInteractionOptions GetInteractionOptions(const UInteractionComponent& User) const override;
	virtual bool Interact(UInteractionComponent& User, bool bHeld) override;
	virtual TOptional<FVector> GetInteractionLocation() const override;

	// --- Its boss (the build script's settings) ---

	/** Its boss's name in the session (FBossRecallRecord's key; for Abel also his campaign boss id): "Gravemother", "Abel". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vigil Stone")
	FName BossKey;

	/** Its boss's class: a death of one of these is its boss's (a lair's spawn, a placed one). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vigil Stone")
	TSubclassOf<ACreatureBase> BossClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vigil Stone")
	EBossShrineCall CallKind = EBossShrineCall::Lair;

	/** Lair: the encounter that brings its boss (none: the level's spawner whose SpawnerId is BossKey). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vigil Stone")
	TObjectPtr<AEncounterSpawner> Lair;

	/** Placed: its boss in the level (none: the first of BossClass found). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vigil Stone")
	TObjectPtr<ACreatureBase> PlacedBoss;

	/** The name the menu gives it ("The Gravemother"). Empty: its boss's bar name, else its creature's name. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vigil Stone")
	FText BossName;

	/** What it drops, after "Drops:" ("Two guns from a Legendary's table, and ammo"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vigil Stone")
	FText LootHint;

	/**
	 * The story counts its boss beaten (a save from before the stones: Side 3 or Main 6 finished). The stone also knows from
	 * its own record, the campaign's beaten bosses (BossKey) and its lair's Legendary death.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vigil Stone")
	FStoryCondition BeatenWhen;

	/** It can be called back only while this holds (Abel: after Main 6). Empty: always. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vigil Stone")
	FStoryCondition CallableWhen;

	/** Its boss's answer from its arena a moment after the bell (its roar, its wail). None: silence. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vigil Stone")
	FName AnswerCue;

	/** The prompt's words. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vigil Stone")
	FText Prompt;

	/** It rises the first time the player comes this near (cm) after its boss was beaten. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vigil Stone", meta = (ClampMin = "0", Units = "cm"))
	float RiseRadius = 3500.f;

	/** How far under the ground it starts its rise (cm), and how long the rise takes (s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vigil Stone", meta = (ClampMin = "0", Units = "cm"))
	float RiseDepth = 170.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vigil Stone", meta = (ClampMin = "0.1", Units = "s"))
	float RiseSeconds = 2.6f;

	/** The candles' light (candela) and reach (cm): small, warm, never a shadow. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vigil Stone", meta = (ClampMin = "0"))
	float CandleCandela = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vigil Stone", meta = (ClampMin = "50", Units = "cm"))
	float CandleRadius = 420.f;

	// --- Where it stands ---

	/** Its state now (read again four times a second, and whenever it's asked to Refresh). */
	EBossShrineState GetState() const { return Verdict.State; }

	/** Play left before its boss can be called back (while Waiting). */
	double GetSecondsLeft() const { return Verdict.SecondsLeft; }

	/** It stands beside the arena (risen, or rising). */
	bool IsRisen() const { return bRisen; }
	bool IsRising() const { return bRising; }

	/** Its candles burn (it's ready). */
	bool AreCandlesLit() const { return bCandlesLit; }

	/** Its boss was beaten at least once in this session (or the story says so). */
	bool HasBossBeenBeaten() const;

	/** Its boss is out there now: alive in its arena, or due there as the player comes (its lair on and not cleared). */
	bool IsBossPresent() const;

	/** The boss's name (BossName, else its bar's, else its creature's). */
	FText GetBossName() const;

	/** The boss's level: as it is when out there, else as it would rise now (its kind's set level in its area, or the levels where it stands). */
	int32 GetBossLevel() const;

	/** Everything its menu shows. */
	FBossShrineView GetView() const;

	/** The map pin's second line ("Ready to call back", "Ready in 3:12", "Out there now"). */
	FText GetMapDetail() const;

	// --- Using it ---

	/** Opens its menu for Player (their pawn or controller), as a tap of Interact does. False when it can't. */
	bool Use(AActor* Player);

	/**
	 * Calls its boss back now (the menu's button): the bell, the candles out, the boss readied in its arena. False, with
	 * the refusal's sound, when it isn't Ready or nothing could come back.
	 */
	bool CallBack(APawn* Player = nullptr);

	/** Reads its state again now: its boss, its record, the story; rises when it should; lights or puts out its candles. */
	void Refresh();

	/** Stands now: with bShow it rises with its dirt and sound (the first time), else it's simply there. */
	void Rise(bool bShow);

	/**
	 * A lair's boss away for this visit, beaten before the session was loaded (a Save & Quit since), comes back now as the
	 * level begins (BeginPlay asks on its next frame). Whether it came.
	 */
	bool BringBackAfterLoad();

	/** Moves its rise, bell, candles and a call back's moments on by DeltaSeconds (its tick does; the tests call it). */
	void Advance(float DeltaSeconds);

	/** The player it judges by: the first local player's pawn (a test level's player stand-in). */
	APawn* FindPlayer() const;

	/** For the tests (a test level has no session): the time played and the session's loads to judge by. Unset: the session's. */
	struct FTestClock
	{
		TOptional<double> PlayedSeconds;
		TOptional<int32> Loads;
	};
	FTestClock TestClock;

	// --- Components ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	/** What rises out of the ground: the stone, its bell, flames and light hang off it. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Body;

	/** The stone, its steps, the unlit candles and the iron crook: SM_VigilStone, or a plain block until it's imported. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Stone;

	/** The iron hand bell on the crook's SOCKET_Bell (it swings when rung). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Bell;

	/** The candles' flames (a glowing surface), shown while they burn. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Flames;

	/** The candles' warm light at SOCKET_Light, on while they burn. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPointLightComponent> CandleLight;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Its tick's pace while nothing moves and the player isn't near (s): four looks a second. */
	static constexpr float IdleTickSeconds = 0.25f;

	// --- BossShrine.cpp: its life, its boss and its state ---
	void HandleAnyBossWon(UBossComponent& Boss);
	bool IsOwnBoss(const ACreatureBase& Creature) const;
	/** Its lair or placed boss, found when the build script left them unset. */
	void FindBoss();
	/** The record the session keeps of its boss (or, with no session, the stone's own). */
	const FBossRecallRecord* GetRecord() const;
	double GetPlayedNow() const;
	int32 GetLoadsNow() const;
	bool StoryHolds(const FStoryCondition& Condition) const;
	/** What its state is read from now (BossShrineRules::Evaluate's inputs). */
	BossShrineRules::FInputs ReadInputs() const;
	/** Where its boss comes back: its lair, or its placed boss's spot. */
	FVector GetArenaSpot() const;

	// --- BossShrineShow.cpp: its show and frames ---
	/** Out of the world (hidden, nothing to meet) until it rises. */
	void SetStanding(bool bStanding);
	void SetCandles(bool bLit, bool bShow);
	/** The bell rung: it swings out and settles (Strength 0-1). */
	void SwingBell(float Strength);
	void TickRise(float DeltaSeconds);
	void TickBell(float DeltaSeconds);
	void TickCandles(float DeltaSeconds);
	/** Grave dirt and dust round its foot as it rises. */
	void KickDirt(float Strength);
	/** Soul-light rising at Where (the stone, the arena). */
	void SoulLight(const FVector& Where, float Size) const;
	/** The call back's show at the stone and in the arena (the bell, the candles out, soul-light, the boss's answer). */
	void PlaySummonShow(const FVector& ArenaSpot);
	/** The tick's pace: every frame while something moves or the player is near, else a few times a second. */
	void UpdateTickPace();

	// --- BossShrineCallBack.cpp: bringing its boss back ---
	bool CallFromLair();
	bool CallPlaced();
	bool CallAbel(AAbelKeeper& Abel);
	/** Marks its lair's boss out there a called-back fight (its loot's luck), while a call from the lair is on. */
	void MarkLairBosses();
	/** Abel's called-back fight, frame by frame: once he has knelt a moment, his ending without its scene. */
	void TickAbelRematch();
	/** Abel's called-back fight is over: Pa on his board again, the Keeper's story as the story keeps it. */
	void EndAbelRematch(bool bWon);

	/** Its state as last read. */
	BossShrineRules::FVerdict Verdict;

	bool bRisen = false;
	bool bRising = false;
	float RiseTime = 0.f;
	float NextDirt = 0.f;
	bool bCandlesLit = false;
	/** The candles' flare (1 at a flare, falling to 0) and their flicker's clock. */
	float CandleFlare = 0.f;
	float FlickerClock = 0.f;
	/** A summons' flare: the candles go out this long after it (s). */
	float CandlesOutIn = 0.f;
	/** The bell's swing: its strength and how long since it was rung. */
	float BellStrength = 0.f;
	float BellTime = 0.f;
	FQuat BellRest = FQuat::Identity;
	/** Seconds since its boss died this visit (a large number: not this visit). */
	float SinceDefeat = 1.0e6f;
	/** Seconds to its next Refresh. */
	float RefreshIn = 0.f;
	/** Seconds to its boss's answer from the arena after the bell (0: none waiting), and where it answers from. */
	float AnswerIn = 0.f;
	FVector AnswerAt = FVector::ZeroVector;
	/** A call from the lair is on: its boss, once out, is marked a called-back fight. */
	bool bLairCallOn = false;

	/** With no session (a test level), the stone keeps its boss's record itself. */
	FBossRecallRecord LocalRecord;

	// Abel's called-back fight
	TWeakObjectPtr<AAbelKeeper> RematchAbel;
	bool bAbelRematch = false;
	FStoryCondition SavedFightWhen;
	FStoryCondition SavedShownWhen;

	FDelegateHandle BossWonHandle;
	FTimerHandle LoadCheckTimer;
};
