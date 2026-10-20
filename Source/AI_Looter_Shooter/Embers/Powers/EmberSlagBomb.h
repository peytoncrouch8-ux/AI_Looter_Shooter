#pragma once

#include "CoreMinimal.h"
#include "Embers/Powers/EmberFX.h"
#include "Embers/Powers/EmberPowerBase.h"
#include "EmberSlagBomb.generated.h"

class ACreatureBase;
class AEmberPropActor;
class UAudioComponent;
class UMaterialInstanceDynamic;

/**
 * Slag Bomb, Gauge's Forge power, lobbed from its firebox (Docs/Handoffs/CloudIslandConcepts_2026-10-05.md, "Ember
 * powers"). Its duration (7 s) is the flight and the pool.
 *  - Where: the creature in its sights (its feet), else where the aim meets the world (the ground under it, for a wall),
 *    else Range (20 m) out along the aim, down onto the ground; nowhere to land and the press is refused.
 *  - The lob: an arc from the firebox to there (FlightSeconds by the distance, its top higher the farther it goes), a
 *    glowing ball trailing sparks and smoke; it bursts early on anything solid in its way.
 *  - The burst: creatures within InnerRadius (2.5 m, to their body's nearest side) take Damage (48 at level 1), within
 *    Radius (4.5 m) OuterDamage (26), x the level's scale, in sight of it (UEmberDamageType). A flash, a ring of fire,
 *    molten globs thrown out, grave dirt, Ember.SlagBomb.Burst.
 *  - The pool: PoolSeconds (6) of slag Radius round the burst: whatever stands in it, or left it under BurnLinger (0.6 s)
 *    ago, burns for BurnDamage (10) every BurnInterval (0.5 s), worn orange while it burns (the hit flash's overlay),
 *    Ember.SlagBomb.Sear on each burn. Its edge glows, hot blobs crust over as it cools, sparks and pale smoke rise; its
 *    bubbling is a loop on it (Ember.SlagBomb.Pool). The player is never burned. Its time running out with the pool still
 *    hot (a late landing), the pool burns out its own time on the linger; cut short, it goes at once. EmberSlagBombLook.cpp
 *    draws the pool.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UEmberSlagBomb : public UEmberPowerBase
{
	GENERATED_BODY()

public:
	virtual EEmberDenied CanActivate(const FEmberContext& Context) const override;
	virtual void Activate(const FEmberContext& Context) override;
	virtual void Tick(const FEmberContext& Context, float DeltaSeconds) override;
	virtual void End(const FEmberContext& Context, bool bInterrupted) override;

	/** Where a lob of up to Range from Character lands now (false: nowhere). */
	static bool FindLanding(const ACharacter& Character, float Range, FVector& OutLanding);

	/** Moves the bomb, then the pool, on by DeltaSeconds (its Tick; the tests call it). */
	void StepBomb(float DeltaSeconds);

	bool IsFlying() const { return bFlying; }
	bool IsPooling() const { return PoolLeft > 0.f; }
	const FVector& GetLanding() const { return Landing; }
	const FVector& GetBurstAt() const { return BurstAt; }
	/** The burns dealt so far this pool. */
	int32 NumBurns() const { return Burns; }

	static constexpr float DefaultDamage = 48.f;
	static constexpr float DefaultRange = 2000.f;
	static constexpr float DefaultRadius = 450.f;
	static constexpr float DefaultInnerRadius = 250.f;
	static constexpr float DefaultOuterDamage = 26.f;
	static constexpr float DefaultBurnDamage = 10.f;
	static constexpr float DefaultBurnInterval = 0.5f;
	static constexpr float DefaultBurnLinger = 0.6f;
	static constexpr float DefaultPoolSeconds = 6.f;
	/** How far over or under the pool a body may stand and still be in it (cm, at size 1). */
	static constexpr float PoolHeight = 160.f;

protected:
	virtual bool Linger(float DeltaSeconds) override;

private:
	void Burst(const FVector& At);
	void BurnTick();
	void EndPool();
	void DrawPool(float DeltaSeconds);

	// The numbers, kept at the use.
	float InnerDamage = DefaultDamage;
	float OuterDamage = DefaultOuterDamage;
	float InnerRadius = DefaultInnerRadius;
	float Radius = DefaultRadius;
	float BurnDamage = DefaultBurnDamage;
	float BurnInterval = DefaultBurnInterval;
	float BurnLinger = DefaultBurnLinger;
	float PoolSeconds = DefaultPoolSeconds;

	// The lob.
	bool bFlying = false;
	FVector From = FVector::ZeroVector;
	FVector Landing = FVector::ZeroVector;
	FVector BombAt = FVector::ZeroVector;
	float Flight = 0.6f;
	float Apex = 150.f;
	float FlightTime = 0.f;

	// The pool.
	FVector BurstAt = FVector::ZeroVector;
	float PoolLeft = 0.f;
	float PoolAge = 0.f;
	float BurnClock = 0.f;
	int32 Burns = 0;
	TArray<FVector> Edge;
	TArray<FVector> Blobs;
	TMap<TWeakObjectPtr<ACreatureBase>, float> LastInPool;
	TWeakObjectPtr<AEmberPropActor> Anchor;
	TWeakObjectPtr<UAudioComponent> Bubbling;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BurnLook;
	EmberFX::FEmberTint Burning;

	// What lingers: the burst's ring of fire.
	float RingAge = -1.f;
	/** The power's time ran out with the pool still hot: it burns out on the linger. */
	bool bPoolOnLinger = false;
	EmberFX::FEmberSparks Sparks;
	float SmokeClock = 0.f;
	FRandomStream Random{ 0x51a6 };
};
