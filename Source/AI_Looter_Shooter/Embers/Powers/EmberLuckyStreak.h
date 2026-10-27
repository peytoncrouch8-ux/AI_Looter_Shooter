#pragma once

#include "CoreMinimal.h"
#include "Embers/Powers/EmberFX.h"
#include "Embers/Powers/EmberPowerBase.h"
#include "EmberLuckyStreak.generated.h"

class ACreatureBase;
class AWeaponBase;
class UBulletSubsystem;
struct FBulletLanding;
struct FBulletShot;
struct FHitResult;

/**
 * Lucky Streak, Odessa's Fortune power (Docs/Handoffs/CloudIslandConcepts_2026-10-05.md, "Ember powers"): for its
 * duration (8 s) her luck rides on her shots.
 *  - Every CritEvery-th pull of the trigger (3; Extras "CritEvery") is a lucky round: critical wherever it lands (the
 *    shot's own critical multiplier, x1.5 or a cursed iron's), its tracer gold and wider, a gold glint and a coin's ring
 *    (Ember.LuckyStreak.Lucky) as it leaves. A shotgun's pellets from one pull are one round.
 *  - Every hit on a living creature sends a ricochet (one per pull) into the nearest other living creature within Range
 *    (12 m) of the hit, in sight of it: a gold streak flying at RicochetSpeed, Damage x the level's scale (14 at level 1;
 *    UEmberDamageType, no gun's notch) where it lands, a card flicked off the hit and a bright glance
 *    (Ember.LuckyStreak.Ricochet). Ricochets don't ricochet.
 *  - Seen and heard on use: a fan of cards bursting from the hand, gold glints, a deck's riffle and a coin
 *    (Ember.LuckyStreak.Start).
 * It listens to the bullets (UBulletSubsystem::OnBulletFiring, OnBulletLanding) only while it runs; ricochets already
 * flying land after it ends (its linger).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UEmberLuckyStreak : public UEmberPowerBase
{
	GENERATED_BODY()

public:
	virtual void Activate(const FEmberContext& Context) override;
	virtual void Tick(const FEmberContext& Context, float DeltaSeconds) override;
	virtual void End(const FEmberContext& Context, bool bInterrupted) override;

	/** A shot about to fly (the bullets' OnBulletFiring; the tests call it): the player's every CritEvery-th pull is lucky. */
	void NoteFiring(FBulletShot& Shot);
	/** A shot landing (OnBulletLanding; the tests call it): the player's hit on a living creature sends a ricochet. */
	void NoteLanding(const FBulletShot& Shot, const FHitResult& Hit, FBulletLanding& Landing);
	/** Moves the ricochets and the cards on (its Tick and linger; the tests call it). */
	void StepFlight(float DeltaSeconds);

	bool IsStreaking() const { return bStreaking; }
	/** Trigger pulls counted this streak, and whether the latest one was lucky. */
	int32 GetPulls() const { return Pulls; }
	bool WasLastPullLucky() const { return bLuckyPull; }
	int32 NumRicochetsFlying() const { return Ricochets.Num(); }
	/** The creature the latest ricochet was sent at (null before the first). */
	ACreatureBase* GetLastRicochetTarget() const { return LastTarget.Get(); }

	static constexpr float DefaultDamage = 14.f;
	static constexpr float DefaultRange = 1200.f;
	static constexpr int32 DefaultCritEvery = 3;
	/** How fast a ricochet flies (cm/s): quick, but seen crossing. */
	static constexpr float RicochetSpeed = 6500.f;

protected:
	virtual bool Linger(float DeltaSeconds) override;

private:
	void Bind(bool bOn);
	void SendRicochet(ACreatureBase& From, const FVector& At);
	void LandRicochet(ACreatureBase& Target, const FVector& At, const FVector& Along);
	void FlickCard(const FVector& At, const FVector& Away, float Speed);
	void ShowStart();

	struct FRicochet
	{
		TWeakObjectPtr<ACreatureBase> Target;
		FVector From = FVector::ZeroVector;
		float Age = 0.f;
		float Flight = 0.1f;
	};
	struct FCard
	{
		FVector Location = FVector::ZeroVector;
		FVector Velocity = FVector::ZeroVector;
		float Spin = 0.f;
		float SpinRate = 10.f;
		/** Its flutter's phase and rate (radians, radians a second). */
		float Flutter = 0.f;
		float FlutterRate = 9.f;
		float Age = 0.f;
		float Life = 0.6f;
	};
	/** The cards drawn as real cards (EmberFX::FCardDeck; the user's note 6, 2026-10-27). */
	EmberFX::FCardDeck Deck;

	TWeakObjectPtr<UBulletSubsystem> Bullets;
	FDelegateHandle FiringHandle;
	FDelegateHandle LandingHandle;

	float Damage = DefaultDamage;
	float Range = DefaultRange;
	int32 CritEvery = DefaultCritEvery;
	bool bStreaking = false;

	int32 Pulls = 0;
	bool bLuckyPull = false;
	uint64 LastPullFrame = TNumericLimits<uint64>::Max();
	TWeakObjectPtr<AWeaponBase> LastPullWeapon;
	/** One ricochet per pull: the frame the last one was sent in. */
	uint64 LastRicochetFrame = TNumericLimits<uint64>::Max();

	TArray<FRicochet> Ricochets;
	TArray<FCard> Cards;
	EmberFX::FEmberSparks Sparks;
	TWeakObjectPtr<ACreatureBase> LastTarget;
	FRandomStream Random{ 0x1c4d7 };
};
