#pragma once

#include "CoreMinimal.h"
#include "Embers/Powers/EmberFX.h"
#include "Embers/Powers/EmberPowerBase.h"
#include "EmberFiftyTwoPickup.generated.h"

class ACreatureBase;

/**
 * Fifty-Two Pickup, Odessa's second Fortune power (instant): she flings the whole deck.
 *  - Cards (52; Extras "Cards") fan out from her hand along the aim, spread evenly over a fan ConeDegrees wide (35) and
 *    FanTall high (12: a fan, wider than it's tall), each flying CardSpeed (with a little give) for up to Range (14 m),
 *    spinning.
 *  - A card that meets a creature's body cuts it for Damage x the level's scale (8 at level 1) and is spent; a creature's
 *    cuts in one frame are dealt as one hit (one number, one Ember.FiftyTwoPickup.Hit), so a point-blank fan reads as a
 *    flurry, not a blur of numbers. A card that meets the world sticks in it a moment, glinting, then fades; one that
 *    flies its whole range drops away.
 *  - On use: the deck sprung from the hand (Ember.FiftyTwoPickup.Throw), a small kick of the view.
 * Instant: the component ends it the moment it's used; the cards fly on its linger (the player's clock).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UEmberFiftyTwoPickup : public UEmberPowerBase
{
	GENERATED_BODY()

public:
	virtual void Activate(const FEmberContext& Context) override;
	virtual void End(const FEmberContext& Context, bool bInterrupted) override;

	/** Moves every card on by DeltaSeconds: cuts, sticks, falls (its linger; the tests call it). */
	void StepCards(float DeltaSeconds);

	/** Cards still in the air. */
	int32 NumFlying() const;
	/** Cards thrown this use, and how many found a body. */
	int32 NumThrown() const { return Cards.Num(); }
	int32 NumCuts() const { return Cuts; }

	static constexpr float DefaultDamage = 8.f;
	static constexpr float DefaultRange = 1400.f;
	static constexpr int32 DefaultCards = 52;
	static constexpr float DefaultConeDegrees = 35.f;
	static constexpr float DefaultFanTall = 12.f;
	/** How fast a card flies (cm/s), give or take a tenth. */
	static constexpr float CardSpeed = 3600.f;
	/** A card's reach round its line (cm): half a card. */
	static constexpr float CardRadius = 5.f;

protected:
	virtual bool Linger(float DeltaSeconds) override;

private:
	struct FCard
	{
		FVector Location = FVector::ZeroVector;
		FVector Velocity = FVector::ZeroVector;
		float Travelled = 0.f;
		float Spin = 0.f;
		float SpinRate = 20.f;
		/** Stuck in the world (seconds since), or cut into a body (gone), or fallen. */
		bool bStuck = false;
		bool bDone = false;
		float StuckAge = 0.f;
		FVector StuckAlong = FVector::ForwardVector;
	};

	TArray<FCard> Cards;
	TArray<TWeakObjectPtr<ACreatureBase>> Candidates;
	float Damage = DefaultDamage;
	float Range = DefaultRange;
	int32 Cuts = 0;
	EmberFX::FEmberSparks Sparks;
	FRandomStream Random{ 0x52c4d };
};
