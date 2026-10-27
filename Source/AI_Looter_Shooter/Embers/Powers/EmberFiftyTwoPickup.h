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
 *  - A card that meets a creature's body cuts it for Damage x the level's scale (8 at level 1) and sticks in it, riding
 *    the body, then shrinks away; a creature's cuts in one frame are dealt as one hit (one number, one
 *    Ember.FiftyTwoPickup.Hit), so a point-blank fan reads as a flurry, not a blur of numbers. A card that meets the world
 *    sticks in it edge first a moment, then shrinks away; one that flies its whole range drops away.
 *  - The cards are real cards (the user's note 6, 2026-10-27: "tiny particles ... bigger card like projectiles"):
 *    EmberFX::FCardDeck, an ivory face with a red pip and Odessa's red back, drawn Looter.Embers.Cards.Size (2.2) times a
 *    real card, the face tipped toward the thrower, spinning and fluttering, a faint trail of her colour behind each.
 *  - On use: the deck flung from the left hand (the hands' cast, a fling: FirstPersonHandsMotion; the power fires at its
 *    release), Ember.FiftyTwoPickup.Throw, a small kick of the view.
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
	/** Cards stuck now (in the world or a body), and those in a body. */
	int32 NumStuck() const { return Cards.FilterByPredicate([](const FCard& Card) { return Card.bStuck && !Card.bDone; }).Num(); }
	int32 NumInBodies() const { return Cards.FilterByPredicate([](const FCard& Card) { return Card.bStuck && Card.bOnBody && !Card.bDone; }).Num(); }
	/** Cards the deck drew in the last step (flying and stuck). */
	int32 NumDrawn() const { return Deck.NumDrawn(); }

	static constexpr float DefaultDamage = 8.f;
	static constexpr float DefaultRange = 1400.f;
	static constexpr int32 DefaultCards = 52;
	static constexpr float DefaultConeDegrees = 35.f;
	static constexpr float DefaultFanTall = 12.f;
	/** How fast a card flies (cm/s), give or take a tenth. */
	static constexpr float CardSpeed = 3600.f;
	/** A card's reach round its line (cm): half a card (as it's drawn now, about half its short side). */
	static constexpr float CardRadius = 6.f;

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
		/** Its flutter's phase and rate (radians, radians a second), and its slant in the fan (radians about its way). */
		float Flutter = 0.f;
		float FlutterRate = 10.f;
		float Roll = 0.f;
		/** Stuck in the world or in a body (seconds since), or gone. */
		bool bStuck = false;
		bool bOnBody = false;
		bool bDone = false;
		float StuckAge = 0.f;
		/** Where it sits stuck: in the world, or on the body it cut (StuckIn), relative to that body's actor. */
		FTransform StuckAt = FTransform::Identity;
		TWeakObjectPtr<ACreatureBase> StuckIn;
	};

	/** Sticks Card at At (the point it struck, flying along Way), edge first; on Body if it's one. */
	void StickCard(FCard& Card, const FVector& At, const FVector& Way, ACreatureBase* Struck) const;
	/** Draws a stuck card (or lets it go: its body gone or its time up). */
	void StepStuck(FCard& Card, float DeltaSeconds);

	TArray<FCard> Cards;
	EmberFX::FCardDeck Deck;
	TArray<TWeakObjectPtr<ACreatureBase>> Candidates;
	float Damage = DefaultDamage;
	float Range = DefaultRange;
	int32 Cuts = 0;
	EmberFX::FEmberSparks Sparks;
	FRandomStream Random{ 0x52c4d };
};
