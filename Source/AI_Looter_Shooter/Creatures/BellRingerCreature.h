#pragma once

#include "CoreMinimal.h"
#include "Creatures/BellRingerRules.h"
#include "Creatures/UnpaidCreature.h"
#include "BellRingerCreature.generated.h"

class ABellRingerRing;
class USphereComponent;
class UStaticMeshComponent;

/**
 * The Bell-Ringer (Docs/Polish/BorderlandsComparison.md, item 8): the chapel's old ringer, one of the Unpaid, who still
 * tolls his hand bell to call the dead. The support kind: kill it first. Bare-headed in his Sunday clothes, a bronze hand
 * bell in his right fist (SM_UnpaidBell) and the chapel bell's broken pull rope trailing from his left (SM_UnpaidBellRope),
 * both on the Unpaid's own rig and posed by code (BellRingerCreaturePose.cpp). 110 health (fragile), 6 damage.
 *
 *  - It hangs back behind its pack (BellRingerCreatureMoves.cpp): it drifts in to about 11 m of its target and holds there,
 *    facing them; come within 7.5 m and it backs off a little, slower than a player walks; run it down to 2.6 m and it
 *    swipes with the bell (its only attack, in place of the Unpaid's lunge).
 *  - Every 7 s (the first 2 s after it turns on a player) it tolls (BellRingerCreatureToll.cpp). The tell is clear: the
 *    bell raised high beside its head and shaken, a halo of light gathering round it, the clapper rattling (1.3 s). Then
 *    the toll, heard across the fight, and a pale ring of light running out over the ground. The toll raises one of the
 *    Unpaid fallen within 18 m in the last 20 s (UBellRingerGraves), rising where it fell (RiseIn) at half health and half
 *    experience, never more than two in a life; with none to raise it steadies every Unpaid within 15 m, itself too, for 6 s
 *    (UBellRingerSteady: a faint aura, 30% faster, attacking sooner).
 *  - The bell is its weak spot (a ball round the bell: always critical). Shot in the tell, the toll is lost: it reels, the
 *    crit staggers it, and its next toll waits a whole interval.
 *  - Its rank: a Restless ringer tolls sooner; a Gravebound one's bell is cracked, a sour toll whose ring slows the player
 *    as its kind's shriek does.
 *  - It has its own lines (CreatureVoiceBarkLines.cpp, its voice's BarkKind) and a page of its own in the Ledger.
 */
UCLASS(Config = Game)
class AI_LOOTER_SHOOTER_API ABellRingerCreature : public AUnpaidCreature
{
	GENERATED_BODY()

public:
	ABellRingerCreature();

	virtual void Tick(float DeltaSeconds) override;

	/** The bell is a weak spot of its own, besides the coal. */
	virtual bool IsCriticalSpot(const FHitResult& Hit) const override;

	// --- The toll (BellRingerCreatureToll.cpp) ---

	/** What it's doing with the bell: nothing (carrying it), the tell, lowering it after a toll, or reeling from a shot. */
	enum class EAct : uint8
	{
		None,
		Tell,
		Lower,
		Reel,
	};
	EAct GetAct() const { return Act; }
	bool IsTelling() const { return Act == EAct::Tell; }

	/** Seconds into what it's doing now. */
	float GetActTime() const { return ActTime; }

	/**
	 * Starts its toll's tell now if it can: alive, after a target, hunting rather than swiping, not phasing and not already
	 * at the bell. Its own frame calls it when a toll is due; the tests call it. True when the tell began.
	 */
	bool BeginToll();

	/** Seconds until it may toll again, and whether one is due now (BeginToll's conditions and its target within reach). */
	float GetTollWait() const { return TollWait; }
	bool WantsToll() const;

	/** What a toll landing now would do: raise one of the fallen (its index in the graves) or steady the pack. */
	EBellToll ChooseTollNow(int32* OutFallen = nullptr) const;

	/** The tolls that landed this life, the dead it raised, and what the last toll did. */
	int32 GetTollCount() const { return TollCount; }
	int32 GetRaisedCount() const { return RaisedCount; }
	EBellToll GetLastToll() const { return LastToll; }

	/** The ones its tolls raised this life (still in the level). */
	TArray<ACreatureBase*> GetRaised() const;

	// --- The bell ---

	UStaticMeshComponent* GetBell() const { return Bell; }
	UStaticMeshComponent* GetRope() const { return Rope; }
	USphereComponent* GetBellHitZone() const { return BellHitZone; }

	/** The middle of the bell's body (world): its hit zone's. */
	FVector GetBellCenter() const;

	/** Whether a hit at Point (world) landed on the bell. */
	bool IsBellHit(const FVector& Point) const;

	/** The bell's swing on its bail now (degrees: forward and to its right), for the tests and Looter.CastShots. */
	FVector2D GetBellSwing() const;

	// --- Hanging back (BellRingerCreatureMoves.cpp) ---

	/** Backing off from a target who came too near. */
	bool IsBackingOff() const { return bBackingOff; }

	/** Its stance against its target now (BellRingerRules::ChooseStance), as its last frame saw it. */
	EBellRingerStance GetStance() const { return Stance; }

	// --- Settings ---

	UPROPERTY(EditAnywhere, Category = "Bell-Ringer")
	FBellRingerTuning Tuning;

	/** The bell (SM_UnpaidBell) and the rope (SM_UnpaidBellRope): their pivots are where the fist holds them. */
	static const TCHAR* const BellPath;
	static const TCHAR* const RopePath;

	/** Its voice's lines (CreatureVoiceBarkLines.cpp's own for it). */
	static const FName BarkKind;

	/** The toll's soul-light (the wave, the burst, the halo) and a raise's. */
	static FLinearColor TollColor();
	static FLinearColor RaiseColor();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnAttackStarted() override;
	virtual void Strike() override;
	virtual bool CanStartAttack() const override;
	virtual float GetAttackStartRange() const override;
	virtual void OnHurt(bool bCritical, const FVector& HitLocation) override;
	virtual void OnDied() override;
	virtual void OnRespawned() override;
	virtual void SetHitVolumesEnabled(bool bEnabled) override;
	virtual void LayerPose(float DeltaSeconds) override;

private:
	// --- The toll (BellRingerCreatureToll.cpp) ---
	/** Counts down to the next toll, starts it when due, and plays out the tell, the lowering and the reel. */
	void TickToll(float DeltaSeconds);
	void SetAct(EAct NewAct);
	/** The toll lands: it raises one of the fallen or steadies the pack, rings out and lights up. */
	void LandToll();
	/** The bell shot in the tell: the toll is lost and it reels. */
	void BreakToll();
	/** The tell or the lowering ends without a toll (it died, or lost its target). */
	void AbandonToll();
	bool RaiseFallen(int32 Index);
	int32 SteadyPack();
	/** Whether the caps leave room for one more of Class (the Unpaid's 12, the 16 near the player). */
	bool HasRoomFor(const UClass* Class) const;
	/** A bullet rang off the bell: the clang, and the bell knocked swinging. */
	void BellStruck(const FVector& HitLocation);

	// --- Hanging back and the swipe (BellRingerCreatureMoves.cpp) ---
	void TickStance(float DeltaSeconds);
	/** The first way back from its target that's clear, on its ground and out of every safe zone; false: cornered. */
	bool FindBackOff(FVector& OutDirection) const;
	void StopBackingOff();
	/** Turns to its target at a constant rate, itself (its walking doesn't turn it while it stands or backs off). */
	void FaceTarget(float DeltaSeconds);
	/** Its walking turns it to where it goes, or not (it keeps its eyes on its target backing off and at the bell). */
	void SetTurnsWithMovement(bool bTurns);
	/** Holds its ground while it's at the bell: no steps, no phase-steps. */
	void HoldStill();
	void SwipeHit();

	// --- The body: the bell's arm, the rope's, the props swinging (BellRingerCreaturePose.cpp) ---
	/** Reads the arms' rest from the rig (their bones and lengths), once it has its model. */
	void SetupArmLayer();
	/** The directions its arms want now (component space), for what it's doing. */
	void WantedArms(FVector& OutUpper, FVector& OutFore, FVector& OutLeftUpper, FVector& OutLeftFore, float& OutWrist) const;
	/** Lays one arm's turns so its upper arm and forearm point along Upper and Fore, Weight over the Unpaid's own. */
	void LayArm(int32 UpperBone, int32 LowerBone, int32 HandBone, const FVector& RestUpper, const FVector& RestFore, const FVector& Upper,
		const FVector& Fore, float Wrist, float Weight);
	/** Hangs the bell and the rope from the fists as the body was posed this frame, swinging on their pendulums. */
	void UpdateProps(float DeltaSeconds);
	/** The bell's hit zone on or off with its body's (off while it's dead or faded away). */
	void ApplyBellHitZone();

	/** One prop hanging from a fist: a damped pendulum (radians forward and to its right, and their speeds). */
	struct FSwing
	{
		FVector2D Angle = FVector2D::ZeroVector;
		FVector2D Speed = FVector2D::ZeroVector;
		FVector LastGrip = FVector::ZeroVector;
		FVector LastVelocity = FVector::ZeroVector;
		bool bStarted = false;
	};
	void StepSwing(FSwing& Swing, const FVector& GripWorld, float Length, float Damping, float DeltaSeconds) const;

	UPROPERTY(VisibleAnywhere, Category = "Bell-Ringer")
	TObjectPtr<UStaticMeshComponent> Bell;

	UPROPERTY(VisibleAnywhere, Category = "Bell-Ringer")
	TObjectPtr<UStaticMeshComponent> Rope;

	UPROPERTY(VisibleAnywhere, Category = "Bell-Ringer")
	TObjectPtr<USphereComponent> BellHitZone;

	// The toll
	EAct Act = EAct::None;
	float ActTime = 0.f;
	float TollWait = 0.f;
	int32 TollCount = 0;
	int32 RaisedCount = 0;
	EBellToll LastToll = EBellToll::None;
	TArray<TWeakObjectPtr<ACreatureBase>> Raised;
	TWeakObjectPtr<ABellRingerRing> Halo;
	/** It was after a player last frame: a fresh hunt waits FirstTollDelay before its first toll. */
	bool bWasHunting = false;
	/** The phase-step's cooldown as set, kept while it's held off at the bell. */
	float HeldPhaseCooldown = -1.f;

	// Hanging back
	EBellRingerStance Stance = EBellRingerStance::Hold;
	bool bBackingOff = false;
	float BackOffTime = 0.f;
	float SinceBackOff = 1000.f;
	bool bTurnsWithMovement = true;

	// The bell's hit zone: wanted (alive, its hit zones on) and on now
	bool bBellZoneWanted = true;
	bool bBellZoneOn = true;

	// The arms' layer
	bool bArmLayerReady = false;
	int32 RightUpper = INDEX_NONE;
	int32 RightLower = INDEX_NONE;
	int32 RightHand = INDEX_NONE;
	int32 RightKnuckle = INDEX_NONE;
	int32 LeftUpper = INDEX_NONE;
	int32 LeftLower = INDEX_NONE;
	int32 LeftHand = INDEX_NONE;
	int32 LeftKnuckle = INDEX_NONE;
	FVector RestRightUpper = FVector::DownVector;
	FVector RestRightFore = FVector::DownVector;
	FVector RestLeftUpper = FVector::DownVector;
	FVector RestLeftFore = FVector::DownVector;
	/** Where each arm points now (component space), eased toward what it wants; the layer's weight (out as it dies). */
	FVector NowRightUpper = FVector::DownVector;
	FVector NowRightFore = FVector::ForwardVector;
	FVector NowLeftUpper = FVector::DownVector;
	FVector NowLeftFore = FVector::DownVector;
	float LayerWeight = 1.f;
	bool bArmsStarted = false;
	float RingerClock = 0.f;

	// The props
	FSwing BellSwing;
	FSwing RopeSwing;
};
