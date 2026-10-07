#pragma once

#include "CoreMinimal.h"
#include "Creatures/SpiderCreature.h"
#include "GravemotherCreature.generated.h"

class AGroundCrack;

/** The Gravemother's numbers that never change in play (Docs/Areas/RansomsRest.md, "Enemies by rank" and Side 3). */
namespace Gravemother
{
	/** Her size against a brown spider's, as seen: her BodyScale is this over her rank's own size. */
	inline constexpr float Size = 1.8f;

	/** The Legendary rank's size (UCreatureRankSettings' default) her BodyScale is worked out against. */
	inline constexpr float RankSize = 1.4f;

	/** Her brood: spiderlings, a brown spider at this size with this share of its health (60 at level 1), Basic. */
	inline constexpr float SpiderlingSize = 0.45f;
	inline constexpr float SpiderlingHealthShare = 0.2f;

	/** Spiderlings one call brings, and the shares of her health at which she calls, each once a life: 66%, then 33%. */
	inline constexpr int32 BroodSize = 4;
	inline constexpr int32 NumBroodCalls = 2;
	inline constexpr float BroodCallShare(int32 Call) { return Call <= 0 ? 0.66f : 0.33f; }

	/** Her lair's id (AEncounterSpawner::SpawnerId and LegendaryId), and the tag on the den's place (Side 3's "Enter the den"). */
	inline const FName LairId(TEXT("Gravemother"));
	inline const FName DenPlaceTag(TEXT("Place_Den"));
}

/** Where the Gravemother's charge is. */
UENUM(BlueprintType)
enum class EGravemotherCharge : uint8
{
	/** Not charging: she walks and bites as any spider does. */
	None,
	/** The wind-up: she rears and turns to her target, then holds her aim while the ground cracks open along her line. */
	Telegraph,
	/** The dash: straight down the crack at full tilt, running down whoever stands in it. */
	Dash,
	/** Stopped at the crack's end with her forelegs slammed down: a moment before she can bite again. */
	Recover,
};

/**
 * The Gravemother (Docs/Areas/RansomsRest.md, Side 3 and "Enemies by rank"): Ransom's Rest's Legendary monster, a brown
 * spider grown huge in the Sink's den on what the Unpaid leave behind. SK_Spider and its rig at Gravemother::Size (1.8x)
 * a spider's size, in a pale hide (MI_SpiderBody_Pale once it's made, else the spider's own), Legendary: her name in
 * orange on her tag (no "Soulfed"), twelve times a spider's health (about 5,600 at level 8). Her head and abdomen are
 * critical (their hit hulls; no new bones). She walks and bites as a spider does, never climbing or leaping, and two
 * moves are her own:
 *  - The charge (GravemotherCreatureCharge.cpp): with her target 5 to 16 m off, her way clear and ChargeCooldown passed,
 *    she rears and tracks them for ChargeAimSeconds, then holds her aim while the ground cracks open along her line
 *    (AGroundCrack) through the rest of a long ChargeTelegraph, and dashes straight down it, never off a drop or through
 *    a wall. It runs down whoever is in it (ChargeStrength times a bite's damage and shove), and at the crack's end she
 *    slams down, the ground bursts, and she's slow to rise. Step out of the crack's line before she comes.
 *  - Her brood (GravemotherCreatureBrood.cpp): at 66% and 33% of her health, each once a life, she calls four
 *    spiderlings, which claw up out of the ground round her on her own level and hunting ground and come for her target.
 *    A spiderling is a configuration of the brown spider (SpawnSpiderling), not a class of its own, so its Ledger page is
 *    the spider's.
 * Her lair, an AEncounterSpawner with a LegendaryId (Tools/Unreal/build_area_den.py), brings her, and brings her back
 * only on an arrival 20 minutes of play after her last death (USessionSubsystem::IsLegendaryBack). Hurt, she calls
 * every spider within 30 m onto her attacker (her rank's pack call; spiders share the Spider pack tag).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AGravemotherCreature : public ASpiderCreature
{
	GENERATED_BODY()

public:
	AGravemotherCreature();

	virtual void Tick(float DeltaSeconds) override;

	// --- Her brood (GravemotherCreatureBrood.cpp) ---

	/**
	 * How a spiderling starts: a Basic brown spider at Gravemother::SpiderlingSize with its share of health, at Level (0:
	 * its area's band).
	 */
	static FRuntimeSpawn MakeSpiderlingSpawn(int32 Level = 0);

	/**
	 * Spawns a spiderling standing on Feet, facing Yaw: gone for good once killed, named for what it is on its tag, and
	 * begun even in a level that isn't playing (a test's). Null when it couldn't spawn.
	 */
	static ASpiderCreature* SpawnSpiderling(UWorld* World, const FVector& Feet, float Yaw, int32 Level = 0);

	/** What a spiderling's tag calls it. */
	static FText SpiderlingName();

	/** Calls her brood now, as her health does at 66% and 33%: up to BroodSize spiderlings round her. How many came. */
	int32 CallBrood();

	/** Her calls this life (two at most). */
	int32 GetBroodCallsMade() const { return BroodCallsMade; }

	/** Her brood alive now. */
	TArray<ASpiderCreature*> GetBrood() const;

	// --- Her charge (GravemotherCreatureCharge.cpp) ---

	EGravemotherCharge GetChargeState() const { return ChargeState; }

	/** Its cooldown is done: the next time her target is far enough off and her way to them clear, she charges. */
	bool IsChargeReady() const { return ChargeCooldownLeft <= 0.f; }

	/** Readies her charge at once (the tests, the console). */
	void ReadyCharge();

	/** She has taken her aim on this charge: its line is fixed and its crack open. */
	bool HasTakenAim() const { return bAimTaken; }

	/** The line this charge runs (world): from where she stood as she took aim, along its direction, this far. */
	FVector GetChargeStart() const { return ChargeStart; }
	FVector GetChargeDirection() const { return ChargeDirection; }
	float GetChargeLength() const { return ChargeLength; }

	/** This charge ran its target down (a charge hits once). */
	bool HasChargeHit() const { return bChargeHit; }

	/** The charge's crack in the ground, while it's hers (it lingers and closes on its own after). */
	AGroundCrack* GetCrack() const { return Crack.Get(); }

	// --- Settings: every distance and speed is at a brown spider's size and grows with hers ---

	/** She charges a target up to this far off (cm), and no nearer than ChargeMinDistance: closer, she bites. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravemother|Charge", meta = (ClampMin = "100", Units = "cm"))
	float ChargeRange = 900.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravemother|Charge", meta = (ClampMin = "0", Units = "cm"))
	float ChargeMinDistance = 300.f;

	/** She runs on this far past where her target stood as she took aim (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravemother|Charge", meta = (ClampMin = "0", Units = "cm"))
	float ChargeOvershoot = 150.f;

	/** How fast the dash runs (cm/s; the constructor scales it with the player's speed). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravemother|Charge", meta = (ClampMin = "100", Units = "cm/s"))
	float ChargeSpeed = 900.f;

	/** The whole wind-up (s): she tracks her target for ChargeAimSeconds of it, then holds her aim while the crack runs out. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravemother|Charge", meta = (ClampMin = "0.2", Units = "s"))
	float ChargeTelegraph = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravemother|Charge", meta = (ClampMin = "0", Units = "s"))
	float ChargeAimSeconds = 0.6f;

	/** How long she stays down after the slam before she can bite again (s). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravemother|Charge", meta = (ClampMin = "0", Units = "s"))
	float ChargeRecover = 1.f;

	/** From one charge's end to the next one's readiness, while she's after someone (s); and from her first sight of them. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravemother|Charge", meta = (ClampMin = "0", Units = "s"))
	float ChargeCooldown = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravemother|Charge", meta = (ClampMin = "0", Units = "s"))
	float ChargeFirstDelay = 2.f;

	/** The dash hits this many times as hard as her bite, damage and shove. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravemother|Charge", meta = (ClampMin = "0"))
	float ChargeStrength = 2.f;

	/** The dash runs down whoever comes within this of her body's edge (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravemother|Charge", meta = (ClampMin = "0", Units = "cm"))
	float ChargeHitMargin = 40.f;

	/** The crack's width at its widest, and how far its burst reaches round the slam (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravemother|Charge", meta = (ClampMin = "1", Units = "cm"))
	float ChargeCrackWidth = 18.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravemother|Charge", meta = (ClampMin = "10", Units = "cm"))
	float ChargeBurstRadius = 170.f;

	/** Her brood comes up within this of her (cm, at a spider's size), on her level (BroodMaxStep cm above or below her feet). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravemother|Brood", meta = (ClampMin = "100", Units = "cm"))
	float BroodRadius = 450.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravemother|Brood", meta = (ClampMin = "10", Units = "cm"))
	float BroodMaxStep = 200.f;

	/** Spiderlings come up at least this far apart, and from her middle (cm, as they are). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravemother|Brood", meta = (ClampMin = "50", Units = "cm"))
	float BroodSpacing = 200.f;

	/** None comes up nearer the player than this (cm): out of the ground round her, not under the player's feet. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravemother|Brood", meta = (ClampMin = "0", Units = "cm"))
	float BroodClearOfPlayer = 300.f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnAttackStarted() override;
	virtual void Strike() override;
	virtual void OnHurt(bool bCritical, const FVector& HitLocation) override;
	virtual void OnDied() override;
	virtual void OnRespawned() override;
	virtual bool CanStartAttack() const override;
	virtual float GetAttackStartRange() const override;
	virtual bool TracksTargetInWindup() const override;

private:
	// --- The charge (GravemotherCreatureCharge.cpp) ---
	/** Her target is in reach of a charge and her way to them is clear, with the charge ready. */
	bool WantsToCharge() const;
	/** Her way to her target is clear for a charge (looked at a few times a second, not every frame). */
	bool IsLaneClear() const;
	/** How far a charge along Direction can run, up to Wanted (cm): short of a wall, a drop or a rise she couldn't walk. */
	float MeasureLane(const FVector& Direction, float Wanted) const;
	void BeginTelegraph();
	/** Fixes the line: toward her target as they stand now, its length measured, its crack laid. */
	void TakeAim();
	void BeginDash();
	void TickCharge(float DeltaSeconds);
	void TickDash(float DeltaSeconds);
	/** Runs down her target if her body passed within reach of them going From To (once a charge). Whether it did now. */
	bool TryChargeHit(const FVector& From, const FVector& To);
	/** The dash is over: she slams down where she is and the ground bursts. */
	void EndDash();
	/** Back to walking and biting, the cooldown started (when there was a charge) and the crack left to close. */
	void FinishCharge();
	/** Her feet: the ground under her middle. */
	FVector GetFeet() const;
	/** Grave dirt thrown up at Where (FWeaponFX), Strength 0 to 1. */
	void KickDirt(const FVector& Where, float Strength);

	// --- The brood (GravemotherCreatureBrood.cpp) ---
	/** Drops the dead from her brood. */
	void PruneBrood();

	// The charge
	EGravemotherCharge ChargeState = EGravemotherCharge::None;
	float ChargeCooldownLeft = 0.f;
	/** Her own clock, which runs as she ticks (the lane's looks are timed by it). */
	float ChargeClock = 0.f;
	mutable float LaneCheckedAt = -1.f;
	mutable bool bLaneClear = false;
	bool bAimTaken = false;
	bool bChargeHit = false;
	FVector ChargeStart = FVector::ZeroVector;
	FVector ChargeDirection = FVector::ForwardVector;
	float ChargeLength = 0.f;
	float DashTime = 0.f;
	float DashSeconds = 0.f;
	FVector DashLast = FVector::ZeroVector;
	float TrailIn = 0.f;
	TWeakObjectPtr<AGroundCrack> Crack;
	/** Her bite's timing, which the charge's takes the place of while she charges. */
	float BiteWindup = 0.55f;
	float BiteRecovery = 0.45f;

	// The brood
	int32 BroodCallsMade = 0;
	TArray<TWeakObjectPtr<ASpiderCreature>> Brood;
};
