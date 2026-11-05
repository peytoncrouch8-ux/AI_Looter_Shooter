#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Bosses/AbelCorruptionRules.h"
#include "Story/StoryLine.h"
#include "AbelCorruption.generated.h"

class AAbelKeeper;
class AActor;
class AController;
class AKeeperLanternPost;
class APawn;
class UAudioComponent;
class UBossComponent;
class UCapsuleComponent;
class UExponentialHeightFogComponent;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class USphereComponent;
class UStaticMeshComponent;
class UStoryLineSet;

/**
 * What the furnace did to Abel (Main 7 "Given to the Wind", MAINS_2_7.md section 11): his smog, the call-backs that burn it
 * off him a third at a time, the smog tendrils, the drink lines to the six dead on the boards and the fog surge. A part of
 * AAbelKeeper (his default subobject), driven by his own frame (Advance, from his Tick: the tests tick him by hand) and by
 * his fight's moments (his phases' CallBack and Tendril events, his fight starting, starting over and won). Its rules are
 * AbelCorruptionRules'. Split by topic: AbelCorruption.cpp (life, hits, his smog-black look), AbelCorruptionCallBack.cpp
 * (the lock, the kneel, the pips, the last call), AbelCorruptionTendril.cpp (the tendrils and the lines' look),
 * AbelCorruptionDrink.cpp (the six dead) and AbelCorruptionSurge.cpp (the fog surge).
 *
 *  - The call-back: at 66% and 33% his Smog bar locks (his boss untargetable without leaving the fight: hits only spark) and
 *    he goes down on his knees; a 1.5 s hold on the keeper's post "Call him back" fills a gold pip under his bar, boils a
 *    third of the smog off him for good (his hat and coat get their colour back), and he says something of himself. Then the
 *    phase goes on: out into the fog to drink ("The Hose"), or the fog surge and the Gravewind ("Keeper"). Missed, he rises
 *    with his bar still locked and kneels again 12 s later. In a rematch (the smog's echo of him, from his vigil stone) there
 *    is no Abel to call back: no lock, no pips.
 *  - The last call: at zero, knelt, a 3 s hold on the keeper's post while a tendril drains the player; the black lantern
 *    cracks and falls into the canyon; then the scene (Scenes/FreeAbel).
 *  - "Abel.SixSaved" (event and story flag) once the drinking is over with none of the six drunk ("Keeper" begun, or the
 *    fight won): Main 7's optional.
 */
UCLASS(ClassGroup = (Looter))
class AI_LOOTER_SHOOTER_API UAbelCorruptionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAbelCorruptionComponent();

	/** Moves it all on by DeltaSeconds (AAbelKeeper::Tick calls it while he's in the world; the tests call it through him). */
	void Advance(float DeltaSeconds);

	// --- His fight's moments (AAbelKeeper forwards them) ---
	void HandleFightStarted();
	void HandleFightReset();
	void HandleFightWon();
	/** A staggering hit (his boss's stagger): his tendrils let go. */
	void HandleStaggered();
	/** He's out of the world (the story, his ending): nothing of his drawn, nothing held. */
	void HandleHidden();

	// --- The call-back (AbelCorruptionCallBack.cpp) ---

	/** His phase's call-back event: his bar locks and he kneels (a second one owed waits for the first). */
	void BeginCallBack();
	/** His bar is locked for a call-back (not the fog's spell). */
	bool IsLocked() const { return bLocked; }
	/** Knelt for a call-back now (the keeper's post's hold is open). */
	bool IsKneeling() const { return bKneeling; }
	/** The pips filled (0-3) and how much smog is left on him (1-0; the scene eases the last third off). */
	int32 GetPips() const { return Pips; }
	float GetCorruption() const { return CorruptionShown; }
	/** Sets the pips at once, his look with them (the console). */
	void SetPips(int32 InPips);
	/** The scene's hold on how much smog shows (0-1), whatever the pips say. */
	void SetCorruptionShown(float Share);

	/** A hold on the keeper's post finished (the post's OnCalledBack): a pip, or the last call's end. */
	void HandleCalledBack(AKeeperLanternPost& Post);

	/** At zero, knelt: the last call at the keeper's post (a 3 s hold) under a tendril's drain. Once a fight. */
	void BeginFinalCall();
	bool IsFinalCallOpen() const { return bFinalCall; }
	/** The last call is done: the scene may play (AAbelKeeper::PlayFreeScene). */
	bool IsFreed() const { return bFreed; }
	/** Freed for good, as the scene leaves him (or its end state). */
	void SetFreed();

	// --- The tendrils (AbelCorruptionTendril.cpp) ---

	/** The tendril event: one (bTwo: a second just after) at the player, after the black lantern's one-second flare. */
	bool ThrowTendrils(bool bTwo);
	/** Tendrils out (telling, flying, latched) and latched now. */
	int32 NumTendrilsOut() const;
	int32 NumTendrilsLatched() const;
	/** The black lantern is flaring now (its window to be shot: a tell, or a latched tendril's flare). */
	bool IsLanternFlaring() const;
	/** A shot on the black lantern (its hit sphere, during a flare): its tendrils let go (the tests and the hit's handler). */
	bool ShootLantern();
	/** Every tendril lets go (quiet: no snap, no sound). */
	void BreakTendrils(AbelCorruptionRules::EBreak Why, bool bQuiet = false);
	/** All the drain his tendrils have dealt (health, before the ward): counted, as a test level deals no damage to a pawn. */
	float GetTendrilDrainDealt() const { return TendrilDrainDealt; }

	// --- The six dead (AbelCorruptionDrink.cpp) ---

	/** Where the six shrouded dead lie on the boards (world, a body's middle on its bier): build_area_deck.py sets them. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abel|Dead")
	TArray<FVector> BoardsDead;

	/** The body being drunk now (its index in BoardsDead), or INDEX_NONE; how long, and the hits on its collar so far. */
	int32 GetDrinking() const { return Drinking; }
	float GetDrinkTime() const { return DrinkTime; }
	int32 GetCollarHits() const { return CollarHits; }
	int32 NumDrunk() const;
	bool IsDrunk(int32 Index) const { return Drunk.IsValidIndex(Index) && Drunk[Index]; }
	/** A line to body Index now, if he's out in the fog and it isn't drunk (the console, the tests). */
	bool StartDrink(int32 Index);
	/** A hit on the collar of the line drinking now (its capsule): the third breaks it. True when one landed. */
	bool HitCollar();
	/** The line lets go of its body (broken, or he left the fog). */
	void StopDrink(bool bBroken);
	/** Where body Index lies (world), or zero. */
	FVector GetDeadSpot(int32 Index) const;
	/** The body his "grief" turns him to: the nearest to From of those not drunk (the keeper going to walk it); false without. */
	bool GetDeadToWalk(const FVector& From, FVector& OutBody) const;

	// --- The fog surge (AbelCorruptionSurge.cpp) ---

	/** The smog from the wreck up over the deck ("Keeper"), or given back. */
	void SetSurge(bool bOn);
	bool IsSurging() const { return bSurgeWanted; }
	float GetSurgeShare() const { return SurgeShare; }

	// --- His words ---

	/** A mutter of his as he turns to the six (DA_Lines_AbelMutter, one line a time, in turn). */
	void Mutter();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	AAbelKeeper* GetAbel() const;
	UBossComponent* GetBoss() const;
	APawn* GetPlayer() const;
	bool IsRematch() const;

	// --- AbelCorruption.cpp ---
	UFUNCTION()
	void HandleDamaged(float Damage, bool bCritical, FVector HitLocation, AController* InstigatedBy, AActor* DamageCauser);
	/** His look: the smog-black on his body, hat and props (their MIDs' Tint), his coal's furnace orange, his lantern black. */
	void MakeLook();
	void ApplyLook(bool bForce);
	/** Smog streaming off him (pooled puffs off his bones), as much as is left on him. */
	void EmitSmog(float DeltaSeconds);
	/** Says a set's lines (Fallback when the set isn't made); one of them only with LineIndex 0+. */
	void Say(FName SetName, const TArray<FStoryLine>& Fallback, int32 LineIndex = INDEX_NONE, bool bInterrupt = false);
	/** Loads the line sets Say is asked for as the level begins, and holds them (HeldLines). */
	void HoldLines();
	/** Where the shots and lines on him are drawn: the effects' and the hit spheres' components, made once. */
	void MakeParts();

	// --- AbelCorruptionCallBack.cpp ---
	void StartLock();
	void BeginKneel();
	void EndKneel(bool bCalled);
	/** What the phase does once its call-back is done: the drift into the fog, or the surge and the wind. */
	void FinishCallBack(int32 Index);
	void TickCallBack(float DeltaSeconds);
	void TickFinalCall(float DeltaSeconds);
	AKeeperLanternPost* BindKeepersPost();
	void UpdatePips();
	/** The pip's burn: the smog torn off him, the gold, the sound, the shake. */
	void PlayPipBurst(int32 Pip) const;
	void DropBlackLantern();
	void TickFallingLantern(float DeltaSeconds);
	void SendSixSaved();

	// --- AbelCorruptionTendril.cpp ---
	void TickTendrils(float DeltaSeconds);
	void DrawLines(float DeltaSeconds);

	// --- AbelCorruptionDrink.cpp ---
	void TickDrink(float DeltaSeconds);
	/** The body Index drunk out: he heals, it rises Restless, its shroud hides. */
	void DrinkOut(int32 Index);
	/** Every body back on its bier (a reset). */
	void RestoreDead();
	void FindShrouds();

	// --- AbelCorruptionSurge.cpp ---
	void TickSurge(float DeltaSeconds);
	UExponentialHeightFogComponent* FindFog();

	/** One smog tendril. */
	struct FTendril
	{
		enum class EState : uint8 { Off, Tell, Flying, Latched, Snapping };
		EState State = EState::Off;
		TWeakObjectPtr<APawn> Victim;
		FVector Tip = FVector::ZeroVector;
		FVector Aim = FVector::ZeroVector;
		FVector SnapFrom = FVector::ZeroVector;
		float Clock = 0.f;
		float Flown = 0.f;
		float LatchedFor = 0.f;
		float DrainOwed = 0.f;
		float DrainClock = 0.f;
		float TouchClock = 0.f;
		float SnapShare = 0.f;
		/** It waits this long before its tell (the second of two). */
		float Delay = 0.f;
		bool bFinal = false;
		TWeakObjectPtr<UAudioComponent> DrainSound;
	};
	void StepTendril(FTendril& Tendril, float DeltaSeconds);
	void LatchTendril(FTendril& Tendril);
	void LetGo(FTendril& Tendril, AbelCorruptionRules::EBreak Why, bool bQuiet);
	/** Throws the last call's tendril at the player (it holds through the hold). */
	void ThrowFinalTendril();

	// The call-back
	bool bLocked = false;
	bool bKneeling = false;
	float KneelLeft = 0.f;
	float RetryIn = 0.f;
	int32 LocksOwed = 0;
	int32 Pips = 0;
	bool bHintSaid = false;
	bool bFinalCall = false;
	bool bFreed = false;
	bool bSixSent = false;
	float CorruptionShown = 1.f;
	bool bSceneHoldsLook = false;
	TWeakObjectPtr<AKeeperLanternPost> BoundPost;
	/** Who his fight was against (at its start): the last call comes after his boss's fight has let go of its player. */
	TWeakObjectPtr<APawn> FoughtPlayer;
	/** The black lantern falling into the canyon once he's freed: where it is, how fast, how long. */
	TWeakObjectPtr<UStaticMeshComponent> FallingLantern;
	FVector FallVelocity = FVector::ZeroVector;
	float FallClock = 0.f;

	// The tendrils
	TArray<FTendril> Tendrils;
	float LineClock = 0.f;
	bool bLanternShotThisFrame = false;
	float TendrilDrainDealt = 0.f;

	// The six dead
	TArray<bool> Drunk;
	TArray<TWeakObjectPtr<AActor>> Shrouds;
	bool bShroudsFound = false;
	int32 Drinking = INDEX_NONE;
	int32 LastDrunkTried = INDEX_NONE;
	float DrinkTime = 0.f;
	float DrinkRestLeft = 0.f;
	int32 CollarHits = 0;
	bool bDrinkLineSaid = false;
	TWeakObjectPtr<UAudioComponent> DrinkSound;

	// The surge
	bool bSurgeWanted = false;
	float SurgeShare = 0.f;
	bool bFogSaved = false;
	float OwnFogDensity = 0.f;
	float OwnFogStart = 0.f;
	TWeakObjectPtr<UExponentialHeightFogComponent> Fog;
	bool bSurgeLineSaid = false;

	// The look
	float SmogClock = 0.f;
	int32 MutterIndex = 0;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> LookMaterials;
	TArray<FLinearColor> OwnTints;
	TArray<bool> LookIsLantern;
	float ShownLook = -1.f;
	/** His look's materials are made on his first frame (after the Unpaid's own BeginPlay has put on his clothes). */
	bool bLookMade = false;

	/** The tendrils' and the drink line's hoses: engine cylinders on one instanced mesh (one draw). */
	UPROPERTY(Transient)
	TObjectPtr<UInstancedStaticMeshComponent> Lines;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> LineLook;
	/** What a shot meets: the drink line's collar on its body, and the black lantern while it flares (Weapon channel only). */
	UPROPERTY(Transient)
	TObjectPtr<UCapsuleComponent> CollarHit;
	UPROPERTY(Transient)
	TObjectPtr<USphereComponent> LanternHit;

	/**
	 * His line sets, loaded as the level begins and held for the play: loaded at his first line, one cost the fight a 7 ms
	 * load (the frame 16 ms, Insights 2026-11-05), and again after each garbage collection let it go.
	 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UStoryLineSet>> HeldLines;
};
