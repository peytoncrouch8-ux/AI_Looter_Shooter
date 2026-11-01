#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CompanyTender.generated.h"

class AController;
class ACreatureBase;
class ALamplighterShip;
class UAudioComponent;
class UCapsuleComponent;
class UDamageType;
class UPrimitiveComponent;
class USceneComponent;
class UStaticMeshComponent;

/**
 * Vane Consolidated's boarding tender (Main 1 steps 1-2): a small iron skiff that runs the Lamplighter down from astern as
 * Vane's voice breaks in on her horn, comes alongside, fires its two grapples into her rail, drops a gangplank across for
 * the boarders, and keeps two riflemen on its own deck firing across while the player cuts the lines (they never count for
 * the deck: they're the tender's). When both lines are cut it falls away into the canyon, its bag burning and trailing
 * black smoke, its riflemen with it. The Lamplighter spawns it (ALamplighterShip::CallTender) and tells it what to do; it
 * keeps station on her calm pose (her course with no bob), so while she's grappled it's dead still too and its riflemen
 * stand on a still deck.
 *
 * Stand-in look: SM_Skiff_A_Gang (the gang's black skiff) scaled to ~12 m, the gangplank SM_Skiff_Gangplank, the launchers
 * iron cylinders (one path each in the .cpp; the Tripo company tender replaces the hull).
 *
 * Main 6's escorts (2026-11-01, "The Pilot Light"; CompanyTenderEscort.cpp): spawned by the sortie (SpawnEscort) to keep a
 * station of their own on her calm pose (one standing off while its riflemen fire, one coming alongside to board her, then
 * standing off), and its **gas-bag is a weak spot**: shots on it (BagHit, the Weapon channel only) leak it, then tear it
 * (the bag goes up, she falls burning into the fields below, crashes with a blast and smokes a while). Landfall's tender
 * never takes a shot on its bag: none of that is on unless it's an escort.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ACompanyTender : public AActor
{
	GENERATED_BODY()

public:
	ACompanyTender();

	/** Spawns the tender astern of ForShip, on her TenderSide, and starts its run in. (Not named Ship: the member's name.) */
	static ACompanyTender* SpawnFor(ALamplighterShip& ForShip);

	/** Moves its run in, its station alongside or its fall on: the Lamplighter calls it right after moving herself. */
	void Advance(float DeltaSeconds);

	/** Alongside: its run in is done and it keeps station by her rail. */
	bool IsAlongside() const { return bAlongside; }

	/** Puts it alongside at once (the dev jumps, a skipped approach). */
	void SetAlongsideNow();

	/** The world point launcher Index (0 the bow's, 1 the stern's) fires from. */
	FVector GetLauncher(int32 Index) const;

	/** Lowers its gangplank across to her rail, with a thud. */
	void LowerGangplank();

	/** Puts its two riflemen on its deck (DREDGER's rifleman), alerted to the player. */
	void SpawnRiflemen(int32 Level = 0);

	/** Its riflemen still standing. */
	int32 GetRiflemenAlive() const;

	/** The lines are cut: it falls away into the canyon, burning, and is gone FallSeconds later with its riflemen. */
	void FallAway();
	bool IsFalling() const { return bFalling; }

	/** Seconds its run in takes, and its fall before it's gone. */
	static constexpr float ApproachSeconds = 9.f;
	static constexpr float FallSeconds = 8.f;

	/** Alongside: this far out from her middle and this much higher, so its riflemen look down onto her deck (cm). An
	 *  escort's boarding station is exactly this (its gangplank reaches her rail from here). */
	static constexpr float StationOut = 850.f;
	static constexpr float StationUp = 110.f;

	// --- Main 6's escorts (CompanyTenderEscort.cpp) ---

	/**
	 * Spawns an escort for ForShip: at FromLocal in her calm frame (cm), running in over RunSeconds to StationLocal and
	 * keeping it, on her Side (+1 starboard, -1 port: what its launchers and plank face), its gas-bag good for BagHealth of
	 * damage. Null without a world.
	 */
	static ACompanyTender* SpawnEscort(ALamplighterShip& ForShip, const FVector& FromLocal, const FVector& StationLocal, float InSide,
		float RunSeconds, float BagHealth);

	/** An escort slides from its station to a new one (her calm frame, cm) over Seconds: the boarder pulling off to stand off. */
	void MoveToStation(const FVector& StationLocal, float Seconds);

	/** Raises its gangplank again (an escort pulling away from her rail). */
	void RaiseGangplank();

	bool IsEscort() const { return bEscort; }

	/** Its gas-bag: the share of its damage it has taken (0-1), and torn (it's falling or down). */
	float GetBagShare() const;
	bool IsBagBurst() const { return bBagBurst; }

	/** Tears its bag now as enough shots do (By: who, or nobody): the burst, then the burning fall. False if already, or not an escort. */
	bool BurstBag(AController* By);

	/** An escort that fell has hit the ground: its wreck smokes in the field until it's gone. */
	bool HasCrashed() const { return bCrashed; }

	/** It's out of the fight: falling, crashed, or never was. */
	bool IsDown() const { return bFalling || bCrashed; }

	/** What shots on its gas-bag hit (only the Weapon channel, and only on an escort): round the bag, a little bigger. */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UCapsuleComponent> BagHit;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Hull;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Gangplank;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TArray<TObjectPtr<UStaticMeshComponent>> Launchers;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Its place beside her in her calm frame, Alpha (0-1) through its run in (1: alongside). */
	FTransform StationAt(float Alpha) const;

	/** Its gangplank swung toward her rail by PlankAlpha (both kinds). */
	void UpdatePlank(float DeltaSeconds);

	/** Falling away, burning (both kinds; an escort's ends on the ground: Crash). */
	void AdvanceFall(float DeltaSeconds);

	// --- Escorts (CompanyTenderEscort.cpp) ---
	void AdvanceEscort(float DeltaSeconds);
	/** Its wreck on the ground: the smoke column, then gone. */
	void AdvanceWreck(float DeltaSeconds);
	/** Its riflemen held where they stand on its deck as it flies (they hold their ground; it moves under them). */
	void HoldRiflemen();
	/** It hits the ground at Ground: the blast, the dust, the wreck. */
	void Crash(const FVector& Ground);
	UFUNCTION()
	void HandleBagDamage(AActor* DamagedActor, float Damage, AController* InstigatedBy, FVector HitLocation,
		UPrimitiveComponent* HitComponent, FName BoneName, FVector ShotFromDirection, const UDamageType* DamageType, AActor* DamageCauser);

	bool bEscort = false;
	FVector EscortFrom = FVector::ZeroVector;
	FVector EscortStation = FVector::ZeroVector;
	float EscortRunSeconds = 9.f;
	/** Moving to a new station: from where, over how long, how far through. */
	FVector MoveFrom = FVector::ZeroVector;
	float MoveSeconds = 0.f;
	float MoveClock = -1.f;
	float BagHealthMax = 1.f;
	float BagDamage = 0.f;
	bool bBagBurst = false;
	/** Past half its damage the bag leaks: a jet of gas from its first hole (its own frame), now and then. */
	bool bLeaking = false;
	FVector LeakLocal = FVector::ZeroVector;
	float LeakWait = 0.f;
	bool bCrashed = false;
	float WreckClock = 0.f;
	float WreckSmokeWait = 0.f;
	float GroundWait = 0.f;
	/** Where it keeps station now (her calm frame), its run-in done. */
	FVector EscortNow = FVector::ZeroVector;
	TWeakObjectPtr<UAudioComponent> LeakSound;
	TWeakObjectPtr<UAudioComponent> WreckSound;

	/** +1: on her starboard (+Y) side; -1: port. Hers, read when it's spawned. */
	float Side = 1.f;

	TWeakObjectPtr<ALamplighterShip> Ship;
	float Clock = 0.f;
	bool bAlongside = false;

	/** The gangplank swinging down (0 raised, 1 across), and whether it has been asked to. */
	float PlankAlpha = 0.f;
	bool bPlankLowering = false;

	bool bFalling = false;
	float FallClock = 0.f;
	FVector FallVelocity = FVector::ZeroVector;
	FTransform FallFrom = FTransform::Identity;
	float SmokeWait = 0.f;
	bool bBurst = false;

	/** Its riflemen and where each stands on its deck (tender-local), so they go down with it. */
	TArray<TWeakObjectPtr<ACreatureBase>> Riflemen;
	TArray<FVector> RiflemanSpots;

	TWeakObjectPtr<UAudioComponent> EngineSound;
};
