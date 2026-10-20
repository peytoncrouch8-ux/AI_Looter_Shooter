#pragma once

#include "CoreMinimal.h"
#include "Embers/Powers/EmberFX.h"
#include "Embers/Powers/EmberPowerBase.h"
#include "EmberDoubleOrNothing.generated.h"

class AWeaponBase;
class UBulletSubsystem;
struct FBulletShot;

/**
 * Double or Nothing, Odessa's Risk power: for its duration (6 s) every pull of the trigger is a coin toss.
 *  - Heads (Odds 0.5; Extras "Odds"): every pellet of the pull deals Strength (2) times its damage; its tracer burns in the
 *    power's green, wider, a green glint at the muzzle and a coin's bright ring (Ember.DoubleOrNothing.Double).
 *  - Tails: nothing: every pellet is thrown wide (MissMinDegrees to MissMaxDegrees off the aim, 5-9) and is a blank
 *    (FBulletShot::bBlank: it can't hurt or show a hit on anything with health, so it misses entirely); a dim tracer, a
 *    dull coin on felt (Ember.DoubleOrNothing.Nothing).
 *  - Odessa never loses twice running (Extras "MissStreak", 1): after that many tails in a row the next pull is heads.
 *    Each toss is even odds, but the house leans her way, so over a streak it pays (about 4 in 3 of her damage).
 *  - On use: a coin flicked up, spinning, and slapped down (Ember.DoubleOrNothing.Start), a green flare at the hand.
 * It changes the player's shots as they're fired (UBulletSubsystem::OnBulletFiring) only while it runs.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UEmberDoubleOrNothing : public UEmberPowerBase
{
	GENERATED_BODY()

public:
	virtual void Activate(const FEmberContext& Context) override;
	virtual void Tick(const FEmberContext& Context, float DeltaSeconds) override;
	virtual void End(const FEmberContext& Context, bool bInterrupted) override;

	/** A shot about to fly (OnBulletFiring; the tests call it): the player's pull tossed, every pellet of it changed alike. */
	void NoteFiring(FBulletShot& Shot);

	/** The toss itself: heads (double) or tails (nothing), with the house's lean after MissStreak tails. */
	bool TossHeads();

	bool IsGambling() const { return bGambling; }
	int32 GetHeads() const { return Heads; }
	int32 GetTails() const { return Tails; }
	/** The latest pull came up heads. */
	bool WasLastHeads() const { return bLastHeads; }
	/** The tosses' dice (the tests fix them). */
	void SetSeed(int32 Seed) { Random.Initialize(Seed); }

	static constexpr float DefaultStrength = 2.f;
	static constexpr float DefaultOdds = 0.5f;
	static constexpr int32 DefaultMissStreak = 1;
	static constexpr float MissMinDegrees = 5.f;
	static constexpr float MissMaxDegrees = 9.f;

protected:
	virtual bool Linger(float DeltaSeconds) override;

private:
	void Bind(bool bOn);
	void ShowStart();

	TWeakObjectPtr<UBulletSubsystem> Bullets;
	FDelegateHandle FiringHandle;

	float Strength = DefaultStrength;
	float Odds = DefaultOdds;
	int32 MissStreak = DefaultMissStreak;
	bool bGambling = false;
	int32 Heads = 0;
	int32 Tails = 0;
	/** Tails in a row right now. */
	int32 TailsRunning = 0;
	bool bLastHeads = true;
	uint64 LastPullFrame = TNumericLimits<uint64>::Max();
	TWeakObjectPtr<AWeaponBase> LastPullWeapon;

	/** The coin flicked up on use: where it is, its speed, its spin. */
	FVector CoinAt = FVector::ZeroVector;
	FVector CoinVelocity = FVector::ZeroVector;
	float CoinAge = -1.f;
	EmberFX::FEmberSparks Sparks;
	FRandomStream Random{ 0x2d0e };
};
