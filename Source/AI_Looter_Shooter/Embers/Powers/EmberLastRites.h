#pragma once

#include "CoreMinimal.h"
#include "Embers/Powers/EmberFX.h"
#include "Embers/Powers/EmberPowerBase.h"
#include "EmberLastRites.generated.h"

class ACreatureBase;
class AEmberMarkActor;
class UMaterialInstanceDynamic;

/**
 * Last Rites, Crane's Judgment power: he marks the creature in his sights, and for its duration (8 s) its end is
 * foretold.
 *  - The mark: the living creature nearest the crosshair within Range (40 m), in sight; none and the press is refused. Its
 *    body worn pale in the power's colour, its sigil over it (AEmberMarkActor with the power's Inked icon, the seconds
 *    left under it), a ring of light turning at its feet; one low bell (Ember.LastRites.Mark).
 *  - It dies within the time (by anything): its soul bursts on its pack: every other living creature within Radius
 *    (4.5 m) of it, in sight of the burst, takes Damage x the level's scale (60 at level 1; UEmberDamageType) and is
 *    thrown back a little (not a boss); a flare of soul-light, a ring racing out, Ember.LastRites.Burst. The power is
 *    done then (UEmberComponent::Finish).
 *  - It lives to the end: the mark strikes it for Strike x the level's scale (Extras "Strike", 90): a column of pale
 *    light down onto it, a shock of dust, Ember.LastRites.Strike.
 *  - Death, a scene or the level ending (End interrupted): the mark simply fades.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UEmberLastRites : public UEmberPowerBase
{
	GENERATED_BODY()

public:
	virtual EEmberDenied CanActivate(const FEmberContext& Context) const override;
	virtual void Activate(const FEmberContext& Context) override;
	virtual void Tick(const FEmberContext& Context, float DeltaSeconds) override;
	virtual void End(const FEmberContext& Context, bool bInterrupted) override;

	/** One frame of the mark: its countdown, its ring, a death settling it (its Tick; the tests call it). Returns true once settled. */
	bool StepMark(float DeltaSeconds);

	ACreatureBase* GetMarked() const { return Marked.Get(); }
	bool IsMarking() const { return bMarking; }
	/** How it was settled: the soul burst (it died in time) or the strike (it lived). */
	bool DidBurst() const { return bBurst; }
	bool DidStrike() const { return bStruck; }
	AEmberMarkActor* GetSigil() const { return Sigil.Get(); }

	static constexpr float DefaultDamage = 60.f;
	static constexpr float DefaultStrike = 90.f;
	static constexpr float DefaultRange = 4000.f;
	static constexpr float DefaultRadius = 450.f;
	static constexpr float SightsDegrees = 4.f;

protected:
	virtual bool Linger(float DeltaSeconds) override;

private:
	void Burst(const FVector& At);
	void Strike(ACreatureBase& Creature);
	void ClearMark();

	TWeakObjectPtr<ACreatureBase> Marked;
	TWeakObjectPtr<AEmberMarkActor> Sigil;
	TWeakObjectPtr<UEmberComponent> Owner;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> PaleLook;
	EmberFX::FEmberTint Pale;

	float BurstDamage = DefaultDamage;
	float StrikeDamage = DefaultStrike;
	float Radius = DefaultRadius;
	float Seconds = 8.f;
	float Time = 0.f;
	bool bMarking = false;
	bool bBurst = false;
	bool bStruck = false;
	FVector LastAt = FVector::ZeroVector;

	/** What lingers once it's settled: the burst's ring racing out, the strike's column of light fading. */
	float RingAge = -1.f;
	FVector RingAt = FVector::ZeroVector;
	float ColumnAge = -1.f;
	FVector ColumnAt = FVector::ZeroVector;
	EmberFX::FEmberSparks Sparks;
	FRandomStream Random{ 0x1a57 };
};
