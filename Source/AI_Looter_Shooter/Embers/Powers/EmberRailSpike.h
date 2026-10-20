#pragma once

#include "CoreMinimal.h"
#include "Embers/Powers/EmberFX.h"
#include "Embers/Powers/EmberPowerBase.h"
#include "EmberRailSpike.generated.h"

class ACreatureBase;

/** One spike's line, laid out before it flies: from the aim's start to where the world stops it (or its reach). */
struct AI_LOOTER_SHOOTER_API FRailSpikeLine
{
	FVector Start = FVector::ZeroVector;
	FVector End = FVector::ZeroVector;
	FVector Direction = FVector::ForwardVector;
	float Length = 0.f;
	/** It stops in the world at End (else it flies its whole reach and drops). */
	bool bEmbeds = false;
	FVector EmbedNormal = FVector::UpVector;
	/** Every living creature on the line (within its half width of the body), nearest first, and how far along each is. */
	TArray<TWeakObjectPtr<ACreatureBase>> Creatures;
	TArray<float> Along;
};

/**
 * Rail Spike, Gauge's Steam power (instant): a spike driven from its forearm on a burst of steam, through every creature in
 * a line.
 *  - The line (PlanLine): the aim, out to Range (25 m) or the first of the world's solid shapes; every living creature whose
 *    body comes within Radius (60 cm) of it, however many, is on it.
 *  - The spike flies the line at SpikeSpeed (a white-hot streak, a steam trail laid behind it, a glowing line along its way
 *    fading over a moment and a smoke thread left hanging, since the flight itself is gone in a few frames); as it passes each creature
 *    it deals Damage x the level's scale (60 at level 1; UEmberDamageType, no gun's notch) and shoves it along the line
 *    (Extras "Shove" 150 cm, "Dizzy" 0.3 s; never a boss): Ember.RailSpike.Hit, sparks and steam. In the world at the end
 *    it embeds, glowing and cooling for a while (Ember.RailSpike.Embed, sparks and dust); with nothing there it falls away.
 *  - On use: a steam piston's bang (Ember.RailSpike.Fire), steam from the forearm, a hard kick of the view.
 * Instant: the component ends it the moment it's used; the spike flies on its linger (the player's clock).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UEmberRailSpike : public UEmberPowerBase
{
	GENERATED_BODY()

public:
	virtual void Activate(const FEmberContext& Context) override;
	virtual void End(const FEmberContext& Context, bool bInterrupted) override;

	/** The line a spike from Character would fly now (see the class comment). */
	static FRailSpikeLine PlanLine(const ACharacter& Character, float Range, float HalfWidth);

	/** Moves the spike on by DeltaSeconds (its linger; the tests call it). */
	void StepSpike(float DeltaSeconds);

	bool IsFlying() const { return bFlying; }
	const FRailSpikeLine& GetLine() const { return Line; }
	/** The creatures it has gone through this use. */
	int32 NumPierced() const { return Pierced; }

	static constexpr float DefaultDamage = 60.f;
	static constexpr float DefaultRange = 2500.f;
	static constexpr float DefaultRadius = 60.f;
	static constexpr float DefaultShove = 150.f;
	static constexpr float DefaultDizzy = 0.3f;
	/** How fast the spike flies (cm/s): the whole 25 m in under a tenth of a second, still seen going. */
	static constexpr float SpikeSpeed = 28000.f;
	/** It glows where it's embedded this long, cooling. */
	static constexpr float EmbedSeconds = 2.5f;

protected:
	virtual bool Linger(float DeltaSeconds) override;

private:
	void Pierce(ACreatureBase& Creature, float Along);
	void Embed();

	FRailSpikeLine Line;
	FVector VisualStart = FVector::ZeroVector;
	float Damage = DefaultDamage;
	float Shove = DefaultShove;
	float Dizzy = DefaultDizzy;
	bool bFlying = false;
	float Flown = 0.f;
	float SteamLaid = 0.f;
	int32 NextCreature = 0;
	int32 Pierced = 0;
	/** Embedded (seconds since), or falling away (negative: not). */
	float EmbedAge = -1.f;
	float FallAge = -1.f;
	/** The glowing line it leaves along its way, fading (seconds since it was driven; negative: gone). */
	float TrailAge = -1.f;
	EmberFX::FEmberSparks Sparks;
	FRandomStream Random{ 0x5a1e };
};
