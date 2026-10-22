#pragma once

#include "CoreMinimal.h"
#include "Creatures/SpiderCreature.h"
#include "Creatures/SpitterRules.h"
#include "SpitterSpider.generated.h"

class ASpitterGlob;
class UFightDirectorSubsystem;
class UMaterialInstanceDynamic;

/** Which of its two attacks the Spitter is at (each is the creature's one attack, wound up, struck and recovered). */
UENUM(BlueprintType)
enum class ESpitterMove : uint8
{
	/** Rearing with its sac swelling and glowing, then a glob lobbed at its target. */
	Spit,
	/** A bite, only when it's caught or cornered. */
	Bite,
};

/** The Spitter's numbers that never change in play. */
namespace SpitterLook
{
	/** The bone of its venom sac (SK_Spitter's, a child of the abdomen): its weak spot, swelling as it tells a spit. */
	inline const FName SacBone(TEXT("sac"));
	/** The sac's material slot (Spitter.py's SpitterSac, on the Gel master), whose InnerGlow the tell drives. */
	inline const FName SacSlot(TEXT("SpitterSac"));
	/** Its venom's color: globs, puddles, the glow at its fangs. */
	inline const FLinearColor Venom(0.45f, 1.f, 0.12f);
}

/**
 * The Venom Spitter (Docs/Polish/BorderlandsComparison.md, item 8: the ranged kind): the brown spider's thinner-legged
 * cousin with a swollen, translucent venom sac on its abdomen, glowing green (SK_Spitter, from Art/Models/Creatures/
 * Spitter.py: the spider's rig, legs and atlas in an olive tint, so ASpiderCreature's gait walks it). 200 health and a 10
 * bite at level 1; the head and the sac are critical. Ranks work as for spiders, with two traits of its own: a Restless one
 * spits a fifth sooner, a Gravebound one throws three globs.
 *
 * It keeps its distance (SpitterSpiderKeepAway.cpp): its brain closes in to CloseInTo and stops (AttackRange is set so
 * the brain's own chase stops there), and inside that it moves itself, always facing its target: it backs away inside
 * BackOffWithin, and in its band it sidesteps round its target in bursts. With no way back it's cornered.
 *
 * Its attacks (SpitterSpiderMoves.cpp): every SpitEvery seconds, from SpitMinDistance to SpitMaxDistance with its lob's
 * way clear, it spits: a clear tell (it rears up, the spider's attack pose; its sac swells and throbs brighter and a glow
 * grows at its fangs, UEnemyProjectileSubsystem::ShowCharge; a hiss), then a glob lobbed high (ASpitterGlob) that hurts
 * whom it comes down on, splashes those near, and leaves a venom puddle (ASpitterPuddle). A shot that crits its sac during
 * the tell chokes the spit: the venom bursts out of the sac and nothing flies. It bites only when caught or cornered.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ASpitterSpider : public ASpiderCreature
{
	GENERATED_BODY()

public:
	ASpitterSpider();

	virtual void Tick(float DeltaSeconds) override;

	/** Which attack it's at (or was at last), and what it's doing about its distance now. */
	ESpitterMove GetMove() const { return Move; }
	ESpitterKeep GetKeep() const { return Keep; }

	/** Its spit is ready (its cooldown done); seconds until it is. */
	bool IsSpitReady() const { return SpitReadyIn <= 0.f; }
	float GetSpitReadyIn() const { return FMath::Max(SpitReadyIn, 0.f); }

	/** Readies its spit at once, or holds it off a while (the tests, the console). */
	void ReadySpit() { SpitReadyIn = 0.f; }
	void HoldSpit(float Seconds) { SpitReadyIn = Seconds; }

	/** In a spit's tell now (before the glob flies), and how far through it (0 to 1, eased: the sac's swell and glow). */
	bool IsTelling() const;
	float GetTellStrength() const;

	/** No way back from a target too close: it stands its ground and bites. */
	bool IsCornered() const { return CorneredFor >= CorneredAfter; }

	/** A spit choked by a shot to its sac in the tell (until its next spit). */
	bool WasSpitChoked() const { return bSpitChoked; }

	/**
	 * Chokes the spit it's telling (a crit on its sac, OnHurt): the venom bursts out of the sac at Where, its tell is cut
	 * short with nothing thrown, it flinches, and its spit starts over from a full cooldown. Nothing outside a tell.
	 */
	void ChokeSpit(const FVector& Where);

	/** Whether a hit at HitLocation struck its sac rather than its head (the nearer of the two bones). */
	bool IsSacHit(const FVector& HitLocation) const;

	/** Globs its spit throws at its rank now (SpitterRules::GlobsFor), and its time between spits now. */
	int32 GetGlobsPerSpit() const { return GlobsPerSpit; }
	float GetSpitInterval() const { return SpitEvery * IntervalShare; }

	/** Its sac's swell (its bone's scale) and glow (the sac's InnerGlow) now. */
	float GetSacScale() const { return SacScale; }
	float GetSacGlow() const { return SacGlow; }

	/** Where its globs leave (world): just ahead of its fangs. */
	FVector GetMuzzle() const;

	/** The globs its last spit threw (while they last). */
	TArray<ASpitterGlob*> GetLastGlobs() const;

	// --- Its place in a fight (UFightDirectorSubsystem; SpitterSpiderKeepAway.cpp) ---

	/** A ranged kind: it holds a spot in its band, off the melee pack's side, and spits only with a ranged turn. */
	virtual EFightRole GetFightRole() const override { return EFightRole::Ranged; }
	/** Its spit takes a ranged turn, its bite (caught or cornered) a melee one. */
	virtual EFightRole GetAttackTokenRole() const override;
	/** Its band: from where it backs off to where its brain stops closing in (CloseInTo). */
	virtual bool GetFightBand(float& OutNear, float& OutFar) const override;
	/** It backs away from a target nearer than this (cm): Looter.Fight.RangedBackOff, or BackOffWithin with that at 0. */
	float GetBackOffDistance() const;

	// --- Settings (cm and seconds; distances at its target, not its size, unless they say so) ---

	/**
	 * It backs away from a target nearer than this, and its brain brings it in to this from farther off. 900 since the fight
	 * contract (2026-10-21; was 1000); in play Looter.Fight.RangedBackOff stands for it while that's above 0 (GetBackOffDistance).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spitter|Range", meta = (ClampMin = "100", Units = "cm"))
	float BackOffWithin = 900.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spitter|Range", meta = (ClampMin = "200", Units = "cm"))
	float CloseInTo = 1600.f;

	/** Its share of its chase speed backing away, and strafing. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spitter|Range", meta = (ClampMin = "0", ClampMax = "1.5"))
	float BackOffSpeedShare = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spitter|Range", meta = (ClampMin = "0", ClampMax = "1.5"))
	float StrafeSpeedShare = 0.4f;

	/** It spits at a target between these (the near one at its size), every SpitEvery seconds (release to release). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spitter|Spit", meta = (ClampMin = "0", Units = "cm"))
	float SpitMinDistance = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spitter|Spit", meta = (ClampMin = "200", Units = "cm"))
	float SpitMaxDistance = 2200.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spitter|Spit", meta = (ClampMin = "1", Units = "s"))
	float SpitEvery = 3.f;

	/** The tell before the glob flies (the time to get behind something or move), and the recovery after. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spitter|Spit", meta = (ClampMin = "0.3", Units = "s"))
	float SpitTell = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spitter|Spit", meta = (ClampMin = "0.1", Units = "s"))
	float SpitRecovery = 0.5f;

	/** It sizes up a target it has just turned on this long before its first spit. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spitter|Spit", meta = (ClampMin = "0", Units = "s"))
	float FirstSpitDelay = 1.2f;

	/** Its lob leads a moving target by this share of the flight (SpitterRules::AimPoint). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spitter|Spit", meta = (ClampMin = "0", ClampMax = "1"))
	float LeadShare = 0.5f;

	/** A glob's size, a square hit's damage and the splash's, as shares of its bite (AttackDamage), and the splash's reach. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spitter|Spit", meta = (ClampMin = "1", Units = "cm"))
	float GlobRadius = 14.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spitter|Spit", meta = (ClampMin = "0"))
	float GlobDamageShare = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spitter|Spit", meta = (ClampMin = "0"))
	float SplashDamageShare = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spitter|Spit", meta = (ClampMin = "0", Units = "cm"))
	float SplashRadius = 160.f;

	/** A Gravebound one's three globs land this far apart, across its line. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spitter|Spit", meta = (ClampMin = "0", Units = "cm"))
	float GlobSpacing = 260.f;

	/** The puddle a glob leaves: its reach, how long it lies, one burn's damage (a share of its bite) and how often it burns. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spitter|Puddle", meta = (ClampMin = "10", Units = "cm"))
	float PuddleRadius = 130.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spitter|Puddle", meta = (ClampMin = "0.5", Units = "s"))
	float PuddleSeconds = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spitter|Puddle", meta = (ClampMin = "0"))
	float PuddleDamageShare = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spitter|Puddle", meta = (ClampMin = "0.1", Units = "s"))
	float PuddleEvery = 0.5f;

	/** It bites a target within BiteFrom (at its size) when cornered, or one within CaughtWithin (it can't back away from that). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spitter|Bite", meta = (ClampMin = "50", Units = "cm"))
	float BiteFrom = 230.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spitter|Bite", meta = (ClampMin = "0", Units = "cm"))
	float CaughtWithin = 260.f;

	/** Seconds from one bite to the next. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spitter|Bite", meta = (ClampMin = "0", Units = "s"))
	float BiteCooldown = 1.2f;

	/** Its sac's glow at rest, and at the height of a tell (the sac's InnerGlow on M_Gel). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spitter|Sac", meta = (ClampMin = "0"))
	float SacRestGlow = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spitter|Sac", meta = (ClampMin = "0"))
	float SacTellGlow = 4.f;

	/** It has had no way back this long (s): cornered. */
	static constexpr float CorneredAfter = 0.4f;

protected:
	virtual void BeginPlay() override;
	virtual void OnAttackStarted() override;
	virtual void Strike() override;
	virtual void OnHurt(bool bCritical, const FVector& HitLocation) override;
	virtual void OnDied() override;
	virtual void OnRespawned() override;
	virtual void OnRankChanged() override;
	virtual float GetAttackStartRange() const override;
	/** Its lob comes down on a player up on a rock or a roof as well as on the flat: never out of its reach for height. */
	virtual bool CanReachHeight(float Rise) const override { return true; }
	/** A spit called off before it flew (hit off its turn): its tell's glow goes out. */
	virtual void OnAttackCalledOff() override;

private:
	// --- Its attacks (SpitterSpiderMoves.cpp) ---
	/** Its spit can start now: ready, its target in range, its lob's way clear. */
	bool WantsToSpit() const;
	/** It bites now: caught or cornered, its target in reach, the bite ready. */
	bool WantsToBite() const;
	/** Whether a lob at its target clears what's between them (looked at a few times a second, not every frame). */
	bool IsLobClear(const APawn& Victim) const;
	void BeginSpit();
	void ReleaseSpit();
	void BeginBite();
	void BiteNow();
	/** Its spit's and bite's clocks, its first sight of a target. */
	void TickMoves(float DeltaSeconds);

	// --- Keeping its distance (SpitterSpiderKeepAway.cpp) ---
	/** What it does about its distance, and moves itself inside its brain's hold. */
	void TickKeepAway(float DeltaSeconds);
	/** In its band with a fight spot (UFightDirectorSubsystem): round its target to Spot, then holding there facing it. */
	void TickToSpot(const APawn& Victim, const FVector& Spot, float DeltaSeconds, UFightDirectorSubsystem& Director);
	/** The first clear way among Candidates (flat directions), staying on its hunting ground; zero when none is. */
	FVector FirstClearWay(TConstArrayView<FVector> Candidates) const;
	/** Its walking movement as its brain's chase wants it (turning to where it walks), or as keeping its distance does (facing its target, never off a ledge). */
	void SetKeepingDistance(bool bKeeping);

	// --- Its sac (SpitterSpider.cpp) ---
	/** Finds its sac (bone and material slot) on its model; nothing on a model without one (SK_Spider before SK_Spitter is imported). */
	void SetupSac();
	/** The sac's swell, glow and pose this frame. */
	void TickSac(float DeltaSeconds);
	/** Venom flung out of the sac at Where, with its splat (a choked spit, its death). */
	void BurstSac(const FVector& Where, float Size);

	// Attacks
	ESpitterMove Move = ESpitterMove::Spit;
	float SpitReadyIn = 0.f;
	float BiteReadyIn = 0.f;
	bool bSpitChoked = false;
	bool bHadTarget = false;
	int32 GlobsPerSpit = 1;
	float IntervalShare = 1.f;
	/** Its bite's timing as its class gave it: a spit borrows the attack's timing and gives it back. */
	float OwnBiteWindup = 0.5f;
	float OwnBiteRecovery = 0.45f;
	mutable float LobCheckedAt = -1.f;
	mutable bool bLobClear = false;
	/** Its own clock (the lob's looks are timed by it). */
	float Clock = 0.f;
	TArray<TWeakObjectPtr<ASpitterGlob>> LastGlobs;

	// Keeping its distance
	ESpitterKeep Keep = ESpitterKeep::Close;
	float CorneredFor = 0.f;
	/** Its strafe's rhythm: sidestepping or pausing for StrafeLeft more seconds, to its StrafeSign (+1 right, -1 left). */
	bool bStrafing = false;
	float StrafeLeft = 0.f;
	float StrafeSign = 1.f;
	/** Its way now, looked for again when KeepLookIn runs out. */
	FVector KeepWay = FVector::ZeroVector;
	float KeepLookIn = 0.f;
	bool bKeepingDistance = false;
	/** Seconds it has stood at its fight spot with its spit ready and its lob blocked (it asks for a new spot after a while). */
	float LobBlockedFor = 0.f;

	// The sac
	int32 SacPoseSlot = INDEX_NONE;
	int32 AbdomenPoseSlot = INDEX_NONE;
	FTransform SacInAbdomen;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> SacLook;
	float SacScale = 1.f;
	float SacGlow = 0.f;
	/** 1 just after a spit (the sac emptied), easing back to 0 as it fills. */
	float SacSpent = 0.f;
	/** 1 at a crit on it, fading fast: a flash. */
	float SacFlash = 0.f;
	float SacClock = 0.f;
};
