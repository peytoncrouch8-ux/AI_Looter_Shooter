#pragma once

#include "CoreMinimal.h"
#include "Creatures/DredgerCreature.h"
#include "Creatures/DredgerRules.h"
#include "DredgerRifleman.generated.h"

class UWeaponModelComponent;

/**
 * A Dredger rifleman (EDredgerRole::Rifleman; the contract's numbers: 140 HP, 7 a bullet, a burst of 3 every 2.2 s, 70%
 * accurate at 15 m). A ranged fighter on the fight director's ranged turns (EFightRole::Ranged): he keeps 10-25 m, finds
 * cover by traces round his target (a spot where the world stands between him and them, crouched, and his rifle clears it
 * standing), spread from the other riflemen so pairs flank from both sides, crouches there and rises to fire. Each burst has
 * its tell (he rises and shoulders, his lenses flare, a gold glint at the muzzle), then three real rounds the player sees
 * coming (UEnemyProjectileSubsystem's pellets with a tracer, hits by flight, stopped by cover, misses cracking past), each
 * with its flash, its sound and its kick. Three bursts empty his magazine: he ducks and reloads (MM_Rifle_Reload, his call,
 * the magazine out and in), the player's moment. Too close, he clubs with the butt; staggered, his burst is cut short.
 * Topics: DredgerRifleman.cpp (his kit, turns and attacks), DredgerRiflemanCover.cpp (distance and cover),
 * DredgerRiflemanFire.cpp (rounds, tracers, reload, the butt).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ADredgerRifleman : public ADredgerCreature
{
	GENERATED_BODY()

public:
	ADredgerRifleman();

	// --- Its place in a fight ---
	virtual EFightRole GetFightRole() const override { return EFightRole::Ranged; }
	virtual EFightRole GetAttackTokenRole() const override;
	virtual bool GetFightBand(float& OutNear, float& OutFar) const override;

	/** His company rifle (the project's AR from its parts until the Tripo rifle: DredgerLook::RifleDefinitionPath). */
	UWeaponModelComponent* GetRifle() const { return Rifle; }

	// --- For the tests, the dev command and the HUD ---
	EDredgerTactic GetTactic() const { return Tactic; }
	/** The spot he chose (world, his feet there) and whether it covers him; false with none. */
	bool GetSpot(FVector& OutSpot, bool& bOutCovered) const;
	int32 GetRoundsLeft() const { return Rounds; }
	bool IsReloading() const { return ReloadLeft > 0.f; }
	int32 GetShotsFired() const { return ShotsFired; }
	/** Runs a whole cover search now, every candidate at once (the tests; play spreads it over frames). True if a spot was found. */
	bool DevFindSpot();
	/** Empties his magazine (the next burst's end starts a reload) or starts one now. */
	void DevStartReload();

protected:
	virtual void BeginPlay() override;
	virtual void TickJob(float DeltaSeconds) override;
	virtual void OnAttackStarted() override;
	virtual void Strike() override;
	virtual void OnAttackCalledOff() override;
	virtual void OnTurnPlayedOut() override;
	virtual float GetAttackStartRange() const override;
	virtual bool CanStartAttack() const override;
	virtual bool CanReachHeight(float Rise) const override { return true; }
	virtual void OnHurt(bool bCritical, const FVector& HitLocation) override;
	virtual void OnDied() override;
	virtual void OnRespawned() override;

	virtual FVector GetHeldGrip() const override;
	virtual FVector GetHeldForegrip() const override;
	virtual FVector GetHeldMuzzle() const override;
	virtual float GetHeldScale() const override;
	virtual bool FightsFromPost() const override { return true; }

private:
	// --- Distance and cover (DredgerRiflemanCover.cpp) ---
	void TickTactics(float DeltaSeconds);
	/** Starts a cover search round its target (its squad's flank bearing first); StepSearch measures a few spots a frame. */
	void BeginSearch();
	void StepSearch(int32 Budget);
	/** Measures one candidate spot (ground, cover, a clear shot) into its facts; false when it has no ground he can stand on. */
	bool MeasureSpot(const FVector& Prey, const FVector& Ground, FDredgerSpotFacts& OutFacts) const;
	bool FindGroundAt(const FVector& Where, FVector& OutGround) const;
	/** The bearing (degrees, from its target) his squad wants him on: his flank among the riflemen after the same target. */
	float SquadBearing(const APawn& Prey) const;
	/** Whether another rifleman has a spot near Where. */
	bool IsClaimed(const FVector& Where) const;
	void SetKeepingDistance(bool bKeeping);
	FVector FirstClearWay(TConstArrayView<FVector> Ways) const;
	/** Whether his standing muzzle has a clear line to his target now (looked at a few times a second). */
	bool IsShotClear() const;
	bool WantsButt() const;

	// --- Rounds, tracers, the reload, the butt (DredgerRiflemanFire.cpp) ---
	void FireRound();
	void TickBurst(float DeltaSeconds);
	void TickTracers(float DeltaSeconds);
	void TickReload(float DeltaSeconds);
	void StartReload();
	void ButtStrike();

	UPROPERTY(VisibleAnywhere, Category = "Dredger")
	TObjectPtr<UWeaponModelComponent> Rifle;

	/** The rifle assembled (else a stand-in box's points). */
	bool bRifleBuilt = false;

	// Tactics
	EDredgerTactic Tactic = EDredgerTactic::None;
	bool bHasSpot = false;
	bool bSpotCovered = false;
	FVector Spot = FVector::ZeroVector;
	float RecheckIn = 0.f;
	int32 BurstsHere = 0;
	/** Seconds walking to its spot without getting nearer (it gives that spot up). */
	float StuckFor = 0.f;
	float LastGap = 0.f;
	bool bKeepingDistance = false;
	FVector KeepWay = FVector::ZeroVector;
	float KeepLookIn = 0.f;
	/** Hurt in its cover lately: it moves on sooner (s left of the urge). */
	float FlankedFor = 0.f;
	/** Standing in the open: a sidestep now and then (its clock and side). */
	float StrafeClock = 0.f;
	float StrafeSide = 1.f;

	// The search under way: its candidates (world ground spots), how far it has got, the best so far.
	TArray<FVector> Candidates;
	int32 NextCandidate = 0;
	bool bSearching = false;
	float BestCost = 0.f;
	FVector BestSpot = FVector::ZeroVector;
	bool bBestCovered = false;
	float WantBearing = 0.f;
	/** This search moves him on: his own spot counts as taken. */
	bool bAvoidCurrent = false;

	mutable double ShotCheckedAt = -10.0;
	mutable bool bShotClear = false;

	// The burst and the gun
	bool bShotAttack = false;
	bool bButtAttack = false;
	int32 ShotsLeft = 0;
	float BurstClock = 0.f;
	int32 Rounds = 9;
	int32 ShotsFired = 0;
	float ReloadLeft = 0.f;
	float ReloadLength = 1.f;
	FRandomStream Dice;

	/** A round's tracer, drawn each frame along its flight until it lands (where its line met the world, or its range). */
	struct FTracer
	{
		FVector Start = FVector::ZeroVector;
		FVector Way = FVector::ForwardVector;
		float Speed = 7500.f;
		float End = 4000.f;
		float Age = 0.f;
		bool bHitsWorld = false;
		FVector Normal = FVector::UpVector;
	};
	TArray<FTracer> Tracers;
};
