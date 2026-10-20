#pragma once

#include "CoreMinimal.h"
#include "Embers/Powers/EmberFX.h"
#include "Embers/Powers/EmberPowerBase.h"
#include "EmberTollChain.generated.h"

class ACreatureBase;

/**
 * Toll Chain, Crane's Debt power: the chain from his shoulder hooks the creature in his sights and hauls it to his feet.
 * Its duration (0.5 s) is the throw and the haul, run on the power's Tick (the component's, the player's clock).
 *  - The mark: the living creature nearest the crosshair within Range (18 m) and a few degrees of the aim, in sight
 *    (UEmberPowerBase::FindInSights); none and the press is refused (the socket shakes).
 *  - The throw (ThrowSeconds): the chain's head flies from the hand to its chest, the links paying out behind it.
 *  - The hook: Damage x the level's scale (20 at level 1; UEmberDamageType, no gun's notch), sparks and the hook's bite
 *    (Ember.TollChain.Hook), and it reels: dazed for the haul and Stun (1.5 s) after it (UCreatureKnockComponent::Daze, so
 *    its brain waits and the gold sparks circle its head).
 *  - The haul: the rest of the duration, it's dragged along the ground (each step's ground found under it; stopped short by
 *    anything solid) to a stride in front of him, eased in and out, the chain drawn taut between them; dust kicked up as it
 *    goes and where it stops. A tougher rank comes a shorter way (UCreatureKnockComponent::FlingShare, over the root of its
 *    size); a boss isn't moved or dazed (the chain goes taut and snaps back: only the bite).
 *  - Felt: Ember.TollChain.Throw as it leaves, a pull of the view toward the creature as it bites, a jolt as it arrives.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UEmberTollChain : public UEmberPowerBase
{
	GENERATED_BODY()

public:
	virtual EEmberDenied CanActivate(const FEmberContext& Context) const override;
	virtual void Activate(const FEmberContext& Context) override;
	virtual void Tick(const FEmberContext& Context, float DeltaSeconds) override;
	virtual void End(const FEmberContext& Context, bool bInterrupted) override;

	/** Moves the throw and the haul on by DeltaSeconds (the power's Tick; the tests call it). */
	void StepChain(float DeltaSeconds);

	ACreatureBase* GetHooked() const { return Hooked.Get(); }
	bool IsThrowing() const { return bChainOut && !bBitten; }
	bool IsHauling() const { return bChainOut && bBitten && bHauls; }
	bool HasBitten() const { return bBitten; }
	/** Where the haul ends (a stride in front of the player). */
	const FVector& GetHaulEnd() const { return HaulEnd; }

	static constexpr float DefaultDamage = 20.f;
	static constexpr float DefaultRange = 1800.f;
	static constexpr float DefaultStun = 1.5f;
	static constexpr float DefaultSeconds = 0.5f;
	/** The creature counts as in the sights within this many degrees of the aim (or its body's width of the line). */
	static constexpr float SightsDegrees = 4.f;
	/** The throw's share of the whole (the rest is the haul), at most this long. */
	static constexpr float ThrowSeconds = 0.13f;
	/** It's left this far from the player's body, between their two capsules (cm). */
	static constexpr float HaulGap = 70.f;
	/** A link drawn every so far along the chain (cm). */
	static constexpr float LinkEvery = 22.f;

protected:
	virtual bool Linger(float DeltaSeconds) override;

private:
	void Bite();
	void Release(bool bArrived);
	void DrawChain(const FVector& From, const FVector& To, float Slack) const;
	FVector HandNow() const;

	TWeakObjectPtr<ACreatureBase> Hooked;
	float Damage = DefaultDamage;
	float Stun = DefaultStun;
	float Seconds = DefaultSeconds;
	float Time = 0.f;
	float ThrowTime = ThrowSeconds;
	bool bChainOut = false;
	bool bBitten = false;
	bool bHauls = false;
	FVector HaulStart = FVector::ZeroVector;
	FVector HaulEnd = FVector::ZeroVector;
	FVector LastHaul = FVector::ZeroVector;
	float HaulDust = 0.f;
	/** After it's let go: the chain snaps back to the hand over a moment (its linger). */
	float SnapBack = -1.f;
	FVector SnapFrom = FVector::ZeroVector;
	EmberFX::FEmberSparks Sparks;
	FRandomStream Random{ 0x7011 };
};
